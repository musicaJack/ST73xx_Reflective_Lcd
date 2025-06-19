#include "font_cache.hpp"
#include <fstream>
#include <iostream>
#include <unordered_map>

// 构造函数
FontCache::FontCache() = default;

// 获取单例实例
FontCache& FontCache::get_instance() {
    static FontCache instance;
    return instance;
}

void FontCache::set_font_size(int size) {
    if (size != 16 && size != 24) {
        return;
    }
    if (selected_size != size) {
        cache_list.clear();
        cache_map.clear();
        loaded = false;
        selected_size = size;
    }
}

int FontCache::get_font_size() const {
    return selected_size;
}

bool FontCache::load_font(const std::string& font_file) {
    if (selected_size == 0) {
        return false;
    }
    cache_list.clear();
    cache_map.clear();
    
    // 预加载ASCII字符 (0x0021-0x007E, 94个字符)
    for (uint16_t code = 0x0021; code <= 0x007E; code++) {
        auto bitmap = load_char_from_file(font_file, code);
        if (!bitmap.empty()) {
            update_cache(code, bitmap);
        }
    }
    loaded = true;
    return true;
}

std::vector<uint8_t> FontCache::get_char_bitmap(uint16_t code, int size) {
    if (size != selected_size || !loaded) {
        return std::vector<uint8_t>();
    }
    auto it = cache_map.find(code);
    if (it != cache_map.end()) {
        cache_list.splice(cache_list.begin(), cache_list, it->second);
        return it->second->data;
    }
    std::string font_file = "output/font" + std::to_string(size) + ".bin";
    auto bitmap = load_char_from_file(font_file, code);
    if (!bitmap.empty()) {
        update_cache(code, bitmap);
    }
    return bitmap;
}

size_t FontCache::get_memory_usage() const {
    size_t bytes_per_char = (selected_size == 16) ? BYTES_PER_CHAR_16 : BYTES_PER_CHAR_24;
    return cache_list.size() * bytes_per_char;
}

size_t FontCache::get_cache_size() const {
    return cache_list.size();
}

std::vector<uint8_t> FontCache::load_char_from_file(const std::string& font_file, uint16_t code) {
    std::ifstream file(font_file, std::ios::binary);
    if (!file) {
        return std::vector<uint8_t>();
    }
    uint16_t version, char_count;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&char_count), sizeof(char_count));
    
    uint32_t offset;
    size_t bytes_per_char = (selected_size == 16) ? BYTES_PER_CHAR_16 : BYTES_PER_CHAR_24;
    
    // 计算字符在字库中的偏移
    if (code >= 0x0021 && code <= 0x007E) {
        // ASCII字符范围：0x0021-0x007E (94个字符)
        offset = sizeof(version) + sizeof(char_count) + (code - 0x0021) * bytes_per_char;
    } else if (code >= 0x4E00 && code <= 0x9FA5) {
        // 汉字范围：0x4E00-0x9FA5 (20902个字符)
        offset = sizeof(version) + sizeof(char_count) + 94 * bytes_per_char + (code - 0x4E00) * bytes_per_char;
    } else {
        // 不支持的字符范围
        return std::vector<uint8_t>();
    }
    
    file.seekg(offset);
    std::vector<uint8_t> bitmap(bytes_per_char);
    file.read(reinterpret_cast<char*>(bitmap.data()), bytes_per_char);
    if (static_cast<size_t>(file.gcount()) != bytes_per_char) {
        return std::vector<uint8_t>();
    }
    return bitmap;
}

void FontCache::update_cache(uint16_t code, const std::vector<uint8_t>& bitmap) {
    size_t max_size = (selected_size == 16) ? CACHE_SIZE_16 : CACHE_SIZE_24;
    auto it = cache_map.find(code);
    if (it != cache_map.end()) {
        cache_list.splice(cache_list.begin(), cache_list, it->second);
        return;
    }
    if (cache_list.size() >= max_size) {
        auto last = cache_list.back();
        cache_map.erase(last.code);
        cache_list.pop_back();
    }
    LRUNode<LRUNode<uint16_t>> node;
    node.code = code;
    node.data = bitmap;
    cache_list.push_front(node);
    cache_map[code] = cache_list.begin();
}

// 声明draw_pixel函数
void draw_pixel(int x, int y, bool is_on);

// 显示单个字符
void display_char(uint16_t code, int x, int y, uint16_t color) {
    FontCache& cache = FontCache::get_instance();
    auto bitmap = cache.get_char_bitmap(code, cache.get_font_size());
    if (bitmap.empty()) return;
    int size = cache.get_font_size();
    int bytes_per_row = size / 8;
    for (int row = 0; row < size; row++) {
        for (int col = 0; col < bytes_per_row; col++) {
            uint8_t byte = bitmap[row * bytes_per_row + col];
            for (int bit = 0; bit < 8; bit++) {
                if (!(byte & (0x80 >> bit))) {
                    draw_pixel(x + col * 8 + bit, y + row, true);
                }
            }
        }
    }
}

// 显示字符串
void display_string(const char* str, int x, int y, uint16_t color) {
    FontCache& cache = FontCache::get_instance();
    int size = cache.get_font_size();
    int pos_x = x;
    while (*str) {
        if ((*str & 0x80) && *(str + 1)) {
            uint16_t code = (*str << 8) | *(str + 1);
            display_char(code, pos_x, y, color);
            pos_x += size;
            str += 2;
        } else {
            display_char(*str, pos_x, y, color);
            pos_x += size / 2;
            str++;
        }
    }
}
