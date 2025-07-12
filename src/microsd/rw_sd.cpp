/**
 * @file rw_sd.cpp
 * @brief 可读写SD卡类实现
 * @version 2.0.0
 */

#include "rw_sd.hpp"
#include "spi_config.hpp"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "ff.h"
#include "diskio.h"
#include "tf_card.h"
#include "pio_spi.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <random>
#include <sstream>
#include <vector>
#include <iomanip>

namespace MicroSD {

// === 构造函数和析构函数 ===

RWSD::RWSD(MicroSD::SPIConfig config) 
    : config_(config), fs_type_(0), is_initialized_(false) {
    memset(&fs_, 0, sizeof(FATFS));
}

RWSD::~RWSD() {
    if (is_initialized_) {
        unmount_filesystem();
        deinitialize_spi();
    }
}

RWSD::RWSD(RWSD&& other) noexcept 
    : config_(other.config_), fs_(other.fs_), fs_type_(other.fs_type_), 
      is_initialized_(other.is_initialized_), current_dir_(std::move(other.current_dir_)),
      current_path_(std::move(other.current_path_)) {
    other.is_initialized_ = false;
    memset(&other.fs_, 0, sizeof(FATFS));
}

RWSD& RWSD::operator=(RWSD&& other) noexcept {
    if (this != &other) {
        if (is_initialized_) {
            unmount_filesystem();
            deinitialize_spi();
        }
        
        config_ = other.config_;
        fs_ = other.fs_;
        fs_type_ = other.fs_type_;
        is_initialized_ = other.is_initialized_;
        current_dir_ = std::move(other.current_dir_);
        current_path_ = std::move(other.current_path_);
        
        other.is_initialized_ = false;
        memset(&other.fs_, 0, sizeof(FATFS));
    }
    return *this;
}

// === 初始化方法 ===

void RWSD::initialize_spi() {
    // 配置pico_fatfs SPI设置
    pico_fatfs_spi_config_t spi_config = {
        config_.spi_port,
        config_.clk_slow,
        config_.clk_fast,
        config_.pins.pin_miso,
        config_.pins.pin_cs,
        config_.pins.pin_sck,
        config_.pins.pin_mosi,
        config_.pins.use_internal_pullup
    };
    
    // 设置SPI配置
    pico_fatfs_set_config(&spi_config);
    
    // 初始化SPI硬件
    if (spi_config.spi_inst != NULL) {
        // 使用硬件SPI
        spi_init(spi_config.spi_inst, spi_config.clk_slow);
        gpio_set_function(spi_config.pin_miso, GPIO_FUNC_SPI);
        gpio_set_function(spi_config.pin_sck, GPIO_FUNC_SPI);
        gpio_set_function(spi_config.pin_mosi, GPIO_FUNC_SPI);
        gpio_set_function(spi_config.pin_cs, GPIO_FUNC_SIO);
        gpio_set_dir(spi_config.pin_cs, GPIO_OUT);
        gpio_put(spi_config.pin_cs, 1);
        
        if (spi_config.pullup) {
            gpio_pull_up(spi_config.pin_miso);
            gpio_pull_up(spi_config.pin_mosi);
        }
    } else {
        // 使用PIO SPI
        pico_fatfs_config_spi_pio(pio0, 0);
    }
}

void RWSD::deinitialize_spi() {
    // 关闭SPI
    spi_deinit(config_.spi_port);
}

Result<void> RWSD::mount_filesystem() {
    FRESULT fr = f_mount(&fs_, "", 1);
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr));
    }
    
    // 获取文件系统类型
    DWORD fre_clust, fre_sect, tot_sect;
    FATFS* fs_ptr = const_cast<FATFS*>(&fs_);
    fr = f_getfree("", &fre_clust, &fs_ptr);
    if (fr == FR_OK) {
        tot_sect = (fs_.n_fatent - 2) * fs_.csize;
        fre_sect = fre_clust * fs_.csize;
        
        if (tot_sect < 4085) {
            fs_type_ = 1; // FAT12
        } else if (fre_sect < 65525) {
            fs_type_ = 2; // FAT16
        } else {
            fs_type_ = 3; // FAT32
        }
    }
    
    return Result<void>();
}

void RWSD::unmount_filesystem() {
    f_unmount("");
}

Result<void> RWSD::initialize() {
    if (is_initialized_) {
        return Result<void>();
    }
    
    // 初始化SPI
    initialize_spi();
    
    // 初始化SD卡硬件
    DSTATUS status = disk_initialize(0);
    if (status != 0) {
        deinitialize_spi();
        return Result<void>(ErrorCode::INIT_FAILED);
    }
    
    // 挂载文件系统
    auto mount_result = mount_filesystem();
    if (!mount_result.is_ok()) {
        deinitialize_spi();
        return mount_result;
    }
    
    is_initialized_ = true;
    return Result<void>();
}

// === 错误码转换 ===

ErrorCode RWSD::fresult_to_error_code(FRESULT fr) const {
    switch (fr) {
        case FR_OK: return ErrorCode::SUCCESS;
        case FR_DISK_ERR: return ErrorCode::IO_ERROR;
        case FR_INT_ERR: return ErrorCode::INIT_FAILED;
        case FR_NOT_READY: return ErrorCode::INIT_FAILED;
        case FR_NO_FILE: return ErrorCode::FILE_NOT_FOUND;
        case FR_NO_PATH: return ErrorCode::FILE_NOT_FOUND;
        case FR_INVALID_NAME: return ErrorCode::INVALID_PARAMETER;
        case FR_DENIED: return ErrorCode::PERMISSION_DENIED;
        case FR_EXIST: return ErrorCode::INVALID_PARAMETER;
        case FR_INVALID_OBJECT: return ErrorCode::INVALID_PARAMETER;
        case FR_WRITE_PROTECTED: return ErrorCode::IO_ERROR;
        case FR_INVALID_DRIVE: return ErrorCode::INVALID_PARAMETER;
        case FR_NOT_ENABLED: return ErrorCode::INIT_FAILED;
        case FR_NO_FILESYSTEM: return ErrorCode::MOUNT_FAILED;
        case FR_MKFS_ABORTED: return ErrorCode::INIT_FAILED;
        case FR_TIMEOUT: return ErrorCode::IO_ERROR;
        case FR_LOCKED: return ErrorCode::PERMISSION_DENIED;
        case FR_NOT_ENOUGH_CORE: return ErrorCode::INIT_FAILED;
        case FR_TOO_MANY_OPEN_FILES: return ErrorCode::INIT_FAILED;
        case FR_INVALID_PARAMETER: return ErrorCode::INVALID_PARAMETER;
        default: return ErrorCode::UNKNOWN_ERROR;
    }
}

// === 文件系统信息 ===

std::string RWSD::get_filesystem_type() const {
    switch (fs_type_) {
        case 1: return "FAT12";
        case 2: return "FAT16";
        case 3: return "FAT32";
        default: return "Unknown";
    }
}

Result<std::pair<size_t, size_t>> RWSD::get_capacity() const {
    if (!is_initialized_) {
        return Result<std::pair<size_t, size_t>>(ErrorCode::INIT_FAILED);
    }
    
    DWORD fre_clust, fre_sect, tot_sect;
    FATFS* fs_ptr = const_cast<FATFS*>(&fs_);
    FRESULT fr = f_getfree("", &fre_clust, &fs_ptr);
    if (fr != FR_OK) {
        return Result<std::pair<size_t, size_t>>(fresult_to_error_code(fr));
    }
    
    tot_sect = (fs_.n_fatent - 2) * fs_.csize;
    fre_sect = fre_clust * fs_.csize;
    
    size_t total_bytes = tot_sect * 512;
    size_t free_bytes = fre_sect * 512;
    
    return Result<std::pair<size_t, size_t>>({total_bytes, free_bytes});
}

// === 目录操作 ===

Result<std::vector<FileInfo>> RWSD::list_directory(const std::string& path) {
    if (!is_initialized_) {
        return Result<std::vector<FileInfo>>(ErrorCode::INIT_FAILED);
    }
    
    std::vector<FileInfo> files;
    DIR dir;
    FILINFO fno;
    
    FRESULT fr = f_opendir(&dir, path.c_str());
    if (fr != FR_OK) {
        return Result<std::vector<FileInfo>>(fresult_to_error_code(fr));
    }
    
    while (true) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) {
            break;
        }
        
        // 跳过 "." 和 ".." 目录
        if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0) {
            continue;
        }
        
        FileInfo info;
        info.name = fno.fname;
        info.full_path = path + "/" + fno.fname;
        info.size = fno.fsize;
        info.is_directory = (fno.fattrib & AM_DIR) != 0;
        
        files.push_back(info);
    }
    
    f_closedir(&dir);
    return Result<std::vector<FileInfo>>(files);
}

Result<std::string> RWSD::list_directory_tree(const std::string& path, int max_depth) const {
    if (!is_initialized_) {
        return Result<std::string>(ErrorCode::INIT_FAILED);
    }
    
    if (max_depth <= 0) {
        return Result<std::string>("[达到最大深度限制]\n");
    }
    
    std::string result;
    std::vector<FileInfo> files;
    DIR dir;
    FILINFO fno;
    
    FRESULT fr = f_opendir(&dir, path.c_str());
    if (fr != FR_OK) {
        return Result<std::string>(fresult_to_error_code(fr));
    }
    
    // 收集所有文件和目录
    while (true) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) {
            break;
        }
        
        // 跳过 "." 和 ".." 目录
        if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0) {
            continue;
        }
        
        FileInfo info;
        info.name = fno.fname;
        info.full_path = path + "/" + fno.fname;
        info.size = fno.fsize;
        info.is_directory = (fno.fattrib & AM_DIR) != 0;
        
        files.push_back(info);
    }
    
    f_closedir(&dir);
    
    // 按名称排序
    std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
        // 目录优先，然后按名称排序
        if (a.is_directory != b.is_directory) {
            return a.is_directory > b.is_directory;
        }
        return a.name < b.name;
    });
    
    // 构建树形输出
    for (size_t i = 0; i < files.size(); ++i) {
        const auto& file = files[i];
        bool is_last = (i == files.size() - 1);
        
        // 添加前缀
        std::string prefix = (max_depth == 10) ? "" : std::string((10 - max_depth) * 2, ' ');
        if (!prefix.empty()) {
            prefix += (is_last ? "└── " : "├── ");
        }
        
        // 添加文件/目录图标和名称
        std::string icon = file.is_directory ? "📁" : "📄";
        result += prefix + icon + " " + file.name;
        
        // 添加文件大小信息
        if (!file.is_directory && file.size > 0) {
            if (file.size < 1024) {
                result += " (" + std::to_string(file.size) + " B)";
            } else if (file.size < 1024 * 1024) {
                result += " (" + std::to_string(file.size / 1024) + " KB)";
            } else {
                result += " (" + std::to_string(file.size / (1024 * 1024)) + " MB)";
            }
        }
        result += "\n";
        
        // 递归处理子目录
        if (file.is_directory) {
            auto sub_result = list_directory_tree(file.full_path, max_depth - 1);
            if (sub_result.is_ok()) {
                std::string sub_prefix = (max_depth == 10) ? "" : std::string((10 - max_depth) * 2, ' ');
                if (!sub_prefix.empty()) {
                    sub_prefix += (is_last ? "    " : "│   ");
                }
                
                // 为子目录的每一行添加前缀
                std::string sub_content = *sub_result;
                size_t pos = 0;
                while ((pos = sub_content.find('\n', pos)) != std::string::npos) {
                    if (pos > 0) {
                        sub_content.insert(pos, sub_prefix);
                        pos += sub_prefix.length() + 1;
                    } else {
                        pos++;
                    }
                }
                result += sub_content;
            }
        }
    }
    
    return Result<std::string>(result);
}

Result<void> RWSD::create_directory(const std::string& path) {
    if (!is_initialized_) {
        return Result<void>(ErrorCode::INIT_FAILED);
    }
    
    FRESULT fr = f_mkdir(path.c_str());
    return Result<void>(fresult_to_error_code(fr));
}

Result<void> RWSD::remove_directory(const std::string& path) {
    if (!is_initialized_) {
        return Result<void>(ErrorCode::INIT_FAILED);
    }
    
    FRESULT fr = f_rmdir(path.c_str());
    return Result<void>(fresult_to_error_code(fr));
}

bool RWSD::file_exists(const std::string& path) const {
    if (!is_initialized_) {
        return false;
    }
    
    FILINFO fno;
    FRESULT fr = f_stat(path.c_str(), &fno);
    return fr == FR_OK;
}

Result<FileInfo> RWSD::get_file_info(const std::string& path) const {
    if (!is_initialized_) {
        return Result<FileInfo>(ErrorCode::INIT_FAILED);
    }
    
    FILINFO fno;
    FRESULT fr = f_stat(path.c_str(), &fno);
    if (fr != FR_OK) {
        return Result<FileInfo>(fresult_to_error_code(fr));
    }
    
    FileInfo info;
    info.name = fno.fname;
    info.full_path = path;
    info.size = fno.fsize;
    info.is_directory = (fno.fattrib & AM_DIR) != 0;
    
    return Result<FileInfo>(info);
}

// === 一次性读写操作 ===

Result<std::vector<uint8_t>> RWSD::read_file(const std::string& path) const {
    if (!is_initialized_) {
        return Result<std::vector<uint8_t>>(ErrorCode::INIT_FAILED);
    }
    
    FIL file;
    FRESULT fr = f_open(&file, path.c_str(), FA_READ);
    if (fr != FR_OK) {
        return Result<std::vector<uint8_t>>(fresult_to_error_code(fr));
    }
    
    UINT bytes_read;
    std::vector<uint8_t> data(f_size(&file));
    
    fr = f_read(&file, data.data(), f_size(&file), &bytes_read);
    f_close(&file);
    
    if (fr != FR_OK) {
        return Result<std::vector<uint8_t>>(fresult_to_error_code(fr));
    }
    
    data.resize(bytes_read);
    return Result<std::vector<uint8_t>>(data);
}

Result<std::vector<uint8_t>> RWSD::read_file_chunk(const std::string& path, 
                                                   size_t offset, size_t size) const {
    if (!is_initialized_) {
        return Result<std::vector<uint8_t>>(ErrorCode::INIT_FAILED);
    }
    
    FIL file;
    FRESULT fr = f_open(&file, path.c_str(), FA_READ);
    if (fr != FR_OK) {
        return Result<std::vector<uint8_t>>(fresult_to_error_code(fr));
    }
    
    fr = f_lseek(&file, offset);
    if (fr != FR_OK) {
        f_close(&file);
        return Result<std::vector<uint8_t>>(fresult_to_error_code(fr));
    }
    
    UINT bytes_read;
    std::vector<uint8_t> data(size);
    
    fr = f_read(&file, data.data(), size, &bytes_read);
    f_close(&file);
    
    if (fr != FR_OK) {
        return Result<std::vector<uint8_t>>(fresult_to_error_code(fr));
    }
    
    data.resize(bytes_read);
    return Result<std::vector<uint8_t>>(data);
}

Result<std::string> RWSD::read_text_file(const std::string& path) const {
    auto result = read_file(path);
    if (!result.is_ok()) {
        return Result<std::string>(result.error_code());
    }
    
    std::string text(reinterpret_cast<const char*>(result->data()), result->size());
    return Result<std::string>(text);
}

Result<void> RWSD::write_file(const std::string& path, const std::vector<uint8_t>& data) {
    if (!is_initialized_) {
        return Result<void>(ErrorCode::INIT_FAILED);
    }
    
    FIL file;
    FRESULT fr = f_open(&file, path.c_str(), FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr));
    }
    
    UINT bytes_written;
    fr = f_write(&file, data.data(), data.size(), &bytes_written);
    f_close(&file);
    
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr));
    }
    
    return Result<void>();
}

Result<void> RWSD::write_text_file(const std::string& path, const std::string& content) {
    std::vector<uint8_t> data(content.begin(), content.end());
    return write_file(path, data);
}

Result<void> RWSD::append_file(const std::string& path, const std::vector<uint8_t>& data) {
    if (!is_initialized_) {
        return Result<void>(ErrorCode::INIT_FAILED);
    }
    
    FIL file;
    FRESULT fr = f_open(&file, path.c_str(), FA_WRITE | FA_OPEN_APPEND);
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr));
    }
    
    UINT bytes_written;
    fr = f_write(&file, data.data(), data.size(), &bytes_written);
    f_close(&file);
    
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr));
    }
    
    return Result<void>();
}

Result<void> RWSD::append_text_file(const std::string& path, const std::string& content) {
    std::vector<uint8_t> data(content.begin(), content.end());
    return append_file(path, data);
}

Result<void> RWSD::delete_file(const std::string& path) {
    if (!is_initialized_) {
        return Result<void>(ErrorCode::INIT_FAILED);
    }
    
    FRESULT fr = f_unlink(path.c_str());
    return Result<void>(fresult_to_error_code(fr));
}

Result<void> RWSD::rename(const std::string& old_path, const std::string& new_path) {
    if (!is_initialized_) {
        return Result<void>(ErrorCode::INIT_FAILED);
    }
    
    FRESULT fr = f_rename(old_path.c_str(), new_path.c_str());
    return Result<void>(fresult_to_error_code(fr));
}

Result<void> RWSD::copy_file(const std::string& src_path, const std::string& dst_path) {
    auto read_result = read_file(src_path);
    if (!read_result.is_ok()) {
        return Result<void>(read_result.error_code());
    }
    
    return write_file(dst_path, *read_result);
}

Result<void> RWSD::sync() {
    if (!is_initialized_) {
        return Result<void>(ErrorCode::INIT_FAILED);
    }
    
    // 同步所有打开的文件
    f_sync(nullptr);
    return Result<void>();
}

// === 文件句柄类实现 ===

RWSD::FileHandle::FileHandle(FileHandle&& other) noexcept 
    : file_(other.file_), is_open_(other.is_open_), 
      path_(std::move(other.path_)), mode_(std::move(other.mode_)) {
    other.is_open_ = false;
}

Result<void> RWSD::FileHandle::open(const std::string& path, const std::string& mode) {
    if (is_open_) {
        close();
    }
    
    BYTE flags = 0;
    if (mode.find('r') != std::string::npos) flags |= FA_READ;
    if (mode.find('w') != std::string::npos) flags |= FA_WRITE;
    if (mode.find('a') != std::string::npos) flags |= FA_WRITE | FA_OPEN_APPEND;
    if (mode.find('+') != std::string::npos) flags |= FA_READ | FA_WRITE;
    
    if (mode.find('w') != std::string::npos && mode.find('a') == std::string::npos) {
        flags |= FA_CREATE_ALWAYS;
    } else if (mode.find('a') != std::string::npos) {
        flags |= FA_OPEN_APPEND;
    } else {
        flags |= FA_OPEN_EXISTING;
    }
    
    FRESULT fr = f_open(&file_, path.c_str(), flags);
    if (fr != FR_OK) {
        return Result<void>(static_cast<ErrorCode>(fr));
    }
    
    is_open_ = true;
    path_ = path;
    mode_ = mode;
    
    return Result<void>();
}

void RWSD::FileHandle::close() {
    if (is_open_) {
        f_close(&file_);
        is_open_ = false;
        path_.clear();
        mode_.clear();
    }
}

Result<std::vector<uint8_t>> RWSD::FileHandle::read(size_t size) {
    if (!is_open_) {
        return Result<std::vector<uint8_t>>(ErrorCode::INVALID_PARAMETER);
    }
    
    std::vector<uint8_t> data(size);
    UINT bytes_read;
    
    FRESULT fr = f_read(&file_, data.data(), size, &bytes_read);
    if (fr != FR_OK) {
        return Result<std::vector<uint8_t>>(static_cast<ErrorCode>(fr));
    }
    
    data.resize(bytes_read);
    return Result<std::vector<uint8_t>>(data);
}

Result<size_t> RWSD::FileHandle::read_text(std::string& text, size_t max_size) {
    auto result = read(max_size);
    if (!result.is_ok()) {
        return Result<size_t>(result.error_code());
    }
    
    text.assign(reinterpret_cast<const char*>(result->data()), result->size());
    return Result<size_t>(result->size());
}

Result<size_t> RWSD::FileHandle::write(const std::vector<uint8_t>& data) {
    if (!is_open_) {
        return Result<size_t>(ErrorCode::INVALID_PARAMETER);
    }
    
    UINT bytes_written;
    FRESULT fr = f_write(&file_, data.data(), data.size(), &bytes_written);
    if (fr != FR_OK) {
        return Result<size_t>(static_cast<ErrorCode>(fr));
    }
    
    return Result<size_t>(bytes_written);
}

Result<size_t> RWSD::FileHandle::write(const std::string& text) {
    std::vector<uint8_t> data(text.begin(), text.end());
    return write(data);
}

Result<size_t> RWSD::FileHandle::write_line(const std::string& line) {
    std::string text = line + "\n";
    return write(text);
}

Result<void> RWSD::FileHandle::seek(size_t position) {
    if (!is_open_) {
        return Result<void>(ErrorCode::INVALID_PARAMETER);
    }
    
    FRESULT fr = f_lseek(&file_, position);
    return Result<void>(static_cast<ErrorCode>(fr));
}

Result<size_t> RWSD::FileHandle::tell() const {
    if (!is_open_) {
        return Result<size_t>(ErrorCode::INVALID_PARAMETER);
    }
    
    return Result<size_t>(f_tell(&file_));
}

Result<size_t> RWSD::FileHandle::size() const {
    if (!is_open_) {
        return Result<size_t>(ErrorCode::INVALID_PARAMETER);
    }
    
    return Result<size_t>(f_size(&file_));
}

Result<void> RWSD::FileHandle::flush() {
    if (!is_open_) {
        return Result<void>(ErrorCode::INVALID_PARAMETER);
    }
    
    FRESULT fr = f_sync(&file_);
    return Result<void>(static_cast<ErrorCode>(fr));
}

Result<void> RWSD::FileHandle::truncate(size_t size) {
    if (!is_open_) {
        return Result<void>(ErrorCode::INVALID_PARAMETER);
    }
    
    FRESULT fr = f_truncate(&file_);
    return Result<void>(static_cast<ErrorCode>(fr));
}

Result<RWSD::FileHandle> RWSD::open_file(const std::string& path, const std::string& mode) {
    if (!is_initialized_) {
        return Result<FileHandle>(ErrorCode::INIT_FAILED);
    }
    
    FileHandle handle;
    auto result = handle.open(path, mode);
    if (!result.is_ok()) {
        return Result<FileHandle>(result.error_code());
    }
    
    return Result<FileHandle>(std::move(handle));
}

// === 高级功能 ===

Result<void> RWSD::format(const std::string& volume_label) {
    if (!is_initialized_) {
        return Result<void>(ErrorCode::INIT_FAILED);
    }
    
    BYTE work[FF_MAX_SS];
    MKFS_PARM opt = {0};
    opt.fmt = FM_FAT32;
    opt.n_fat = 1;
    opt.align = 0;
    opt.n_root = 0;
    opt.au_size = 0;
    
    FRESULT fr = f_mkfs("", &opt, work, sizeof(work));
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr));
    }
    
    // 设置卷标
    if (!volume_label.empty()) {
        fr = f_setlabel(volume_label.c_str());
        if (fr != FR_OK) {
            return Result<void>(fresult_to_error_code(fr));
        }
    }
    
    return Result<void>();
}

Result<std::string> RWSD::get_filesystem_status() const {
    if (!is_initialized_) {
        return Result<std::string>(ErrorCode::INIT_FAILED);
    }
    
    auto capacity_result = get_capacity();
    if (!capacity_result.is_ok()) {
        return Result<std::string>(capacity_result.error_code());
    }
    
    auto [total, free] = *capacity_result;
    double usage_percent = total > 0 ? (double)(total - free) / total * 100.0 : 0.0;
    
    std::ostringstream oss;
    oss << "文件系统: " << get_filesystem_type() << "\n";
    oss << "总容量: " << (total / 1024 / 1024) << " MB\n";
    oss << "可用容量: " << (free / 1024 / 1024) << " MB\n";
    oss << "使用率: " << std::fixed << std::setprecision(1) << usage_percent << "%\n";
    
    return Result<std::string>(oss.str());
}

Result<bool> RWSD::check_filesystem_integrity() const {
    if (!is_initialized_) {
        return Result<bool>(ErrorCode::INIT_FAILED);
    }
    
    // 简单的完整性检查：尝试获取空闲簇信息
    DWORD fre_clust;
    FATFS* fs_ptr = const_cast<FATFS*>(&fs_);
    FRESULT fr = f_getfree("", &fre_clust, &fs_ptr);
    return Result<bool>(fr == FR_OK);
}

// === 工具方法 ===

std::string RWSD::get_config_info() const {
    std::ostringstream oss;
    oss << "=== SPI配置信息 ===\n";
    oss << "SPI实例: " << (config_.spi_port == spi0 ? "SPI0" : "SPI1") << "\n";
    oss << "MOSI引脚: " << (int)config_.pins.pin_mosi << "\n";
    oss << "MISO引脚: " << (int)config_.pins.pin_miso << "\n";
    oss << "SCLK引脚: " << (int)config_.pins.pin_sck << "\n";
    oss << "CS引脚: " << (int)config_.pins.pin_cs << "\n";
    oss << "波特率: " << config_.clk_slow << " Hz\n";
    return oss.str();
}

std::string RWSD::get_status_info() const {
    std::ostringstream oss;
    oss << "=== SD卡状态信息 ===\n";
    oss << "初始化状态: " << (is_initialized_ ? "已初始化" : "未初始化") << "\n";
    
    if (is_initialized_) {
        oss << "文件系统类型: " << get_filesystem_type() << "\n";
        
        auto capacity_result = get_capacity();
        if (capacity_result.is_ok()) {
            auto [total, free] = *capacity_result;
            double usage_percent = total > 0 ? (double)(total - free) / total * 100.0 : 0.0;
            oss << "总容量: " << (total / 1024 / 1024) << " MB\n";
            oss << "可用容量: " << (free / 1024 / 1024) << " MB\n";
            oss << "使用率: " << std::fixed << std::setprecision(1) << usage_percent << "%\n";
        }
    }
    
    return oss.str();
}

std::string RWSD::get_memory_usage() const {
    std::ostringstream oss;
    oss << "=== 内存使用情况 ===\n";
    // 使用标准C库函数获取内存信息
    oss << "堆内存: 可用 (具体大小需要运行时获取)\n";
    return oss.str();
}

} // namespace MicroSD 