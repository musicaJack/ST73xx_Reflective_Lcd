#include "micro_sd_text_reader/MicroSDManager.hpp"
#include <cstdio>

namespace micro_sd_text_reader {

MicroSDManager::MicroSDManager() : ready_(false) {}

MicroSDManager::~MicroSDManager() {}

bool MicroSDManager::initialize() {
    auto result = sd_.initialize();
    ready_ = result.is_ok();
    if (!ready_) {
        last_error_ = "SD卡初始化失败";
    } else {
        last_error_.clear();
    }
    return ready_;
}

bool MicroSDManager::is_ready() const {
    return ready_;
}

std::unique_ptr<MicroSD::RWSD::FileHandle> MicroSDManager::open_file(const std::string& path, const char* mode) {
    if (!ready_) {
        last_error_ = "SD卡未就绪";
        return nullptr;
    }
    auto fh = sd_.open_file(path, mode);
    if (fh.is_ok()) {
        last_error_.clear();
        return std::make_unique<MicroSD::RWSD::FileHandle>(std::move(*fh));
    } else {
        last_error_ = "文件打开失败: " + path;
        return nullptr;
    }
}

size_t MicroSDManager::get_file_size(const std::string& path) {
    if (!ready_) {
        last_error_ = "SD卡未就绪";
        return 0;
    }
    auto info = sd_.get_file_info(path);
    if (info.is_ok()) {
        last_error_.clear();
        return info->size;
    } else {
        last_error_ = "获取文件大小失败: " + path;
        return 0;
    }
}

std::string MicroSDManager::get_last_error() const {
    return last_error_;
}

void MicroSDManager::refresh() {
    auto result = sd_.initialize();
    ready_ = result.is_ok();
    if (!ready_) {
        last_error_ = "SD卡刷新失败";
    } else {
        last_error_.clear();
    }
}

bool MicroSDManager::file_exists(const std::string& path) const {
    return sd_.file_exists(path);
}

MicroSD::Result<MicroSD::FileInfo> MicroSDManager::get_file_info(const std::string& path) const {
    return sd_.get_file_info(path);
}

MicroSD::Result<std::vector<MicroSD::FileInfo>> MicroSDManager::list_directory(const std::string& path) {
    return sd_.list_directory(path);
}

std::string MicroSDManager::get_status_info() const {
    return sd_.get_status_info();
}

std::string MicroSDManager::get_config_info() const {
    return sd_.get_config_info();
}

} // namespace micro_sd_text_reader 