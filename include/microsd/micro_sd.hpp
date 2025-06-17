/**
 * @file micro_sd.hpp
 * @brief Modern C++ MicroSD Card Library for Raspberry Pi Pico
 * @author Refactored from RPi_Pico_WAV_Player project
 * @version 1.0.0
 * 
 * Based on your wiring:
 * GPIO10(SCK) -> SPI clock signal
 * GPIO11(MISO) -> Master In Slave Out
 * GPIO12(MOSI) -> Master Out Slave In  
 * GPIO13(CS) -> Chip Select
 * VCC -> 3.3V
 * GND -> GND
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <optional>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ff.h"
#include "spi_config.hpp"

namespace MicroSD {

/**
 * @brief File information structure
 */
struct FileInfo {
    std::string name;           // File name
    std::string full_path;      // Full path
    size_t size;               // File size (bytes)
    bool is_directory;         // Is directory flag
    uint8_t attributes;        // File attributes
    
    // C++17 structured binding support
    auto tie() const { return std::tie(name, full_path, size, is_directory, attributes); }
};

/**
 * @brief Error code enumeration
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
 * @brief Result template class - Modern C++ error handling approach
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

// Complete specialization for Result<void>
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
 * @brief MicroSD card manager class
 */
class SDCard {
private:
    SPIConfig config_;
    FATFS fs_;
    bool is_mounted_;
    uint8_t fs_type_;
    
    // RAII resource management
    std::unique_ptr<DIR> current_dir_;
    std::string current_path_;
    
    // Private methods
    void initialize_spi();
    void deinitialize_spi();
    Result<void> mount_filesystem();
    void unmount_filesystem();
    
public:
    /**
     * @brief Constructor
     * @param config SPI configuration, uses default if not provided
     */
    explicit SDCard(SPIConfig config = SPIConfig{});
    
    /**
     * @brief Destructor - RAII automatic resource cleanup
     */
    ~SDCard();
    
    // Disable copy constructor and assignment (resource management safety)
    SDCard(const SDCard&) = delete;
    SDCard& operator=(const SDCard&) = delete;
    
    // Support move semantics
    SDCard(SDCard&& other) noexcept;
    SDCard& operator=(SDCard&& other) noexcept;
    
    /**
     * @brief Initialize SD card
     * @return Initialization result
     */
    Result<void> initialize();
    
    /**
     * @brief Check if SD card is mounted
     */
    bool is_mounted() const { return is_mounted_; }
    
    /**
     * @brief Get filesystem type
     */
    std::string get_filesystem_type() const;
    
    /**
     * @brief Get SD card capacity information
     * @return Total and available capacity (bytes)
     */
    Result<std::pair<size_t, size_t>> get_capacity() const;
    
    // === Directory Operations ===
    
    /**
     * @brief Open directory
     * @param path Directory path
     * @return Operation result
     */
    Result<void> open_directory(const std::string& path);
    
    /**
     * @brief Get current directory path
     */
    std::string get_current_directory() const { return current_path_; }
    
    /**
     * @brief List directory contents
     * @param path Directory path, empty string for current directory
     * @return File list
     */
    Result<std::vector<FileInfo>> list_directory(const std::string& path = "");
    
    /**
     * @brief Create directory
     * @param path Directory path
     * @return Operation result
     */
    Result<void> create_directory(const std::string& path);
    
    /**
     * @brief Remove directory (must be empty)
     * @param path Directory path
     * @return Operation result
     */
    Result<void> remove_directory(const std::string& path);
    
    // === File Operations ===
    
    /**
     * @brief Check if file exists
     * @param path File path
     * @return Whether file exists
     */
    bool file_exists(const std::string& path) const;
    
    /**
     * @brief Get file information
     * @param path File path
     * @return File information
     */
    Result<FileInfo> get_file_info(const std::string& path) const;
    
    /**
     * @brief Read entire file into memory
     * @param path File path
     * @return File content
     */
    Result<std::vector<uint8_t>> read_file(const std::string& path) const;
    
    /**
     * @brief Read part of a file
     * @param path File path
     * @param offset Offset
     * @param size Size to read
     * @return File content
     */
    Result<std::vector<uint8_t>> read_file_chunk(const std::string& path, 
                                                  size_t offset, size_t size) const;
    
    /**
     * @brief Write data to file
     * @param path File path
     * @param data Data to write
     * @param append Whether to append
     * @return Operation result
     */
    Result<void> write_file(const std::string& path, 
                           const std::vector<uint8_t>& data, 
                           bool append = false);
    
    /**
     * @brief Write string to file
     * @param path File path
     * @param content String content
     * @param append Whether to append
     * @return Operation result
     */
    Result<void> write_text_file(const std::string& path, 
                                const std::string& content,
                                bool append = false);
    
    /**
     * @brief Delete file
     * @param path File path
     * @return Operation result
     */
    Result<void> delete_file(const std::string& path);
    
    /**
     * @brief Rename file or directory
     * @param old_path Old path
     * @param new_path New path
     * @return Operation result
     */
    Result<void> rename(const std::string& old_path, const std::string& new_path);
    
    /**
     * @brief Copy file
     * @param src_path Source path
     * @param dst_path Destination path
     * @return Operation result
     */
    Result<void> copy_file(const std::string& src_path, const std::string& dst_path);
    
    // === Stream File Operations ===
    
    /**
     * @brief File handle class for stream operations
     */
    class FileHandle {
    private:
        FIL file_;
        bool is_open_;
        std::string path_;
        
    public:
        FileHandle() : is_open_(false) {}
        ~FileHandle() { close(); }
        
        // Disable copy operations
        FileHandle(const FileHandle&) = delete;
        FileHandle& operator=(const FileHandle&) = delete;
        
        // Enable move operations
        FileHandle(FileHandle&& other) noexcept;
        FileHandle& operator=(FileHandle&& other) noexcept;
        
        bool is_open() const { return is_open_; }
        const std::string& path() const { return path_; }
        
        Result<void> open(const std::string& path, const std::string& mode);
        void close();
        
        Result<std::vector<uint8_t>> read(size_t size);
        Result<std::string> read_line();
        Result<size_t> write(const std::vector<uint8_t>& data);
        Result<size_t> write(const std::string& text);
        
        Result<void> seek(size_t position);
        Result<size_t> tell() const;
        Result<size_t> size() const;
        Result<void> flush();
        
        friend class SDCard;
    };
    
    /**
     * @brief Open file for stream operations
     * @param path File path
     * @param mode Open mode ("r", "w", "a", "r+", "w+", "a+")
     * @return File handle
     */
    Result<FileHandle> open_file(const std::string& path, const std::string& mode);
    
    // === Utility Methods ===
    
    /**
     * @brief Sync all file system changes to disk
     * @return Operation result
     */
    Result<void> sync();
    
    /**
     * @brief Format SD card
     * @param filesystem_type Filesystem type ("FAT32", "FAT16", "exFAT")
     * @return Operation result
     */
    Result<void> format(const std::string& filesystem_type = "FAT32");
    
    /**
     * @brief Get error description
     * @param error_code Error code
     * @return Error description string
     */
    static std::string get_error_description(ErrorCode error_code);
    
    // Path utility functions
    static std::string normalize_path(const std::string& path);
    static std::string join_path(const std::string& dir, const std::string& file);
    static std::pair<std::string, std::string> split_path(const std::string& path);
};

// Helper function to convert FATFS result to error code
ErrorCode fresult_to_error_code(FRESULT fr);

} // namespace MicroSD 