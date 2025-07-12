/**
 * @file storage_device.hpp
 * @brief 存储设备抽象基类
 * @version 2.0.0
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace MicroSD {

/**
 * @brief 错误代码枚举
 */
enum class ErrorCode {
    SUCCESS = 0,
    INIT_FAILED,
    MOUNT_FAILED,
    FILE_NOT_FOUND,
    PERMISSION_DENIED,
    DISK_FULL,
    IO_ERROR,
    INVALID_PARAMETER,
    FATFS_ERROR,
    UNKNOWN_ERROR
};

/**
 * @brief 文件信息结构体
 */
struct FileInfo {
    std::string name;           // 文件名
    std::string full_path;      // 完整路径
    size_t size;               // 文件大小 (字节)
    bool is_directory;         // 是否为目录
    uint8_t attributes;        // 文件属性
    
    // C++17结构化绑定支持
    auto tie() const { return std::tie(name, full_path, size, is_directory, attributes); }
};

/**
 * @brief 结果模板类 - 现代C++错误处理方式
 */
template<typename T>
class Result {
private:
    std::optional<T> value_;
    ErrorCode error_code_;
    std::string error_message_;

public:
    Result(T&& value) : value_(std::move(value)), error_code_(ErrorCode::SUCCESS) {}
    Result(const T& value) : value_(value), error_code_(ErrorCode::SUCCESS) {}
    Result(ErrorCode error, std::string message = "")
        : error_code_(error), error_message_(std::move(message)) {}

    bool is_ok() const { return error_code_ == ErrorCode::SUCCESS; }
    bool is_error() const { return !is_ok(); }
    ErrorCode error_code() const { return error_code_; }
    const std::string& error_message() const { return error_message_; }
    const T& operator*() const { return value_.value(); }
    T& operator*() { return value_.value(); }
    const T* operator->() const { return &value_.value(); }
    T* operator->() { return &value_.value(); }
};

// void类型的完全特化
template<>
class Result<void> {
private:
    ErrorCode error_code_;
    std::string error_message_;

public:
    Result() : error_code_(ErrorCode::SUCCESS) {}
    Result(ErrorCode error, std::string message = "")
        : error_code_(error), error_message_(std::move(message)) {}

    bool is_ok() const { return error_code_ == ErrorCode::SUCCESS; }
    bool is_error() const { return !is_ok(); }
    ErrorCode error_code() const { return error_code_; }
    const std::string& error_message() const { return error_message_; }
};

/**
 * @brief 存储设备抽象基类
 */
class StorageDevice {
protected:
    bool is_mounted_;
    std::string device_name_;
    
public:
    StorageDevice(const std::string& name = "Unknown") 
        : is_mounted_(false), device_name_(name) {}
    
    virtual ~StorageDevice() = default;
    
    // 禁用拷贝构造和赋值 (资源管理安全)
    StorageDevice(const StorageDevice&) = delete;
    StorageDevice& operator=(const StorageDevice&) = delete;
    
    // 支持移动语义
    StorageDevice(StorageDevice&& other) noexcept;
    StorageDevice& operator=(StorageDevice&& other) noexcept;
    
    /**
     * @brief 初始化设备
     */
    virtual Result<void> initialize() = 0;
    
    /**
     * @brief 检查设备是否已挂载
     */
    virtual bool is_mounted() const { return is_mounted_; }
    
    /**
     * @brief 获取设备名称
     */
    virtual std::string get_device_name() const { return device_name_; }
    
    /**
     * @brief 获取文件系统类型
     */
    virtual std::string get_filesystem_type() const = 0;
    
    /**
     * @brief 获取设备容量信息
     */
    virtual Result<std::pair<size_t, size_t>> get_capacity() const = 0;
    
    // === 目录操作 ===
    
    /**
     * @brief 列出目录内容
     */
    virtual Result<std::vector<FileInfo>> list_directory(const std::string& path = "") = 0;
    
    /**
     * @brief 检查文件是否存在
     */
    virtual bool file_exists(const std::string& path) const = 0;
    
    /**
     * @brief 获取文件信息
     */
    virtual Result<FileInfo> get_file_info(const std::string& path) const = 0;
    
    // === 文件操作 ===
    
    /**
     * @brief 读取文件
     */
    virtual Result<std::vector<uint8_t>> read_file(const std::string& path) const = 0;
    
    /**
     * @brief 读取文件块
     */
    virtual Result<std::vector<uint8_t>> read_file_chunk(const std::string& path, 
                                                        size_t offset, size_t size) const = 0;
    
    /**
     * @brief 同步文件系统
     */
    virtual Result<void> sync() = 0;
    
    /**
     * @brief 获取错误描述
     */
    static std::string get_error_description(ErrorCode error_code);
    
    /**
     * @brief 路径工具函数
     */
    static std::string normalize_path(const std::string& path);
    static std::string join_path(const std::string& dir, const std::string& file);
    static std::pair<std::string, std::string> split_path(const std::string& path);
};

} // namespace MicroSD 