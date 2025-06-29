/**
 * @file rw_sd.hpp
 * @brief 可读写SD卡类 - 生产级API
 * @version 3.0.0
 */

#pragma once

#include "storage_device.hpp"
#include "spi_config.hpp"
#include "ff.h"
#include <memory>
#include <vector>

namespace MicroSD {

/**
 * @brief 可读写SD卡类 - 生产级实现
 * 支持完整的读写操作，针对Pico内存有限的情况进行优化
 */
class RWSD : public StorageDevice {
private:
    MicroSD::SPIConfig config_;
    FATFS fs_;
    uint8_t fs_type_;
    bool is_initialized_;
    
    // RAII资源管理
    std::unique_ptr<DIR> current_dir_;
    std::string current_path_;
    
    // 私有方法
    void initialize_spi();
    void deinitialize_spi();
    Result<void> mount_filesystem();
    void unmount_filesystem();
    ErrorCode fresult_to_error_code(FRESULT fr) const;
    
public:
    /**
     * @brief 构造函数
     * @param config SPI配置，如果不提供则使用默认配置
     */
    explicit RWSD(MicroSD::SPIConfig config = MicroSD::Config::DEFAULT);
    
    /**
     * @brief 析构函数 - RAII自动资源清理
     */
    ~RWSD() override;
    
    // 禁用拷贝构造和赋值 (资源管理安全)
    RWSD(const RWSD&) = delete;
    RWSD& operator=(const RWSD&) = delete;
    
    // 支持移动语义
    RWSD(RWSD&& other) noexcept;
    RWSD& operator=(RWSD&& other) noexcept;
    
    /**
     * @brief 初始化SD卡
     */
    Result<void> initialize() override;
    
    /**
     * @brief 检查是否已初始化
     */
    bool is_initialized() const { return is_initialized_; }
    
    /**
     * @brief 获取文件系统类型
     */
    std::string get_filesystem_type() const override;
    
    /**
     * @brief 获取SD卡容量信息
     */
    Result<std::pair<size_t, size_t>> get_capacity() const override;
    
    // === 目录操作 ===
    
    /**
     * @brief 列出目录内容
     */
    Result<std::vector<FileInfo>> list_directory(const std::string& path = "") override;
    
    /**
     * @brief 递归列出目录树结构
     * @param path 起始路径，默认为根目录
     * @param max_depth 最大递归深度，默认为10
     * @return 树形结构字符串
     */
    Result<std::string> list_directory_tree(const std::string& path = "", int max_depth = 10) const;
    
    /**
     * @brief 创建目录
     */
    Result<void> create_directory(const std::string& path);
    
    /**
     * @brief 删除目录 (必须为空)
     */
    Result<void> remove_directory(const std::string& path);
    
    /**
     * @brief 检查文件是否存在
     */
    bool file_exists(const std::string& path) const override;
    
    /**
     * @brief 获取文件信息
     */
    Result<FileInfo> get_file_info(const std::string& path) const override;
    
    // === 一次性读写操作 ===
    
    /**
     * @brief 读取文件 (一次性读取，适合小文件)
     */
    Result<std::vector<uint8_t>> read_file(const std::string& path) const override;
    
    /**
     * @brief 读取文件块 (指定偏移和大小)
     */
    Result<std::vector<uint8_t>> read_file_chunk(const std::string& path, 
                                                 size_t offset, size_t size) const override;
    
    /**
     * @brief 读取文本文件
     */
    Result<std::string> read_text_file(const std::string& path) const;
    
    /**
     * @brief 写入文件 (覆盖模式)
     */
    Result<void> write_file(const std::string& path, const std::vector<uint8_t>& data);
    
    /**
     * @brief 写入文本文件 (覆盖模式)
     */
    Result<void> write_text_file(const std::string& path, const std::string& content);
    
    /**
     * @brief 追加写入文件
     */
    Result<void> append_file(const std::string& path, const std::vector<uint8_t>& data);
    
    /**
     * @brief 追加写入文本文件
     */
    Result<void> append_text_file(const std::string& path, const std::string& content);
    
    /**
     * @brief 删除文件
     */
    Result<void> delete_file(const std::string& path);
    
    /**
     * @brief 重命名文件或目录
     */
    Result<void> rename(const std::string& old_path, const std::string& new_path);
    
    /**
     * @brief 复制文件
     */
    Result<void> copy_file(const std::string& src_path, const std::string& dst_path);
    
    /**
     * @brief 同步文件系统
     */
    Result<void> sync() override;
    
    // === 流式读写文件句柄类 ===
    
    /**
     * @brief 文件句柄类 - 支持流式读写
     */
    class FileHandle {
    private:
        FIL file_;
        bool is_open_;
        std::string path_;
        std::string mode_;
        
    public:
        FileHandle() : is_open_(false) {}
        ~FileHandle() { close(); }
        
        // 禁用拷贝
        FileHandle(const FileHandle&) = delete;
        FileHandle& operator=(const FileHandle&) = delete;
        
        // 支持移动
        FileHandle(FileHandle&& other) noexcept;
        
        bool is_open() const { return is_open_; }
        const std::string& get_path() const { return path_; }
        const std::string& get_mode() const { return mode_; }
        
        // 文件操作
        Result<void> open(const std::string& path, const std::string& mode);
        void close();
        
        // 读取操作
        Result<std::vector<uint8_t>> read(size_t size);
        Result<size_t> read_text(std::string& text, size_t max_size);
        
        // 写入操作
        Result<size_t> write(const std::vector<uint8_t>& data);
        Result<size_t> write(const std::string& text);
        Result<size_t> write_line(const std::string& line);
        
        // 文件定位
        Result<void> seek(size_t position);
        Result<size_t> tell() const;
        Result<size_t> size() const;
        
        // 文件控制
        Result<void> flush();
        Result<void> truncate(size_t size);
    };
    
    /**
     * @brief 打开文件句柄
     */
    Result<FileHandle> open_file(const std::string& path, const std::string& mode);
    
    // === 高级功能 ===
    
    /**
     * @brief 格式化文件系统
     */
    Result<void> format(const std::string& volume_label = "");
    
    /**
     * @brief 获取文件系统状态
     */
    Result<std::string> get_filesystem_status() const;
    
    /**
     * @brief 检查文件系统完整性
     */
    Result<bool> check_filesystem_integrity() const;
    
    // === 工具方法 ===
    
    /**
     * @brief 获取配置信息
     */
    std::string get_config_info() const;
    
    /**
     * @brief 获取设备状态信息
     */
    std::string get_status_info() const;
    
    /**
     * @brief 获取内存使用情况
     */
    std::string get_memory_usage() const;
};

} // namespace MicroSD 