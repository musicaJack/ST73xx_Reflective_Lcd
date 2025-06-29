#include "storage_device.hpp"
#include <sstream>
#include <algorithm>

namespace MicroSD {

std::string StorageDevice::get_error_description(ErrorCode error_code) {
    switch (error_code) {
        case ErrorCode::SUCCESS:
            return "操作成功";
        case ErrorCode::INIT_FAILED:
            return "初始化失败";
        case ErrorCode::MOUNT_FAILED:
            return "挂载失败";
        case ErrorCode::FILE_NOT_FOUND:
            return "文件未找到";
        case ErrorCode::PERMISSION_DENIED:
            return "权限被拒绝";
        case ErrorCode::DISK_FULL:
            return "磁盘空间不足";
        case ErrorCode::IO_ERROR:
            return "输入输出错误";
        case ErrorCode::INVALID_PARAMETER:
            return "无效参数";
        case ErrorCode::FATFS_ERROR:
            return "文件系统错误";
        case ErrorCode::UNKNOWN_ERROR:
        default:
            return "未知错误";
    }
}

std::string StorageDevice::normalize_path(const std::string& path) {
    std::string result = path;
    
    // 替换反斜杠为正斜杠
    std::replace(result.begin(), result.end(), '\\', '/');
    
    // 移除连续的斜杠
    std::string::size_type pos = 0;
    while ((pos = result.find("//", pos)) != std::string::npos) {
        result.erase(pos, 1);
    }
    
    // 确保以斜杠开头
    if (!result.empty() && result[0] != '/') {
        result = "/" + result;
    }
    
    // 移除末尾的斜杠（除非是根目录）
    if (result.length() > 1 && result.back() == '/') {
        result.pop_back();
    }
    
    return result;
}

std::string StorageDevice::join_path(const std::string& dir, const std::string& file) {
    if (dir.empty()) {
        return file;
    }
    if (file.empty()) {
        return dir;
    }
    
    std::string result = dir;
    if (result.back() != '/') {
        result += '/';
    }
    result += file;
    
    return normalize_path(result);
}

std::pair<std::string, std::string> StorageDevice::split_path(const std::string& path) {
    std::string normalized = normalize_path(path);
    std::string::size_type pos = normalized.find_last_of('/');
    
    if (pos == std::string::npos) {
        return {"", normalized};
    }
    
    if (pos == 0) {
        return {"/", normalized.substr(1)};
    }
    
    return {normalized.substr(0, pos), normalized.substr(pos + 1)};
}

} // namespace MicroSD 