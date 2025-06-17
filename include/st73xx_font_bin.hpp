#pragma once
#include <cstdint>
#include <string>

namespace fontbin {

// 加载字体文件到内存
bool font_load(const char* filename, int font_size);
// 获取指定ASCII字符的点阵数据，buf需预分配32字节(16x16)或72字节(24x24)
bool font_get_bitmap(char ascii, int font_size, uint8_t* buf);
// 释放字体内存
void font_unload(int font_size);

} // namespace fontbin 