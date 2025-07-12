#pragma once
#include <memory>
#include <string>
#include "microsd/rw_sd.hpp"

namespace micro_sd_text_reader {

class MicroSDManager {
public:
    MicroSDManager();
    ~MicroSDManager();

    // 初始化SD卡
    bool initialize();
    // 检查SD卡是否就绪
    bool is_ready() const;
    // 打开文件，返回文件句柄
    std::unique_ptr<MicroSD::RWSD::FileHandle> open_file(const std::string& path, const char* mode);
    // 获取文件大小
    size_t get_file_size(const std::string& path);
    // 获取最后一次错误信息
    std::string get_last_error() const;
    // 刷新/重载SD卡
    void refresh();
    // 检查文件是否存在
    bool file_exists(const std::string& path) const;
    // 获取文件信息
    MicroSD::Result<MicroSD::FileInfo> get_file_info(const std::string& path) const;
    // 获取SD卡状态信息
    std::string get_status_info() const;
    // 获取SD卡配置信息
    std::string get_config_info() const;
    // 获取SD卡目录内容
    MicroSD::Result<std::vector<MicroSD::FileInfo>> list_directory(const std::string& path = "");

private:
    MicroSD::RWSD sd_;
    bool ready_;
    std::string last_error_;
};

} // namespace micro_sd_text_reader 