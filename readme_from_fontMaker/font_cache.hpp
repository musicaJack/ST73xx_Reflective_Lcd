#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <list>

// LRU缓存节点结构
template<typename T>
struct LRUNode {
    uint16_t code;           // 字符编码
    std::vector<uint8_t> data;  // 点阵数据
    T* prev;                 // 前驱节点
    T* next;                 // 后继节点
};

class FontCache {
private:
    static constexpr size_t CACHE_SIZE_16 = 800;
    static constexpr size_t BYTES_PER_CHAR_16 = 32;
    static constexpr size_t CACHE_SIZE_24 = 600;
    static constexpr size_t BYTES_PER_CHAR_24 = 72;
    int selected_size = 0;
    std::list<LRUNode<LRUNode<uint16_t>>> cache_list;
    std::unordered_map<uint16_t, typename std::list<LRUNode<LRUNode<uint16_t>>>::iterator> cache_map;
    bool loaded = false;
    FontCache();
    FontCache(const FontCache&) = delete;
    FontCache& operator=(const FontCache&) = delete;
    std::vector<uint8_t> load_char_from_file(const std::string& font_file, uint16_t code);
    void update_cache(uint16_t code, const std::vector<uint8_t>& bitmap);
public:
    static FontCache& get_instance();
    void set_font_size(int size);
    int get_font_size() const;
    bool load_font(const std::string& font_file);
    std::vector<uint8_t> get_char_bitmap(uint16_t code, int size);
    size_t get_memory_usage() const;
    size_t get_cache_size() const;
}; 