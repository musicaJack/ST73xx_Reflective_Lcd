#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "microsd/rw_sd.hpp"

namespace micro_sd_text_reader {

struct PageMarker {
    size_t file_offset;
    int wrap_line_index;
};

class PageManager {
public:
    PageManager(const std::string& file_path,
                std::function<std::unique_ptr<MicroSD::RWSD::FileHandle>(const std::string&, const char*)> file_opener,
                std::function<int(const std::string&)> get_string_width);

    bool precompute_pages();
    bool load_page_content(int page_num, std::vector<std::string>& out_lines);
    void preload_next_page(int current_page);
    bool check_file_changed();
    int total_pages() const;
    void clear_next_page_cache();
    bool is_page_cached(int page_num) const;
    void clear_page_markers();

private:
    std::string file_path_;
    std::function<std::unique_ptr<MicroSD::RWSD::FileHandle>(const std::string&, const char*)> file_opener_;
    std::function<int(const std::string&)> get_string_width_;
    std::vector<PageMarker> page_markers_;
    int total_pages_ = 0;
    size_t last_file_size_ = 0;
    std::vector<std::string> next_page_cache_;
    int cached_page_num_ = -1;
    bool pages_precomputed_ = false;
    // ... 其他分页相关成员 ...
};

} // namespace micro_sd_text_reader 