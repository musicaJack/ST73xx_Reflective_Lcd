#include "micro_sd_text_reader/PageManager.hpp"
#include "micro_sd_text_reader/TextWrapper.hpp"

namespace micro_sd_text_reader {

PageManager::PageManager(const std::string& file_path,
                         std::function<std::unique_ptr<MicroSD::RWSD::FileHandle>(const std::string&, const char*)> file_opener,
                         std::function<int(const std::string&)> get_string_width)
    : file_path_(file_path), file_opener_(file_opener), get_string_width_(get_string_width) {}

bool PageManager::precompute_pages() {
    printf("[INIT] 开始预计算分页锚点...\n");
    page_markers_.clear();
    auto file_handle_ptr = file_opener_(file_path_, "r");
    if (!file_handle_ptr) {
        printf("[ERROR] 预计算时打开文件失败\n");
        return false;
    }
    // 这里假设 file_handle_ptr 支持 read(size_t) 方法，需主程序适配
    auto& handle = *file_handle_ptr;

    // 这些参数建议由主程序传入或在PageManager构造时设定
    constexpr int LCD_WIDTH = 300;
    constexpr int LCD_HEIGHT = 400;
    constexpr int SCREEN_MARGIN = 20;
    constexpr int TOP_MARGIN = SCREEN_MARGIN;
    constexpr int BOTTOM_MARGIN = SCREEN_MARGIN;
    constexpr int TITLE_CONTENT_SPACING = 12;
    constexpr int CONTENT_FOOTER_SPACING = 20;
    constexpr int DISPLAY_WIDTH = (LCD_WIDTH - 2 * SCREEN_MARGIN);
    constexpr int PARAGRAPH_SPACING = 8;
    constexpr int LINE_HEIGHT = 22;

    int content_start_y = TOP_MARGIN + 16 + TITLE_CONTENT_SPACING;
    int content_end_y = LCD_HEIGHT - BOTTOM_MARGIN - CONTENT_FOOTER_SPACING;
    int max_page_height = content_end_y - content_start_y;
    printf("[INFO] 每页最大高度: %d 像素\n", max_page_height);

    size_t file_offset = 0;
    int current_height = 0;
    std::string accumulated_text;
    bool file_end = false;
    page_markers_.push_back({0, 0}); // 第一页从文件头第0个wrap行开始
    int page_count = 1;
    while (!file_end) {
        accumulated_text.clear();
        size_t line_start_offset = file_offset;
        // 读取一行
        while (true) {
            auto read_res = handle.read(1);
            if (!read_res.is_ok() || (*read_res).empty()) {
                file_end = true;
                break;
            }
            char ch = (*read_res)[0];
            file_offset++;
            if (ch == '\n') break;
            accumulated_text += ch;
        }
        if (accumulated_text.empty() && file_end) break;
        auto wrapped_lines = TextWrapper::wrap_text_lines(accumulated_text, DISPLAY_WIDTH, get_string_width_);
        for (int i = 0; i < wrapped_lines.size(); ++i) {
            int line_height = wrapped_lines[i].empty() ? PARAGRAPH_SPACING : LINE_HEIGHT;
            if (current_height + line_height > max_page_height) {
                // 记录新页锚点：本行起始偏移+第i个wrap行
                page_markers_.push_back({line_start_offset, i});
                page_count++;
                printf("[INFO] 第 %d 页锚点: 偏移=%zu, wrap_idx=%d\n", page_count, line_start_offset, i);
                current_height = 0;
            }
            current_height += line_height;
        }
    }
    total_pages_ = static_cast<int>(page_markers_.size());
    pages_precomputed_ = true;
    printf("[SUCCESS] 分页锚点预计算完成，共 %d 页\n", total_pages_);
    return true;
}

bool PageManager::load_page_content(int page_num, std::vector<std::string>& out_lines) {
    printf("[加载页面] 正在加载第 %d 页内容...\n", page_num + 1);
    if (page_num < 0 || page_num >= (int)page_markers_.size()) {
        printf("[ERROR] 页面号超出范围: %d (总页数: %zu)\n", page_num, page_markers_.size());
        return false;
    }
    out_lines.clear();
    auto file_handle_ptr = file_opener_(file_path_, "r");
    if (!file_handle_ptr) {
        printf("[ERROR] 打开文件失败\n");
        return false;
    }
    auto& handle = *file_handle_ptr;
    size_t start_offset = page_markers_[page_num].file_offset;
    int wrap_start = page_markers_[page_num].wrap_line_index;
    auto seek_result = handle.seek(start_offset);
    if (!seek_result.is_ok()) {
        printf("[ERROR] 定位到页面开始位置失败\n");
        return false;
    }
    constexpr int LCD_WIDTH = 300;
    constexpr int LCD_HEIGHT = 400;
    constexpr int SCREEN_MARGIN = 20;
    constexpr int TOP_MARGIN = SCREEN_MARGIN;
    constexpr int BOTTOM_MARGIN = SCREEN_MARGIN;
    constexpr int TITLE_CONTENT_SPACING = 12;
    constexpr int CONTENT_FOOTER_SPACING = 20;
    constexpr int DISPLAY_WIDTH = (LCD_WIDTH - 2 * SCREEN_MARGIN);
    constexpr int PARAGRAPH_SPACING = 8;
    constexpr int LINE_HEIGHT = 22;
    int content_start_y = TOP_MARGIN + 16 + TITLE_CONTENT_SPACING;
    int content_end_y = LCD_HEIGHT - BOTTOM_MARGIN - CONTENT_FOOTER_SPACING;
    int max_page_height = content_end_y - content_start_y;
    int current_height = 0;
    std::string accumulated_text;
    bool file_end = false;
    bool first_line = true;
    while (current_height < max_page_height && !file_end) {
        accumulated_text.clear();
        // 读取一行
        while (true) {
            auto read_res = handle.read(1);
            if (!read_res.is_ok() || (*read_res).empty()) {
                file_end = true;
                break;
            }
            char ch = (*read_res)[0];
            if (ch == '\n') break;
            accumulated_text += ch;
        }
        if (accumulated_text.empty() && file_end) break;
        auto wrapped_lines = TextWrapper::wrap_text_lines(accumulated_text, DISPLAY_WIDTH, get_string_width_);
        int wrap_begin = 0;
        if (first_line) {
            wrap_begin = wrap_start;
            first_line = false;
        }
        for (int i = wrap_begin; i < wrapped_lines.size(); ++i) {
            int line_height = wrapped_lines[i].empty() ? PARAGRAPH_SPACING : LINE_HEIGHT;
            if (current_height + line_height > max_page_height) {
                printf("[SUCCESS] 第 %d 页加载完成，包含 %zu 行\n", page_num + 1, out_lines.size());
                return true;
            }
            out_lines.push_back(wrapped_lines[i]);
            current_height += line_height;
        }
    }
    printf("[SUCCESS] 第 %d 页加载完成，包含 %zu 行\n", page_num + 1, out_lines.size());
    return true;
}

void PageManager::preload_next_page(int current_page) {
    int next_page = current_page + 1;
    if (next_page >= total_pages_) {
        next_page_cache_.clear();
        cached_page_num_ = -1;
        return;
    }
    if (cached_page_num_ == next_page && !next_page_cache_.empty()) {
        return;
    }
    printf("[预加载] 正在预加载第 %d 页...\n", next_page + 1);
    std::vector<std::string> temp_content;
    const PageMarker& marker = page_markers_[next_page];
    auto file_handle_ptr = file_opener_(file_path_, "r");
    if (!file_handle_ptr) {
        return;
    }
    auto& handle = *file_handle_ptr;
    auto seek_result = handle.seek(marker.file_offset);
    if (!seek_result.is_ok()) {
        printf("[ERROR] 预加载时定位失败\n");
        return;
    }
    constexpr int LCD_WIDTH = 300;
    constexpr int LCD_HEIGHT = 400;
    constexpr int SCREEN_MARGIN = 20;
    constexpr int TOP_MARGIN = SCREEN_MARGIN;
    constexpr int BOTTOM_MARGIN = SCREEN_MARGIN;
    constexpr int TITLE_CONTENT_SPACING = 12;
    constexpr int CONTENT_FOOTER_SPACING = 20;
    constexpr int DISPLAY_WIDTH = (LCD_WIDTH - 2 * SCREEN_MARGIN);
    constexpr int PARAGRAPH_SPACING = 8;
    constexpr int LINE_HEIGHT = 22;
    int content_start_y = TOP_MARGIN + 16 + TITLE_CONTENT_SPACING;
    int content_end_y = LCD_HEIGHT - BOTTOM_MARGIN - CONTENT_FOOTER_SPACING;
    int max_page_height = content_end_y - content_start_y;
    int current_height = 0;
    std::string accumulated_text;
    while (current_height < max_page_height) {
        accumulated_text.clear();
        bool line_read = false;
        while (true) {
            auto read_result = handle.read(1);
            if (!read_result.is_ok() || (*read_result).empty()) {
                if (!accumulated_text.empty() || line_read) {
                    goto process_preload_line;
                }
                break;
            }
            char ch = (*read_result)[0];
            line_read = true;
            if (ch == '\n' || ch == '\r') {
                if (ch == '\r') {
                    auto next_res = handle.read(1);
                    if (next_res.is_ok() && !(*next_res).empty() && (*next_res)[0] == '\n') {
                        // 跳过 \n
                    }
                }
                break;
            }
            accumulated_text += ch;
        }
        process_preload_line:
        if (accumulated_text.empty() && !line_read) break;
        std::vector<std::string> wrapped_lines = TextWrapper::wrap_text_lines(accumulated_text, DISPLAY_WIDTH, get_string_width_);
        for (const std::string& wrapped_line : wrapped_lines) {
            int line_height = wrapped_line.empty() ? PARAGRAPH_SPACING : LINE_HEIGHT;
            if (current_height + line_height > max_page_height) {
                next_page_cache_ = temp_content;
                cached_page_num_ = next_page;
                printf("[预加载] 第 %d 页预加载完成，缓存 %zu 行\n", next_page + 1, temp_content.size());
                return;
            }
            temp_content.push_back(wrapped_line);
            current_height += line_height;
        }
    }
    next_page_cache_ = temp_content;
    cached_page_num_ = next_page;
    printf("[预加载] 第 %d 页预加载完成，缓存 %zu 行\n", next_page + 1, temp_content.size());
}

void PageManager::clear_next_page_cache() {
    next_page_cache_.clear();
    cached_page_num_ = -1;
}

bool PageManager::is_page_cached(int page_num) const {
    return cached_page_num_ == page_num && !next_page_cache_.empty();
}

void PageManager::clear_page_markers() {
    page_markers_.clear();
    total_pages_ = 0;
    pages_precomputed_ = false;
}

bool PageManager::check_file_changed() {
    // 这里假设 file_opener_ 也能获取文件信息，实际应传入 get_file_info 回调
    // 这里只做简单示例，实际应完善
    // TODO: 你可以扩展接口以支持 get_file_info
    return false;
}

int PageManager::total_pages() const {
    return total_pages_;
}

} // namespace micro_sd_text_reader 