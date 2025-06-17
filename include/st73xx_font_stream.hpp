#pragma once
#include <cstdint>
#include <string>
// 不直接包含具体驱动头文件，改为模板

namespace fontstream {

// 按屏幕宽度自动换行，流式显示文本
// 参数：drv 显示驱动，text 文本，font_size 字体大小，color 颜色，delay_ms 每字延时(ms)
template<typename LCDDriver>
void stream_text(LCDDriver& drv, const std::string& text, int font_size, uint16_t color, int delay_ms = 0);

} // namespace fontstream 