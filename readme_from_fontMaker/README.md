# 中文字体生成器

这是一个功能完整的中文字体生成工具集，支持生成多种格式的字体文件，包括中文字体点阵数据、ASCII字体库和ST7305显示器字体库。

## 功能特点

### 中文字体生成器 (cn_font_generator.py)
- 支持完整的GBK字符集（29,047个字符）
  - ASCII字符（95个，0x20-0x7E）
  - 全角标点符号（64个，0x3000-0x303F）
  - 全角字符（240个，0xFF00-0xFFEF）
  - 汉字（28,648个，0x4E00-0x9FA5）
- 支持16x16和24x24两种字体大小
- 生成二进制字体文件，方便在嵌入式设备上使用
- 支持从TTC字体文件中提取字体
- 内置调试功能，可生成每个字符的PNG图像
- 支持中英文混合显示

### ASCII字体生成器 (main.py)
- 从TTF字体文件生成8x16 ASCII字体库
- 支持ST7305显示器字体格式
- 生成C++头文件和源文件
- 支持IBM VGA 8x16字体数据转换

### 字体验证工具 (font_verifier)
- 验证烧录到Pico W Flash中的字体文件
- 实时监控Flash内存状态
- 支持USB和UART调试输出
- 显示字符点阵数据

## 项目结构

```
font_maker/
├── cn_font_generator.py      # 中文字体生成器主程序
├── main.py                   # ASCII字体生成器
├── test_char_coverage.py     # 字符覆盖测试工具
├── font_verifer.cpp          # 字体验证工具源码
├── font_cache.cpp            # 字体缓存实现
├── font_cache.hpp            # 字体缓存头文件
├── test_cn_font_input.cpp    # 字体测试程序
├── CMakeLists.txt            # CMake构建配置
├── requirements.txt          # Python依赖
├── character_mapping.txt     # 字符映射表
├── IBM_VGA_8x16.h           # IBM VGA字体数据
├── BLADE3D_PCI__8x16.h      # BLADE3D字体数据
├── pico_sdk_import.cmake    # Pico SDK导入配置
├── ttc/                     # TTC字体文件目录
│   └── SIMSUN.TTC          # 宋体字体文件
├── ttf/                     # TTF字体文件目录
├── output/                  # 输出文件目录
│   ├── font16.bin          # 16x16中文字体文件
│   ├── font24.bin          # 24x24中文字体文件
│   ├── st73xx_font.cpp     # ST7305字体源文件
│   └── st73xx_font.hpp     # ST7305字体头文件
├── debug/                   # 调试输出目录
│   ├── debug_16/           # 16x16字符PNG图像
│   └── debug_24/           # 24x24字符PNG图像
└── build/                   # 构建输出目录
    ├── font_verifier.uf2   # Pico W固件
    └── font_verifier.bin   # 二进制固件
```

## 生成的文件

### 中文字体文件
- `output/font16.bin`：16x16字体文件（约666KB）
- `output/font24.bin`：24x24字体文件（约1.5MB）

### ST7305字体文件
- `output/st73xx_font.hpp`：ST7305字体头文件
- `output/st73xx_font.cpp`：ST7305字体源文件

### 验证工具
- `build/font_verifier.uf2`：Pico W字体验证固件
- `build/font_verifier.bin`：二进制验证工具

## 二进制文件格式

### 中文字体文件格式 (font16.bin/font24.bin)

```
+----------------+
|    文件头      |
|----------------|
| 版本号 (2字节) |
| 字符数 (2字节) |
+----------------+
|    字体数据    |
|----------------|
| 字符1点阵数据  |
| 字符2点阵数据  |
|      ...       |
+----------------+
```

#### 详细格式说明

**文件头（4字节）**
- **字节0-1**: 版本号（uint16_t，小端序，当前为1）
- **字节2-3**: 字符总数（uint16_t，小端序，29,047个）

**字符数据**
- **ASCII字符**: 95个字符（0x20-0x7E，包含空格和所有可打印字符）
- **全角标点符号**: 64个中文标点（0x3000-0x303F）
- **全角字符**: 240个全角英文、数字、符号（0xFF00-0xFFEF）
- **中文字符**: 28,648个汉字（0x4E00-0x9FA5）
- **每个字符**: 32字节（16行×2字节/行）或72字节（24行×3字节/行）
- **数据格式**: 每行2-3字节，高位在前，1表示像素点亮，0表示像素点灭

### ST7305字体格式

```cpp
namespace font {
    constexpr int FONT_WIDTH = 8;
    constexpr int FONT_HEIGHT = 16;
    constexpr int FONT_SIZE = 4096;  // 16 * 256 characters
    
    extern const uint8_t ST7305_FONT[FONT_SIZE];
    
    inline const uint8_t* get_char_data(char c) {
        return &ST7305_FONT[static_cast<unsigned char>(c) * FONT_HEIGHT];
    }
}
```

## 安装和依赖

### Python依赖
```bash
pip install -r requirements.txt
```

依赖包：
- Pillow>=9.0.0
- numpy>=1.20.0
- fontTools>=4.0.0

### 系统要求
- Python 3.7+
- CMake 3.13+（用于编译验证工具）
- Pico SDK（用于编译Pico W固件）
- ARM GCC工具链（用于交叉编译）

## 使用方法

### 1. 生成中文字体文件

```bash
# 生成16x16和24x24中文字体文件
python cn_font_generator.py
```

这将生成：
- `output/font16.bin`：16x16字体文件
- `output/font24.bin`：24x24字体文件
- `debug/debug_16/`：16x16字符PNG调试图像
- `debug/debug_24/`：24x24字符PNG调试图像

### 2. 生成ST7305字体文件

```bash
# 生成ST7305显示器字体文件
python main.py
```

这将生成：
- `output/st73xx_font.hpp`：字体头文件
- `output/st73xx_font.cpp`：字体源文件

### 3. 测试字符覆盖

```bash
# 测试字符覆盖范围
python test_char_coverage.py
```

### 4. 编译字体验证工具

#### Windows (MinGW)
```bash
# 编译验证工具
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build . --config Release
```

#### Linux/macOS
```bash
# 编译验证工具
mkdir build
cd build
cmake ..
make
```

### 5. 烧录验证工具到Pico W

```bash
# 烧录固件
picotool load -v -t bin build/font_verifier.bin -o 0x10000000

# 或者直接复制UF2文件到Pico W虚拟磁盘
# 将build/font_verifier.uf2复制到Pico W的虚拟磁盘
```

### 6. 烧录字体文件到Pico Flash

```bash
# 烧录16x16字体文件到Flash
picotool load -v -t bin output/font16.bin -o 0x10100000
```

## 字体验证工具使用

### 功能特点
- 验证字体文件头（版本号和字符数量）
- 显示字符点阵数据（ASCII和中文字符）
- 实时监控Flash内存状态
- 支持USB和UART调试输出

### 调试输出示例
```
[INFO] 字体验证工具启动
[INFO] Flash基址: 0x10100000
[DEBUG] 检查内存访问...
[DEBUG] 前4字节: 01 00 04 52
[INFO] 字体文件头验证:
  版本号: 1
  字符总数: 29047
[SUCCESS] 字体文件头验证通过
[INFO] 测试字符显示:
[DEBUG] ASCII字符 0x41 ('A') 偏移: 32
字符 0x0041 点阵数据:
  00: ................ (0x0000)
  01: ................ (0x0000)
  02: .......#........ (0x0100)
  ...
[INFO] 字体验证完成，进入循环模式
```

### 调试说明
- **串口配置**: 波特率115200，使用UART0 (GPIO0/1)
- **输出格式**: 同时支持USB和UART输出
- **字符验证**: 自动验证字体文件头和字符数据完整性
- **实时监控**: 每5秒输出运行状态，每50秒检查内存状态

## API文档

### 中文字体API

#### 初始化
```cpp
// 初始化字体缓存
void init_font_cache();
```

#### 字符显示
```cpp
// 显示单个字符
void display_char(uint16_t gbk_code, int x, int y, uint16_t color);

// 显示字符串
void display_string(const char* str, int x, int y, uint16_t color);
```

#### 编码转换
```cpp
// GBK转Unicode
uint16_t gbk_to_unicode(uint16_t gbk_code);

// Unicode转GBK
uint16_t unicode_to_gbk(uint16_t unicode);
```

#### 字体数据加载
```cpp
// 加载字体数据
bool load_font_data(uint16_t gbk_code, uint8_t* bitmap);
```

### ST7305字体API

```cpp
#include "st73xx_font.hpp"

// 获取字符数据
const uint8_t* data = font::get_char_data('A');

// 打印字符点阵（调试用）
for (int i = 0; i < font::FONT_HEIGHT; ++i) {
    for (int b = 7; b >= 0; --b)
        putchar((data[i] & (1 << b)) ? '#' : '.');
    putchar('\n');
}
```

## 使用示例

### 1. 中文字体基本显示

```cpp
#include "font_cache.h"

void setup() {
    // 初始化显示设备
    init_display();
    
    // 初始化字体缓存
    init_font_cache();
    
    // 显示中文字符串
    display_string("Hello 世界！", 0, 0, 0xFFFF);
}
```

### 2. 混合显示

```cpp
// 显示中英文混合文本
display_string("温度: 25.5℃", 0, 0, 0xFFFF);
display_string("湿度: 65%", 0, 16, 0xFFFF);
```

### 3. ST7305字体使用

```cpp
#include "st73xx_font.hpp"

void draw_char_st7305(char c, int x, int y) {
    const uint8_t* data = font::get_char_data(c);
    
    for (int row = 0; row < font::FONT_HEIGHT; row++) {
        for (int col = 0; col < font::FONT_WIDTH; col++) {
            if (data[row] & (0x80 >> col)) {
                // 绘制像素点
                draw_pixel(x + col, y + row, 1);
            }
        }
    }
}
```

## 文件大小信息

### 中文字体文件
- **font16.bin**: 约666KB
- **font24.bin**: 约1.5MB
- **字符总数**: 29,047个
- **每个字符**: 32字节（16x16）或72字节（24x24）

### ST7305字体文件
- **st73xx_font.cpp**: 约27KB
- **st73xx_font.hpp**: 约1.7KB
- **字符总数**: 256个（ASCII 0-255）
- **每个字符**: 16字节（8x16）

## 兼容性

- 支持Pico/PicoW系列MCU
- 支持所有支持16位内存访问的ARM Cortex-M0+设备
- 字库格式为小端序，适用于大多数现代处理器
- ST7305字体兼容大多数8x16点阵显示器

## 调试功能

- 生成过程中会在 `debug_16` 和 `debug_24` 目录下创建每个字符的PNG图像
- 图像文件名格式为 `XXXX.png`，其中XXXX是字符的Unicode编码
- 字体验证工具提供实时调试输出
- 支持字符覆盖测试

## 注意事项

1. **内存使用**
   - 字体缓存默认大小为32个字符
   - 可以根据需要调整 `FONT_CACHE_SIZE`

2. **显示效果**
   - ASCII字符宽度为8像素
   - 中文字符宽度为16像素
   - 字符高度为16像素（16x16字体）或24像素（24x24字体）

3. **性能优化**
   - 使用字体缓存减少SD卡读取
   - 可以调整缓存大小平衡内存使用和性能
   - 直接内存访问比文件读取更快

4. **字库文件使用**
   - **内存对齐**: 确保字库文件烧录到对齐的地址（如0x10100000）
   - **字符编码**: 支持完整的Unicode字符范围
   - **数据格式**: 点阵数据为16×16或24×24像素
   - **错误处理**: 对于不支持的字符，建议显示空格或默认字符

## 许可证

MIT License 