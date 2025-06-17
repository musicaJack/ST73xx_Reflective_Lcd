/**
 * @file micro_sd.cpp
 * @brief Modern C++ MicroSD Card Library Implementation
 * @author Refactored from RPi_Pico_WAV_Player project
 * @version 1.0.0
 */

#include "micro_sd.hpp"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>

#include "hardware/gpio.h"
#include "pico/time.h"
#include "tf_card.h"  // Header from pico_fatfs library

namespace MicroSD {

// === Error Handling ===

ErrorCode fresult_to_error_code(FRESULT fr) {
    switch (fr) {
        case FR_OK: return ErrorCode::SUCCESS;
        case FR_NO_FILE: return ErrorCode::FILE_NOT_FOUND;
        case FR_NO_PATH: return ErrorCode::FILE_NOT_FOUND;
        case FR_INVALID_NAME: return ErrorCode::INVALID_PARAMETER;
        case FR_DENIED: return ErrorCode::PERMISSION_DENIED;
        case FR_DISK_ERR: return ErrorCode::IO_ERROR;
        case FR_NOT_READY: return ErrorCode::INIT_FAILED;
        case FR_WRITE_PROTECTED: return ErrorCode::IO_ERROR;
        default: return ErrorCode::UNKNOWN_ERROR;
    }
}

std::string SDCard::get_error_description(ErrorCode error) {
    switch (error) {
        case ErrorCode::SUCCESS: return "Operation successful";
        case ErrorCode::INIT_FAILED: return "Initialization failed";
        case ErrorCode::MOUNT_FAILED: return "Mount failed";
        case ErrorCode::FILE_NOT_FOUND: return "File or directory not found";
        case ErrorCode::IO_ERROR: return "I/O error";
        case ErrorCode::DISK_FULL: return "Disk is full";
        case ErrorCode::INVALID_PARAMETER: return "Invalid parameter";
        case ErrorCode::PERMISSION_DENIED: return "Permission denied";
        case ErrorCode::FATFS_ERROR: return "FATFS error";
        case ErrorCode::UNKNOWN_ERROR: return "Unknown error";
        default: return "Undefined error";
    }
}

// === Path Utility Functions ===

std::string SDCard::normalize_path(const std::string& path) {
    if (path.empty() || path == ".") {
        return "/";
    }
    
    std::string normalized = path;
    
    // Ensure path starts with /
    if (normalized[0] != '/') {
        normalized = "/" + normalized;
    }
    
    // Remove trailing / (except for root)
    if (normalized.length() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    
    // Handle repeated slashes //
    size_t pos = 0;
    while ((pos = normalized.find("//", pos)) != std::string::npos) {
        normalized.replace(pos, 2, "/");
    }
    
    return normalized;
}

std::string SDCard::join_path(const std::string& dir, const std::string& file) {
    if (dir.empty()) return normalize_path(file);
    if (file.empty()) return normalize_path(dir);
    
    std::string result = dir;
    if (result.back() != '/') {
        result += "/";
    }
    result += file;
    
    return normalize_path(result);
}

std::pair<std::string, std::string> SDCard::split_path(const std::string& path) {
    std::string normalized = normalize_path(path);
    
    size_t pos = normalized.find_last_of('/');
    if (pos == std::string::npos || pos == 0) {
        return {"/", normalized.substr(1)};
    }
    
    return {normalized.substr(0, pos), normalized.substr(pos + 1)};
}

// === SDCard Class Implementation ===

SDCard::SDCard(SPIConfig config) 
    : config_(std::move(config)), is_mounted_(false), fs_type_(0), current_path_("/") {
}

SDCard::~SDCard() {
    if (is_mounted_) {
        unmount_filesystem();
        deinitialize_spi();
    }
}

SDCard::SDCard(SDCard&& other) noexcept 
    : config_(std::move(other.config_))
    , fs_(std::move(other.fs_))
    , is_mounted_(other.is_mounted_)
    , fs_type_(other.fs_type_)
    , current_dir_(std::move(other.current_dir_))
    , current_path_(std::move(other.current_path_)) {
    other.is_mounted_ = false;
}

SDCard& SDCard::operator=(SDCard&& other) noexcept {
    if (this != &other) {
        // Clean up current resources
        if (is_mounted_) {
            unmount_filesystem();
            deinitialize_spi();
        }
        
        // Move resources
        config_ = std::move(other.config_);
        fs_ = std::move(other.fs_);
        is_mounted_ = other.is_mounted_;
        fs_type_ = other.fs_type_;
        current_dir_ = std::move(other.current_dir_);
        current_path_ = std::move(other.current_path_);
        
        other.is_mounted_ = false;
    }
    return *this;
}

void SDCard::initialize_spi() {
    // Configure SPI pins
    gpio_set_function(config_.pin_miso, GPIO_FUNC_SPI);
    gpio_set_function(config_.pin_sck, GPIO_FUNC_SPI);
    gpio_set_function(config_.pin_mosi, GPIO_FUNC_SPI);
    gpio_set_function(config_.pin_cs, GPIO_FUNC_SPI);
    
    // If using internal pullup
    if (config_.use_internal_pullup) {
        gpio_pull_up(config_.pin_miso);
        gpio_pull_up(config_.pin_cs);
    }
    
    // Initialize SPI
    spi_init(config_.spi_port, config_.clk_slow);
    
    // Configure pico_fatfs
    pico_fatfs_spi_config_t fatfs_config = {
        config_.spi_port,
        config_.clk_slow,
        config_.clk_fast,
        config_.pin_miso,
        config_.pin_cs,
        config_.pin_sck,
        config_.pin_mosi,
        config_.use_internal_pullup
    };
    
    pico_fatfs_set_config(&fatfs_config);
}

void SDCard::deinitialize_spi() {
    pico_fatfs_reboot_spi();
    spi_deinit(config_.spi_port);
}

Result<void> SDCard::mount_filesystem() {
    FRESULT fr;
    
    // Try mounting multiple times (based on original retry mechanism)
    for (int i = 0; i < 5; i++) {
        fr = f_mount(&fs_, "", 1);
        if (fr == FR_OK) {
            fs_type_ = fs_.fs_type;
            is_mounted_ = true;
            return Result<void>();
        }
        pico_fatfs_reboot_spi();
        sleep_ms(10);
    }
    
    return Result<void>(fresult_to_error_code(fr), 
                        "SD card mount failed, FRESULT: " + std::to_string(fr));
}

void SDCard::unmount_filesystem() {
    if (is_mounted_) {
        f_unmount("");
        is_mounted_ = false;
    }
    pico_fatfs_reboot_spi();
}

Result<void> SDCard::initialize() {
    if (is_mounted_) {
        return Result<void>(ErrorCode::SUCCESS);
    }
    
    initialize_spi();
    
    // Give some SD cards time to initialize
    sleep_ms(100);
    
    auto result = mount_filesystem();
    if (result.is_error()) {
        deinitialize_spi();
        return result;
    }
    
    return Result<void>();
}

std::string SDCard::get_filesystem_type() const {
    if (!is_mounted_) {
        return "Not mounted";
    }
    
    switch (fs_type_) {
        case FS_FAT12: return "FAT12";
        case FS_FAT16: return "FAT16"; 
        case FS_FAT32: return "FAT32";
        case FS_EXFAT: return "exFAT";
        default: return "Unknown(" + std::to_string(fs_type_) + ")";
    }
}

Result<std::pair<size_t, size_t>> SDCard::get_capacity() const {
    if (!is_mounted_) {
        return Result<std::pair<size_t, size_t>>(ErrorCode::MOUNT_FAILED, "SD card not mounted");
    }
    
    FATFS* fs;
    DWORD free_clusters;
    FRESULT fr = f_getfree("", &free_clusters, &fs);
    
    if (fr != FR_OK) {
        return Result<std::pair<size_t, size_t>>(fresult_to_error_code(fr), 
                                                  "获取容量信息失败");
    }
    
    // 计算总容量和可用容量
    // fs->n_fatent: FAT表中的总条目数（包括保留的前两个条目）
    // fs->csize: 每个簇的扇区数
    // 512: 每个扇区的字节数（标准）
    
    size_t total_clusters = fs->n_fatent - 2;  // 减去保留的两个FAT条目
    size_t sectors_per_cluster = fs->csize;
    size_t bytes_per_sector = 512;  // 标准扇区大小
    
    size_t total_bytes = total_clusters * sectors_per_cluster * bytes_per_sector;
    size_t free_bytes = (size_t)free_clusters * sectors_per_cluster * bytes_per_sector;
    
    // 调试信息（可以通过编译宏控制）
    #ifdef MICRO_SD_DEBUG
    printf("Debug - Capacity calculation:\n");
    printf("  Total FAT entries: %lu\n", (unsigned long)fs->n_fatent);
    printf("  Data clusters: %zu\n", total_clusters);
    printf("  Free clusters: %lu\n", (unsigned long)free_clusters);
    printf("  Sectors per cluster: %zu\n", sectors_per_cluster);
    printf("  Bytes per sector: %zu\n", bytes_per_sector);
    printf("  Calculated total: %zu bytes (%.2f MB)\n", 
           total_bytes, total_bytes / 1024.0 / 1024.0);
    printf("  Calculated free: %zu bytes (%.2f MB)\n", 
           free_bytes, free_bytes / 1024.0 / 1024.0);
    #endif
    
    return Result<std::pair<size_t, size_t>>({total_bytes, free_bytes});
}

// === Directory Operations ===

Result<void> SDCard::open_directory(const std::string& path) {
    if (!is_mounted_) {
        return Result<void>(ErrorCode::MOUNT_FAILED, "SD卡未挂载");
    }
    
    std::string normalized_path = normalize_path(path);
    
    // 如果已经有打开的目录，先关闭
    if (current_dir_) {
        f_closedir(current_dir_.get());
        current_dir_.reset();
    }
    
    current_dir_ = std::make_unique<DIR>();
    FRESULT fr = f_opendir(current_dir_.get(), normalized_path.c_str());
    
    if (fr != FR_OK) {
        current_dir_.reset();
        return Result<void>(fresult_to_error_code(fr), 
                           "打开目录失败: " + normalized_path);
    }
    
    current_path_ = normalized_path;
    return Result<void>();
}

Result<std::vector<FileInfo>> SDCard::list_directory(const std::string& path) {
    if (!is_mounted_) {
        return Result<std::vector<FileInfo>>(ErrorCode::MOUNT_FAILED, "SD卡未挂载");
    }
    
    std::string target_path = path.empty() ? current_path_ : normalize_path(path);
    
    DIR dir;
    FRESULT fr = f_opendir(&dir, target_path.c_str());
    if (fr != FR_OK) {
        return Result<std::vector<FileInfo>>(fresult_to_error_code(fr), 
                                            "打开目录失败: " + target_path);
    }
    
    std::vector<FileInfo> files;
    FILINFO fno;
    
    while (true) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) break;
        
        // 跳过隐藏文件和当前目录项
        if (fno.fname[0] == '.' && 
            (fno.fname[1] == 0 || (fno.fname[1] == '.' && fno.fname[2] == 0))) {
            continue;
        }
        
        FileInfo info;
        info.name = fno.fname;
        info.full_path = join_path(target_path, fno.fname);
        info.size = fno.fsize;
        info.is_directory = (fno.fattrib & AM_DIR) != 0;
        info.attributes = fno.fattrib;
        
        files.push_back(std::move(info));
    }
    
    f_closedir(&dir);
    
    // 排序：目录在前，然后按名称排序
    std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
        if (a.is_directory != b.is_directory) {
            return a.is_directory > b.is_directory;
        }
        return a.name < b.name;
    });
    
    return Result<std::vector<FileInfo>>(std::move(files));
}

Result<void> SDCard::create_directory(const std::string& path) {
    if (!is_mounted_) {
        return Result<void>(ErrorCode::MOUNT_FAILED, "SD卡未挂载");
    }
    
    std::string normalized_path = normalize_path(path);
    FRESULT fr = f_mkdir(normalized_path.c_str());
    
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr), 
                           "创建目录失败: " + normalized_path);
    }
    
    return Result<void>();
}

Result<void> SDCard::remove_directory(const std::string& path) {
    if (!is_mounted_) {
        return Result<void>(ErrorCode::MOUNT_FAILED, "SD卡未挂载");
    }
    
    std::string normalized_path = normalize_path(path);
    FRESULT fr = f_unlink(normalized_path.c_str());
    
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr), 
                           "删除目录失败: " + normalized_path);
    }
    
    return Result<void>();
}

// === File Operations ===

bool SDCard::file_exists(const std::string& path) const {
    if (!is_mounted_) return false;
    
    FILINFO fno;
    FRESULT fr = f_stat(normalize_path(path).c_str(), &fno);
    return fr == FR_OK;
}

Result<FileInfo> SDCard::get_file_info(const std::string& path) const {
    if (!is_mounted_) {
        return Result<FileInfo>(ErrorCode::MOUNT_FAILED, "SD卡未挂载");
    }
    
    std::string normalized_path = normalize_path(path);
    FILINFO fno;
    FRESULT fr = f_stat(normalized_path.c_str(), &fno);
    
    if (fr != FR_OK) {
        return Result<FileInfo>(fresult_to_error_code(fr), 
                               "获取文件信息失败: " + normalized_path);
    }
    
    FileInfo info;
    info.name = fno.fname;
    info.full_path = normalized_path;
    info.size = fno.fsize;
    info.is_directory = (fno.fattrib & AM_DIR) != 0;
    info.attributes = fno.fattrib;
    
    return Result<FileInfo>(std::move(info));
}

Result<std::vector<uint8_t>> SDCard::read_file(const std::string& path) const {
    if (!is_mounted_) {
        return Result<std::vector<uint8_t>>(ErrorCode::MOUNT_FAILED, "SD卡未挂载");
    }
    
    FIL file;
    std::string normalized_path = normalize_path(path);
    FRESULT fr = f_open(&file, normalized_path.c_str(), FA_READ);
    
    if (fr != FR_OK) {
        return Result<std::vector<uint8_t>>(fresult_to_error_code(fr), 
                                           "打开文件失败: " + normalized_path);
    }
    
    size_t file_size = f_size(&file);
    std::vector<uint8_t> data(file_size);
    
    UINT bytes_read;
    fr = f_read(&file, data.data(), file_size, &bytes_read);
    f_close(&file);
    
    if (fr != FR_OK) {
        return Result<std::vector<uint8_t>>(fresult_to_error_code(fr), 
                                           "读取文件失败: " + normalized_path);
    }
    
    data.resize(bytes_read);
    return Result<std::vector<uint8_t>>(std::move(data));
}

Result<std::vector<uint8_t>> SDCard::read_file_chunk(const std::string& path, 
                                                      size_t offset, size_t size) const {
    if (!is_mounted_) {
        return Result<std::vector<uint8_t>>(ErrorCode::MOUNT_FAILED, "SD卡未挂载");
    }
    
    FIL file;
    std::string normalized_path = normalize_path(path);
    FRESULT fr = f_open(&file, normalized_path.c_str(), FA_READ);
    
    if (fr != FR_OK) {
        return Result<std::vector<uint8_t>>(fresult_to_error_code(fr), 
                                           "打开文件失败: " + normalized_path);
    }
    
    // 定位到指定偏移
    fr = f_lseek(&file, offset);
    if (fr != FR_OK) {
        f_close(&file);
        return Result<std::vector<uint8_t>>(fresult_to_error_code(fr), 
                                           "文件定位失败");
    }
    
    std::vector<uint8_t> data(size);
    UINT bytes_read;
    fr = f_read(&file, data.data(), size, &bytes_read);
    f_close(&file);
    
    if (fr != FR_OK) {
        return Result<std::vector<uint8_t>>(fresult_to_error_code(fr), 
                                           "读取文件失败");
    }
    
    data.resize(bytes_read);
    return Result<std::vector<uint8_t>>(std::move(data));
}

Result<void> SDCard::write_file(const std::string& path, 
                               const std::vector<uint8_t>& data, 
                               bool append) {
    if (!is_mounted_) {
        return Result<void>(ErrorCode::MOUNT_FAILED, "SD卡未挂载");
    }
    
    FIL file;
    std::string normalized_path = normalize_path(path);
    BYTE mode = append ? (FA_WRITE | FA_OPEN_ALWAYS) : (FA_WRITE | FA_CREATE_ALWAYS);
    
    FRESULT fr = f_open(&file, normalized_path.c_str(), mode);
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr), 
                           "打开文件失败: " + normalized_path);
    }
    
    // 如果是追加模式，定位到文件末尾
    if (append) {
        fr = f_lseek(&file, f_size(&file));
        if (fr != FR_OK) {
            f_close(&file);
            return Result<void>(fresult_to_error_code(fr), "文件定位失败");
        }
    }
    
    UINT bytes_written;
    fr = f_write(&file, data.data(), data.size(), &bytes_written);
    f_close(&file);
    
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr), 
                           "写入文件失败: " + normalized_path);
    }
    
    if (bytes_written != data.size()) {
        return Result<void>(ErrorCode::IO_ERROR, 
                           "写入数据不完整");
    }
    
    return Result<void>();
}

Result<void> SDCard::write_text_file(const std::string& path, 
                                     const std::string& content, 
                                     bool append) {
    std::vector<uint8_t> data(content.begin(), content.end());
    return write_file(path, data, append);
}

Result<void> SDCard::delete_file(const std::string& path) {
    if (!is_mounted_) {
        return Result<void>(ErrorCode::MOUNT_FAILED, "SD卡未挂载");
    }
    
    std::string normalized_path = normalize_path(path);
    FRESULT fr = f_unlink(normalized_path.c_str());
    
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr), 
                           "删除文件失败: " + normalized_path);
    }
    
    return Result<void>();
}

Result<void> SDCard::rename(const std::string& old_path, const std::string& new_path) {
    if (!is_mounted_) {
        return Result<void>(ErrorCode::MOUNT_FAILED, "SD卡未挂载");
    }
    
    std::string old_normalized = normalize_path(old_path);
    std::string new_normalized = normalize_path(new_path);
    
    FRESULT fr = f_rename(old_normalized.c_str(), new_normalized.c_str());
    
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr), 
                           "重命名失败: " + old_normalized + " -> " + new_normalized);
    }
    
    return Result<void>();
}

Result<void> SDCard::copy_file(const std::string& src_path, const std::string& dst_path) {
    // 读取源文件
    auto src_data = read_file(src_path);
    if (src_data.is_error()) {
        return Result<void>(src_data.error_code(), 
                           "复制文件失败，读取源文件错误: " + src_data.error_message());
    }
    
    // 写入目标文件
    auto write_result = write_file(dst_path, *src_data, false);
    if (write_result.is_error()) {
        return Result<void>(write_result.error_code(), 
                           "复制文件失败，写入目标文件错误: " + write_result.error_message());
    }
    
    return Result<void>();
}

// === Stream File Operations FileHandle Implementation ===

SDCard::FileHandle::FileHandle(FileHandle&& other) noexcept 
    : file_(std::move(other.file_))
    , is_open_(other.is_open_)
    , path_(std::move(other.path_)) {
    other.is_open_ = false;
}

SDCard::FileHandle& SDCard::FileHandle::operator=(FileHandle&& other) noexcept {
    if (this != &other) {
        close();
        file_ = std::move(other.file_);
        is_open_ = other.is_open_;
        path_ = std::move(other.path_);
        other.is_open_ = false;
    }
    return *this;
}

Result<void> SDCard::FileHandle::open(const std::string& path, const std::string& mode) {
    if (is_open_) {
        close();
    }
    
    BYTE fatfs_mode = 0;
    if (mode == "r") {
        fatfs_mode = FA_READ;
    } else if (mode == "w") {
        fatfs_mode = FA_WRITE | FA_CREATE_ALWAYS;
    } else if (mode == "a") {
        fatfs_mode = FA_WRITE | FA_OPEN_ALWAYS;
    } else if (mode == "r+") {
        fatfs_mode = FA_READ | FA_WRITE;
    } else if (mode == "w+") {
        fatfs_mode = FA_READ | FA_WRITE | FA_CREATE_ALWAYS;
    } else if (mode == "a+") {
        fatfs_mode = FA_READ | FA_WRITE | FA_OPEN_ALWAYS;
    } else {
        return Result<void>(ErrorCode::INVALID_PARAMETER, "无效的文件打开模式: " + mode);
    }
    
    std::string normalized_path = SDCard::normalize_path(path);
    FRESULT fr = f_open(&file_, normalized_path.c_str(), fatfs_mode);
    
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr), 
                           "打开文件失败: " + normalized_path);
    }
    
    // 如果是追加模式，定位到文件末尾
    if (mode == "a" || mode == "a+") {
        fr = f_lseek(&file_, f_size(&file_));
        if (fr != FR_OK) {
            f_close(&file_);
            return Result<void>(fresult_to_error_code(fr), "文件定位失败");
        }
    }
    
    is_open_ = true;
    path_ = normalized_path;
    return Result<void>();
}

void SDCard::FileHandle::close() {
    if (is_open_) {
        f_close(&file_);
        is_open_ = false;
        path_.clear();
    }
}

Result<std::vector<uint8_t>> SDCard::FileHandle::read(size_t size) {
    if (!is_open_) {
        return Result<std::vector<uint8_t>>(ErrorCode::PERMISSION_DENIED, "文件未打开");
    }
    
    std::vector<uint8_t> data(size);
    UINT bytes_read;
    FRESULT fr = f_read(&file_, data.data(), size, &bytes_read);
    
    if (fr != FR_OK) {
        return Result<std::vector<uint8_t>>(fresult_to_error_code(fr), "读取文件失败");
    }
    
    data.resize(bytes_read);
    return Result<std::vector<uint8_t>>(std::move(data));
}

Result<std::string> SDCard::FileHandle::read_line() {
    if (!is_open_) {
        return Result<std::string>(ErrorCode::PERMISSION_DENIED, "文件未打开");
    }
    
    std::string line;
    char ch;
    UINT bytes_read;

    while (true) {
        FRESULT fr = f_read(&file_, &ch, 1, &bytes_read);
        if (fr != FR_OK) {
            return Result<std::string>(fresult_to_error_code(fr), 
                "读取文件失败: " + std::to_string(static_cast<int>(fr)));
        }

        if (bytes_read == 0) {
            // 到达文件末尾
            break;
        }

        if (ch == '\n') {
            // 遇到换行符，返回这一行
            break;
        }

        if (ch != '\r') {
            // 忽略回车符，添加其他字符
            line += ch;
        }
    }

    return Result<std::string>(std::move(line));
}

Result<size_t> SDCard::FileHandle::write(const std::vector<uint8_t>& data) {
    if (!is_open_) {
        return Result<size_t>(ErrorCode::PERMISSION_DENIED, "文件未打开");
    }
    
    UINT bytes_written;
    FRESULT fr = f_write(&file_, data.data(), data.size(), &bytes_written);
    
    if (fr != FR_OK) {
        return Result<size_t>(fresult_to_error_code(fr), "写入文件失败");
    }
    
    return Result<size_t>(bytes_written);
}

Result<size_t> SDCard::FileHandle::write(const std::string& text) {
    std::vector<uint8_t> data(text.begin(), text.end());
    return write(data);
}

Result<void> SDCard::FileHandle::seek(size_t position) {
    if (!is_open_) {
        return Result<void>(ErrorCode::PERMISSION_DENIED, "文件未打开");
    }
    
    FRESULT fr = f_lseek(&file_, position);
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr), "文件定位失败");
    }
    
    return Result<void>();
}

Result<size_t> SDCard::FileHandle::tell() const {
    if (!is_open_) {
        return Result<size_t>(ErrorCode::PERMISSION_DENIED, "文件未打开");
    }
    
    return Result<size_t>(f_tell(&file_));
}

Result<size_t> SDCard::FileHandle::size() const {
    if (!is_open_) {
        return Result<size_t>(ErrorCode::PERMISSION_DENIED, "文件未打开");
    }
    
    return Result<size_t>(f_size(&file_));
}

Result<void> SDCard::FileHandle::flush() {
    if (!is_open_) {
        return Result<void>(ErrorCode::PERMISSION_DENIED, "文件未打开");
    }
    
    FRESULT fr = f_sync(&file_);
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr), "同步文件失败");
    }
    
    return Result<void>();
}

Result<SDCard::FileHandle> SDCard::open_file(const std::string& path, const std::string& mode) {
    FileHandle handle;
    auto result = handle.open(path, mode);
    
    if (result.is_error()) {
        return Result<FileHandle>(result.error_code(), result.error_message());
    }
    
    return Result<FileHandle>(std::move(handle));
}

// === Utility Methods ===

Result<void> SDCard::sync() {
    if (!is_mounted_) {
        return Result<void>(ErrorCode::MOUNT_FAILED, "SD卡未挂载");
    }
    
    FRESULT fr = f_sync(nullptr);  // 同步所有文件
    if (fr != FR_OK) {
        return Result<void>(fresult_to_error_code(fr), "同步失败");
    }
    
    return Result<void>();
}

Result<void> SDCard::format(const std::string& fs_type) {
    if (!is_mounted_) {
        return Result<void>(ErrorCode::MOUNT_FAILED, "SD卡未挂载");
    }

    BYTE work[FF_MAX_SS]; // Work area for f_mkfs

    // 设置MKFS参数
    MKFS_PARM opt;
    std::memset(&opt, 0, sizeof(opt));  // 清零所有字段

    // 根据文件系统类型选择格式化选项
    if (fs_type == "FAT32") {
        opt.fmt = FS_FAT32;
    } else if (fs_type == "FAT16") {
        opt.fmt = FS_FAT16;
    } else if (fs_type == "exFAT") {
        opt.fmt = FS_EXFAT;
    } else {
        opt.fmt = FS_FAT32;  // 默认使用FAT32
    }

    opt.n_fat = 1;      // FAT表数量
    opt.align = 0;      // 自动对齐
    opt.n_root = 512;   // 根目录项数
    opt.au_size = 0;    // 自动选择簇大小

    // 调用f_mkfs格式化
    FRESULT fr = f_mkfs("", &opt, work, sizeof(work));
    
    if (fr != FR_OK) {
        return Result<void>(ErrorCode::FATFS_ERROR, 
            std::string("格式化失败: ") + std::to_string(static_cast<int>(fr)));
    }

    return Result<void>();
}

} // namespace MicroSD 