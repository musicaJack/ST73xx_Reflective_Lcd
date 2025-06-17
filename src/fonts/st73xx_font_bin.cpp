#include "st73xx_font_bin.hpp"
#include <cstdio>
#include <cstring>
#include <vector>

namespace fontbin {

struct FontBin {
    uint16_t version;
    uint16_t count;
    std::vector<uint8_t> data; // 全部点阵数据
};

static FontBin font16;
static FontBin font24;
static bool loaded16 = false;
static bool loaded24 = false;

bool font_load(const char* filename, int font_size) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return false;
    uint16_t version = 0, count = 0;
    fread(&version, sizeof(uint16_t), 1, fp);
    fread(&count, sizeof(uint16_t), 1, fp);
    size_t char_size = (font_size == 16) ? 32 : 72;
    size_t total_size = char_size * 94;
    std::vector<uint8_t> buf(total_size);
    size_t n = fread(buf.data(), 1, total_size, fp);
    fclose(fp);
    if (n != total_size) return false;
    if (font_size == 16) {
        font16.version = version;
        font16.count = count;
        font16.data = std::move(buf);
        loaded16 = true;
    } else {
        font24.version = version;
        font24.count = count;
        font24.data = std::move(buf);
        loaded24 = true;
    }
    return true;
}

bool font_get_bitmap(char ascii, int font_size, uint8_t* buf) {
    if (ascii < 0x21 || ascii > 0x7E) return false;
    int idx = ascii - 0x21;
    size_t char_size = (font_size == 16) ? 32 : 72;
    if (font_size == 16 && loaded16) {
        memcpy(buf, &font16.data[idx * char_size], char_size);
        return true;
    } else if (font_size == 24 && loaded24) {
        memcpy(buf, &font24.data[idx * char_size], char_size);
        return true;
    }
    return false;
}

void font_unload(int font_size) {
    if (font_size == 16) {
        font16.data.clear();
        loaded16 = false;
    } else {
        font24.data.clear();
        loaded24 = false;
    }
}

} // namespace fontbin 