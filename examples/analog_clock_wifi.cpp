#include "st7306_driver.hpp"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip4_addr.h"
#include "hardware/spi.h"
#include "hardware/rtc.h"
#include "hardware/clocks.h"

#include "pico_display_gfx.hpp"
#include "st73xx_font.hpp"
#include "gfx_colors.hpp"
#include "spi_config.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <time.h>

// WiFi配置
#define WIFI_SSID "YANGTOU"
#define WIFI_PASSWORD "1q2w3e4r!Q@W#E$R"

// NTP配置
#define NTP_SERVER "182.92.12.11" // 网易NTP服务器
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // 从1900年到1970年的秒数
#define BEIJING_TIMEZONE_OFFSET (8 * 3600) // UTC+8

// 调试开关
#define DEBUG_WIFI 1
#define DEBUG_NTP 1

// 调试输出宏
#if DEBUG_WIFI
#define WIFI_DEBUG(fmt, ...) printf("[WIFI_DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define WIFI_DEBUG(fmt, ...)
#endif

#if DEBUG_NTP
#define NTP_DEBUG(fmt, ...) printf("[NTP_DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define NTP_DEBUG(fmt, ...)
#endif

// 复古时钟配置
namespace vintage_clock_config {
    // 时钟尺寸（300x400显示器，竖屏模式300x400）
    constexpr int SCREEN_WIDTH = 300;
    constexpr int SCREEN_HEIGHT = 400;
    constexpr int CLOCK_CENTER_X = SCREEN_WIDTH / 2;  // 150
    constexpr int CLOCK_CENTER_Y = SCREEN_HEIGHT / 2; // 200
    
    // 时钟表盘配置
    constexpr int OUTER_RADIUS = 120;      // 外圈半径
    constexpr int INNER_RADIUS = 110;      // 内圈半径
    constexpr int HOUR_MARK_OUTER = 105;   // 小时刻度外半径
    constexpr int HOUR_MARK_INNER = 95;    // 小时刻度内半径
    constexpr int MINUTE_MARK_OUTER = 105; // 分钟刻度外半径
    constexpr int MINUTE_MARK_INNER = 100; // 分钟刻度内半径
    
    // 指针长度配置
    constexpr int HOUR_HAND_LENGTH = 60;   // 时针长度
    constexpr int MINUTE_HAND_LENGTH = 80; // 分针长度
    constexpr int SECOND_HAND_LENGTH = 95; // 秒针长度
    constexpr int CENTER_DOT_RADIUS = 4;   // 中心圆点半径
    
    // 复古颜色配置 - 使用ST7306的4级灰度
    constexpr uint8_t COLOR_BACKGROUND = st7306::ST7306Driver::COLOR_WHITE;    // 0x00 - 白色背景
    constexpr uint8_t COLOR_DIAL_DARK = st7306::ST7306Driver::COLOR_BLACK;     // 0x03 - 最深色表盘
    constexpr uint8_t COLOR_DIAL_MEDIUM = st7306::ST7306Driver::COLOR_GRAY2;   // 0x02 - 中等灰度
    constexpr uint8_t COLOR_DIAL_LIGHT = st7306::ST7306Driver::COLOR_GRAY1;    // 0x01 - 浅灰色
    constexpr uint8_t COLOR_HOUR_HAND = st7306::ST7306Driver::COLOR_BLACK;     // 0x03 - 时针黑色
    constexpr uint8_t COLOR_MINUTE_HAND = st7306::ST7306Driver::COLOR_BLACK;   // 0x03 - 分针黑色
    constexpr uint8_t COLOR_SECOND_HAND = st7306::ST7306Driver::COLOR_GRAY2;   // 0x02 - 秒针中灰
    constexpr uint8_t COLOR_TEXT = st7306::ST7306Driver::COLOR_BLACK;          // 0x03 - 文字黑色
    
    // 数字标记位置（12, 3, 6, 9）
    constexpr int NUMBER_RADIUS = 90;      // 数字距离中心的半径
}

// 时间结构体
struct ClockTime {
    int hours;
    int minutes;
    int seconds;
    int day;
    int month;
    int year;
    int weekday;  // 0=Sunday, 1=Monday, ..., 6=Saturday
};

// 星期几的简写
const char* weekday_short[7] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

// 月份简写
const char* month_short[12] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

// 全局变量
static volatile bool ntp_response_received = false;
static volatile time_t ntp_time = 0;
static bool time_synced = false;
static bool wifi_connected = false;
static int wifi_connect_attempts = 0;
static int ntp_sync_attempts = 0;

// LED控制变量
static bool led_blinking = false;
static uint32_t led_last_toggle = 0;
static bool led_state = false;

// LED控制函数
void led_set_blinking(bool enable) {
    led_blinking = enable;
    if (!enable) {
        // 停止闪烁时关闭LED
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        led_state = false;
        printf("💡 LED指示灯关闭\n");
    } else {
        printf("💡 LED指示灯开始闪烁\n");
        led_last_toggle = to_ms_since_boot(get_absolute_time());
    }
}

void led_update() {
    if (!led_blinking) return;
    
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    // 每500ms切换一次LED状态（1Hz闪烁）
    if (current_time - led_last_toggle >= 500) {
        led_state = !led_state;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state ? 1 : 0);
        led_last_toggle = current_time;
    }
}

// NTP回调函数
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    NTP_DEBUG("收到NTP回复，长度: %d", p->len);
    
    if (p->len < NTP_MSG_LEN) {
        NTP_DEBUG("NTP包长度不足: %d < %d", p->len, NTP_MSG_LEN);
        pbuf_free(p);
        return;
    }
    
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);
    
    NTP_DEBUG("NTP模式: %d, 层级: %d", mode, stratum);
    
    if (mode == 4 && stratum > 0) { // 服务器回复
        uint32_t seconds_buf[4];
        pbuf_copy_partial(p, seconds_buf, 16, 40);
        uint32_t seconds_since_1900 = lwip_ntohl(seconds_buf[0]);
        ntp_time = seconds_since_1900 - NTP_DELTA + BEIJING_TIMEZONE_OFFSET;
        ntp_response_received = true;
        NTP_DEBUG("NTP时间同步成功: %lu (原始: %lu)", (unsigned long)ntp_time, (unsigned long)seconds_since_1900);
        
        // 显示时间信息
        time_t local_time = ntp_time;  // 复制到非volatile变量
        struct tm *tm_info = localtime(&local_time);
        printf("同步时间: %04d-%02d-%02d %02d:%02d:%02d (北京时间)\n", 
               tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    } else {
        NTP_DEBUG("无效的NTP回复: mode=%d, stratum=%d", mode, stratum);
    }
    
    pbuf_free(p);
}

// DNS回调函数
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    if (ipaddr) {
        NTP_DEBUG("DNS解析成功: %s -> %s", hostname, ip4addr_ntoa(ipaddr));
        
        struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
        if (pcb != NULL) {
            NTP_DEBUG("创建UDP PCB成功");
            udp_recv(pcb, ntp_recv, NULL);
            
            struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
            if (p != NULL) {
                uint8_t *req = (uint8_t*)p->payload;
                memset(req, 0, NTP_MSG_LEN);
                req[0] = 0x1b; // LI, VN, Mode
                
                NTP_DEBUG("发送NTP请求到 %s:%d", ip4addr_ntoa(ipaddr), NTP_PORT);
                err_t err = udp_sendto(pcb, p, ipaddr, NTP_PORT);
                if (err == ERR_OK) {
                    NTP_DEBUG("NTP请求发送成功");
                } else {
                    NTP_DEBUG("NTP请求发送失败: 错误码 %d", err);
                }
                pbuf_free(p);
            } else {
                NTP_DEBUG("分配NTP请求缓冲区失败");
            }
        } else {
            NTP_DEBUG("创建UDP PCB失败");
        }
    } else {
        NTP_DEBUG("DNS解析失败: %s", hostname);
    }
}

// 同步NTP时间 - 简化版本，参考AsyncDNSServer_RP2040W
bool sync_ntp_time() {
    ntp_sync_attempts++;
    printf("\n=== 开始NTP时间同步 (尝试 %d) ===\n", ntp_sync_attempts);
    NTP_DEBUG("目标NTP服务器: %s", NTP_SERVER);
    
    // 开始LED闪烁指示NTP同步
    led_set_blinking(true);
    
    if (!wifi_connected) {
        printf("❌ WiFi未连接，无法同步NTP\n");
        return false;
    }
    
    ntp_response_received = false;
    
    // 确保网络栈就绪
    printf("检查网络连接状态...\n");
    if (!netif_list || netif_list->ip_addr.addr == 0) {
        printf("❌ 网络接口未就绪\n");
        return false;
    }
    
    // 检查是否是IP地址，如果是则直接使用，否则进行DNS解析
    ip_addr_t ntp_server_addr;
    if (ip4addr_aton(NTP_SERVER, &ntp_server_addr)) {
        // 是IP地址，直接使用
        printf("使用IP地址直接连接NTP服务器: %s\n", NTP_SERVER);
        NTP_DEBUG("直接使用IP地址: %s", NTP_SERVER);
        ntp_dns_found(NTP_SERVER, &ntp_server_addr, NULL);
    } else {
        // 不是IP地址，需要DNS解析
        printf("需要DNS解析，查询域名: %s\n", NTP_SERVER);
        err_t err = dns_gethostbyname(NTP_SERVER, NULL, ntp_dns_found, NULL);
        NTP_DEBUG("DNS查询返回: %d", err);
        
        if (err == ERR_OK) {
            NTP_DEBUG("DNS缓存命中，立即获取结果");
        } else if (err == ERR_INPROGRESS) {
            NTP_DEBUG("DNS查询进行中，等待结果...");
        } else {
            NTP_DEBUG("DNS查询立即失败: %d", err);
            printf("❌ DNS查询失败\n");
            return false;
        }
    }
    
    // 等待NTP响应
    printf("等待NTP服务器响应（15秒超时）...\n");
    uint32_t start_wait = to_ms_since_boot(get_absolute_time());
    uint32_t last_status_print = start_wait;
    
    while (!ntp_response_received && (to_ms_since_boot(get_absolute_time()) - start_wait) < 15000) {
        cyw43_arch_poll();
        led_update(); // 更新LED闪烁
        
        // 每2秒打印一次状态
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - last_status_print >= 2000) {
            uint32_t elapsed = (current_time - start_wait) / 1000;
            printf("等待NTP响应... %lu/15秒\n", elapsed);
            last_status_print = current_time;
        }
        
        sleep_ms(50);
    }
    
    if (ntp_response_received) {
        printf("✅ NTP时间同步成功\n");
        NTP_DEBUG("NTP同步成功");
        // NTP同步成功，停止LED闪烁
        led_set_blinking(false);
        return true;
    } else {
        printf("❌ NTP同步超时（15秒）\n");
        NTP_DEBUG("NTP同步超时");
        // NTP同步失败，停止LED闪烁
        led_set_blinking(false);
        return false;
    }
}

// 显示WiFi状态
void print_wifi_status() {
    int status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    WIFI_DEBUG("WiFi链接状态: %d", status);
    
    switch(status) {
        case CYW43_LINK_DOWN:
            printf("WiFi状态: 链路断开\n");
            break;
        case CYW43_LINK_JOIN:
            printf("WiFi状态: 正在连接\n");
            break;
        case CYW43_LINK_NOIP:
            printf("WiFi状态: 已连接但无IP\n");
            break;
        case CYW43_LINK_UP:
            printf("WiFi状态: 已连接并获得IP\n");
            break;
        case CYW43_LINK_FAIL:
            printf("WiFi状态: 连接失败\n");
            break;
        case CYW43_LINK_NONET:
            printf("WiFi状态: 找不到网络\n");
            break;
        case CYW43_LINK_BADAUTH:
            printf("WiFi状态: 认证失败\n");
            break;
        default:
            printf("WiFi状态: 未知状态 %d\n", status);
            break;
    }
}

// 连接WiFi
bool connect_wifi() {
    wifi_connect_attempts++;
    printf("\n=== 开始WiFi连接 (尝试 %d) ===\n", wifi_connect_attempts);
    WIFI_DEBUG("目标网络: %s", WIFI_SSID);
    WIFI_DEBUG("密码长度: %d", strlen(WIFI_PASSWORD));
    
    printf("正在初始化WiFi硬件...\n");
    
    // 开始LED闪烁指示WiFi初始化
    led_set_blinking(true);
    
    // 确保系统稳定后再初始化WiFi
    printf("等待系统稳定...\n");
    sleep_ms(1000);
    
    // 添加超时保护和详细错误报告
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    printf("开始调用 cyw43_arch_init()...\n");
    
    // 分步骤初始化，更容易诊断问题
    tight_loop_contents(); // 确保系统处于良好状态
    
    int init_result = cyw43_arch_init();
    uint32_t init_time = to_ms_since_boot(get_absolute_time()) - start_time;
    
    if (init_result != 0) {
        printf("❌ WiFi硬件初始化失败，错误码: %d (耗时: %lu ms)\n", init_result, init_time);
        printf("可能的原因:\n");
        printf("  1. CYW43固件未正确加载\n");
        printf("  2. 硬件连接问题\n");
        printf("  3. 时钟配置问题\n");
        printf("  4. lwIP配置问题\n");
        
        // 清理资源并返回
        printf("尝试清理WiFi资源...\n");
        cyw43_arch_deinit();
        return false;
    }
    printf("✅ WiFi硬件初始化成功 (耗时: %lu ms)\n", init_time);
    
    printf("启用STA模式...\n");
    cyw43_arch_enable_sta_mode();
    WIFI_DEBUG("STA模式已启用");
    
    printf("✅ WiFi硬件就绪\n");
    
    // 显示可用网络
    printf("正在扫描可用网络...\n");
    sleep_ms(2000); // 给硬件一些时间启动
    
    // 尝试连接WiFi，最多重试5次
    for (int i = 0; i < 5; i++) {
        printf("\n--- WiFi连接尝试 %d/5 ---\n", i + 1);
        WIFI_DEBUG("开始连接，超时时间: 30秒");
        
        print_wifi_status();
        
        printf("正在连接到 %s...\n", WIFI_SSID);
        
        // 使用更稳定的连接方式，参考AsyncDNSServer_RP2040W
        printf("尝试连接WiFi网络（30秒超时）...\n");
        int connect_result = cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID, 
            WIFI_PASSWORD, 
            CYW43_AUTH_WPA2_MIXED_PSK, 
            30000
        );
        
        WIFI_DEBUG("连接结果: %d", connect_result);
        
        // 额外的状态检查
        if (connect_result == 0) {
            // 确认连接状态
            int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
            printf("连接后状态检查: %d\n", link_status);
        }
        
        if (connect_result == 0) {
            printf("✅ WiFi连接成功!\n");
            
            // 等待获取IP地址
            printf("等待获取IP地址...\n");
            for (int j = 0; j < 100; j++) { // 等待10秒获取IP
                led_update(); // 更新LED闪烁
                if (netif_list && netif_list->ip_addr.addr != 0) {
                    printf("✅ IP地址: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
                    printf("✅ 子网掩码: %s\n", ip4addr_ntoa(netif_ip4_netmask(netif_list)));
                    printf("✅ 网关: %s\n", ip4addr_ntoa(netif_ip4_gw(netif_list)));
                    wifi_connected = true;
                    // WiFi连接成功，停止LED闪烁
                    led_set_blinking(false);
                    return true;
                }
                sleep_ms(100);
                if (j % 10 == 0) {
                    printf("等待IP... %d/10秒\n", j/10);
                }
            }
            printf("⚠️ 获取IP地址超时\n");
        } else {
            printf("❌ WiFi连接失败，错误码: %d\n", connect_result);
            print_wifi_status();
        }
        
        if (i < 4) { // 不是最后一次尝试
            printf("等待5秒后重试...\n");
            for (int k = 0; k < 50; k++) { // 5秒 = 50 * 100ms
                led_update(); // 继续LED闪烁
                sleep_ms(100);
            }
        }
    }
    
    printf("❌ WiFi连接最终失败\n");
    // WiFi连接失败，停止LED闪烁
    led_set_blinking(false);
    return false;
}

// 获取当前时间
ClockTime getCurrentTime() {
    ClockTime time = {12, 0, 0, 1, 1, 2024, 1}; // 默认时间
    
    if (time_synced && ntp_time > 0) {
        // 使用同步的NTP时间
        time_t current_time = ntp_time + (to_ms_since_boot(get_absolute_time()) / 1000);
        struct tm *tm_info = localtime(&current_time);
        
        time.hours = tm_info->tm_hour;
        time.minutes = tm_info->tm_min;
        time.seconds = tm_info->tm_sec;
        time.day = tm_info->tm_mday;
        time.month = tm_info->tm_mon + 1;
        time.year = tm_info->tm_year + 1900;
        time.weekday = tm_info->tm_wday;
    } else {
        // 使用模拟时间（向前移动）
        static uint32_t last_update = 0;
        static ClockTime sim_time = {12, 0, 0, 1, 1, 2024, 1};
        
        uint32_t current_ms = to_ms_since_boot(get_absolute_time());
        if (current_ms - last_update >= 1000) {
            sim_time.seconds++;
            if (sim_time.seconds >= 60) {
                sim_time.seconds = 0;
                sim_time.minutes++;
                if (sim_time.minutes >= 60) {
                    sim_time.minutes = 0;
                    sim_time.hours++;
                    if (sim_time.hours >= 24) {
                        sim_time.hours = 0;
                        sim_time.day++;
                        sim_time.weekday = (sim_time.weekday + 1) % 7;
                        if (sim_time.day > 31) {
                            sim_time.day = 1;
                            sim_time.month++;
                            if (sim_time.month > 12) {
                                sim_time.month = 1;
                                sim_time.year++;
                            }
                        }
                    }
                }
            }
            last_update = current_ms;
        }
        time = sim_time;
    }
    
    return time;
}

// 绘制线条（简化版）
void drawLine(st7306::ST7306Driver& display, int x0, int y0, int x1, int y1, uint8_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    int x = x0, y = y0;
    
    while (true) {
        display.drawPixelGray(x, y, color);
        
        if (x == x1 && y == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

// 绘制厚线条（用于指针）
void drawThickLine(st7306::ST7306Driver& display, int x0, int y0, int x1, int y1, int thickness, uint8_t color) {
    // 绘制中心线
    drawLine(display, x0, y0, x1, y1, color);
    
    // 绘制周围的线条来增加厚度
    for (int t = 1; t <= thickness/2; t++) {
        drawLine(display, x0 + t, y0, x1 + t, y1, color);
        drawLine(display, x0 - t, y0, x1 - t, y1, color);
        drawLine(display, x0, y0 + t, x1, y1 + t, color);
        drawLine(display, x0, y0 - t, x1, y1 - t, color);
    }
}

// 绘制圆圈
void drawCircle(st7306::ST7306Driver& display, int cx, int cy, int radius, uint8_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;
    
    while (y >= x) {
        // 绘制8个对称点
        display.drawPixelGray(cx + x, cy + y, color);
        display.drawPixelGray(cx - x, cy + y, color);
        display.drawPixelGray(cx + x, cy - y, color);
        display.drawPixelGray(cx - x, cy - y, color);
        display.drawPixelGray(cx + y, cy + x, color);
        display.drawPixelGray(cx - y, cy + x, color);
        display.drawPixelGray(cx + y, cy - x, color);
        display.drawPixelGray(cx - y, cy - x, color);
        
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

// 绘制填充圆圈
void drawFilledCircle(st7306::ST7306Driver& display, int cx, int cy, int radius, uint8_t color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                display.drawPixelGray(cx + x, cy + y, color);
            }
        }
    }
}

// 绘制复古表盘
void drawVintageDial(st7306::ST7306Driver& display) {
    using namespace vintage_clock_config;
    
    // 绘制外圈
    drawCircle(display, CLOCK_CENTER_X, CLOCK_CENTER_Y, OUTER_RADIUS, COLOR_DIAL_DARK);
    drawCircle(display, CLOCK_CENTER_X, CLOCK_CENTER_Y, OUTER_RADIUS - 1, COLOR_DIAL_DARK);
    
    // 绘制内圈
    drawCircle(display, CLOCK_CENTER_X, CLOCK_CENTER_Y, INNER_RADIUS, COLOR_DIAL_MEDIUM);
    
    // 绘制小时刻度（12个）
    for (int i = 0; i < 12; i++) {
        float angle = i * M_PI / 6.0f - M_PI / 2.0f; // 从12点开始
        
        // 小时刻度线（粗线）
        int x1 = CLOCK_CENTER_X + HOUR_MARK_INNER * cos(angle);
        int y1 = CLOCK_CENTER_Y + HOUR_MARK_INNER * sin(angle);
        int x2 = CLOCK_CENTER_X + HOUR_MARK_OUTER * cos(angle);
        int y2 = CLOCK_CENTER_Y + HOUR_MARK_OUTER * sin(angle);
        
        // 绘制粗小时刻度
        drawThickLine(display, x1, y1, x2, y2, 3, COLOR_DIAL_DARK);
    }
    
    // 绘制分钟刻度（60个，跳过小时位置）
    for (int i = 0; i < 60; i++) {
        if (i % 5 != 0) { // 跳过小时刻度位置
            float angle = i * M_PI / 30.0f - M_PI / 2.0f;
            
            int x1 = CLOCK_CENTER_X + MINUTE_MARK_INNER * cos(angle);
            int y1 = CLOCK_CENTER_Y + MINUTE_MARK_INNER * sin(angle);
            int x2 = CLOCK_CENTER_X + MINUTE_MARK_OUTER * cos(angle);
            int y2 = CLOCK_CENTER_Y + MINUTE_MARK_OUTER * sin(angle);
            
            drawLine(display, x1, y1, x2, y2, COLOR_DIAL_LIGHT);
        }
    }
    
    // 绘制数字标记（12, 3, 6, 9）
    const char* numbers[4] = {"12", "3", "6", "9"};
    const float number_angles[4] = {-M_PI/2, 0, M_PI/2, M_PI}; // 12, 3, 6, 9点位置
    
    for (int i = 0; i < 4; i++) {
        int x = CLOCK_CENTER_X + NUMBER_RADIUS * cos(number_angles[i]);
        int y = CLOCK_CENTER_Y + NUMBER_RADIUS * sin(number_angles[i]);
        
        // 调整数字位置以居中
        int str_width = strlen(numbers[i]) * 8; // 8是字符宽度
        int str_height = 16; // 16是字符高度
        
        x -= str_width / 2;
        y -= str_height / 2;
        
        // 绘制数字
        display.drawString(x, y, numbers[i], true);
    }
}

// 绘制时钟指针
void drawClockHands(st7306::ST7306Driver& display, const ClockTime& time) {
    using namespace vintage_clock_config;
    
    // 计算角度（从12点开始，顺时针）
    float hour_angle = ((time.hours % 12) + time.minutes / 60.0f) * M_PI / 6.0f - M_PI / 2.0f;
    float minute_angle = time.minutes * M_PI / 30.0f - M_PI / 2.0f;
    float second_angle = time.seconds * M_PI / 30.0f - M_PI / 2.0f;
    
    // 计算指针终点
    int hour_x = CLOCK_CENTER_X + HOUR_HAND_LENGTH * cos(hour_angle);
    int hour_y = CLOCK_CENTER_Y + HOUR_HAND_LENGTH * sin(hour_angle);
    
    int minute_x = CLOCK_CENTER_X + MINUTE_HAND_LENGTH * cos(minute_angle);
    int minute_y = CLOCK_CENTER_Y + MINUTE_HAND_LENGTH * sin(minute_angle);
    
    int second_x = CLOCK_CENTER_X + SECOND_HAND_LENGTH * cos(second_angle);
    int second_y = CLOCK_CENTER_Y + SECOND_HAND_LENGTH * sin(second_angle);
    
    // 绘制时针（最粗）
    drawThickLine(display, CLOCK_CENTER_X, CLOCK_CENTER_Y, hour_x, hour_y, 6, COLOR_HOUR_HAND);
    
    // 绘制分针（中等粗细）
    drawThickLine(display, CLOCK_CENTER_X, CLOCK_CENTER_Y, minute_x, minute_y, 4, COLOR_MINUTE_HAND);
    
    // 绘制秒针（最细）
    drawThickLine(display, CLOCK_CENTER_X, CLOCK_CENTER_Y, second_x, second_y, 2, COLOR_SECOND_HAND);
    
    // 绘制中心圆点
    drawFilledCircle(display, CLOCK_CENTER_X, CLOCK_CENTER_Y, CENTER_DOT_RADIUS, COLOR_DIAL_DARK);
}

// 绘制状态信息
void drawStatusInfo(st7306::ST7306Driver& display, const ClockTime& time) {
    using namespace vintage_clock_config;
    
    // 绘制连接状态 (左上角)
    if (wifi_connected) {
        display.drawString(5, 5, "WiFi", true);
    } else {
        display.drawString(5, 5, "NoWiFi", true);
    }
    
    // 绘制时间同步状态 (右上角)
    if (time_synced) {
        display.drawString(SCREEN_WIDTH - 35, 5, "NTP", true);
    } else {
        display.drawString(SCREEN_WIDTH - 35, 5, "SIM", true);
    }
    
    // 绘制日期 (屏幕中间第一行) - 格式：YYYY-MM-DD
    char date_str[16];
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", time.year, time.month, time.day);
    int date_x = CLOCK_CENTER_X - 40; // 居中显示
    display.drawString(date_x, 25, date_str, true);  // 上调25像素：50->25
    
    // 绘制星期几 (屏幕中间第二行)
    int weekday_x = CLOCK_CENTER_X - 15; // 居中显示
    display.drawString(weekday_x, 45, weekday_short[time.weekday], true);  // 上调25像素：70->45
}

// 绘制装饰元素
void drawDecorations(st7306::ST7306Driver& display) {
    using namespace vintage_clock_config;
    
    // 在时钟上方绘制装饰线条
    int line_y = CLOCK_CENTER_Y - OUTER_RADIUS - 15;
    drawLine(display, CLOCK_CENTER_X - 50, line_y, CLOCK_CENTER_X + 50, line_y, COLOR_DIAL_MEDIUM);
    drawLine(display, CLOCK_CENTER_X - 40, line_y + 3, CLOCK_CENTER_X + 40, line_y + 3, COLOR_DIAL_LIGHT);
}

int main() {
    stdio_init_all();
    
    // 首先确保串口输出正常
    printf("\n");
    printf("=====================================\n");
    printf("    ST7306 WiFi NTP 复古时钟\n");
    printf("=====================================\n");
    printf("编译时间: %s %s\n", __DATE__, __TIME__);
    printf("WiFi网络: %s\n", WIFI_SSID);
    printf("NTP服务器: %s\n", NTP_SERVER);
    printf("时区设置: UTC+8 (北京时间)\n");
    printf("=====================================\n\n");

    // 添加启动延时，确保串口稳定
    printf("串口测试中...\n");
    for (int i = 0; i < 5; i++) {
        printf("倒计时: %d 秒\n", 5-i);
        sleep_ms(1000);
    }
    printf("串口测试完成！\n\n");

    printf("正在初始化硬件...\n");
    
    // 初始化显示器
    printf("- 初始化ST7306显示器...\n");
    st7306::ST7306Driver display(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN);
    pico_gfx::PicoDisplayGFX<st7306::ST7306Driver> gfx(display, st7306::ST7306Driver::LCD_WIDTH, st7306::ST7306Driver::LCD_HEIGHT);

    display.initialize();
    printf("  ✅ 显示器初始化完成\n");

    // 设置屏幕方向
    const int rotation = 0;
    gfx.setRotation(rotation);
    display.setRotation(rotation);
    printf("  ✅ 屏幕方向设置: %s (%dx%d)\n", 
           rotation == 0 ? "竖屏" : "横屏",
           vintage_clock_config::SCREEN_WIDTH, vintage_clock_config::SCREEN_HEIGHT);
    
    printf("✅ 硬件初始化完成\n\n");

    // 显示启动画面
    printf("显示启动信息到屏幕...\n");
    display.clearDisplay();
    display.fill(vintage_clock_config::COLOR_BACKGROUND);
    display.drawString(30, 180, "WIFI NTP CLOCK", true);
    display.drawString(80, 200, "Starting...", true);
    display.display();
    sleep_ms(2000);

    // 显示WiFi连接状态
    printf("显示WiFi连接状态...\n");
    display.clearDisplay();
    display.fill(vintage_clock_config::COLOR_BACKGROUND);
    display.drawString(50, 180, "Connecting WiFi", true);
    display.drawString(80, 200, WIFI_SSID, true);
    display.display();
    
    printf("开始WiFi连接过程...\n");
    
    // 系统状态检查
    printf("系统状态检查:\n");
    printf("  - 系统时钟: %lu Hz\n", clock_get_hz(clk_sys));
    printf("  - 运行时间: %lu ms\n", to_ms_since_boot(get_absolute_time()));
    printf("  - 系统就绪，开始WiFi初始化\n");
    printf("开始WiFi连接（最多等待60秒）...\n");
    bool wifi_success = connect_wifi();
    
    if (wifi_success) {
        printf("✅ WiFi连接成功，开始NTP同步\n");
        
        display.clearDisplay();
        display.fill(vintage_clock_config::COLOR_BACKGROUND);
        display.drawString(60, 180, "WiFi Connected", true);
        display.drawString(60, 200, "Syncing Time...", true);
        display.display();
        
        // 同步NTP时间（也添加超时保护）
        printf("开始NTP同步（最多等待30秒）...\n");
        bool ntp_success = sync_ntp_time();
        
        if (ntp_success) {
            time_synced = true;
            printf("✅ NTP时间同步成功\n");
            
            display.clearDisplay();
            display.fill(vintage_clock_config::COLOR_BACKGROUND);
            display.drawString(70, 180, "Time Synced", true);
            display.drawString(80, 200, "Starting...", true);
            display.display();
            sleep_ms(2000);
        } else {
            printf("❌ NTP同步失败，使用模拟时间\n");
            
            display.clearDisplay();
            display.fill(vintage_clock_config::COLOR_BACKGROUND);
            display.drawString(60, 180, "NTP Sync Failed", true);
            display.drawString(60, 200, "Using Sim Time", true);
            display.display();
            sleep_ms(2000);
        }
    } else {
        printf("❌ WiFi连接失败，使用模拟时间\n");
        
        display.clearDisplay();
        display.fill(vintage_clock_config::COLOR_BACKGROUND);
        display.drawString(60, 180, "WiFi Failed", true);
        display.drawString(60, 200, "Using Sim Time", true);
        display.display();
        sleep_ms(2000);
    }
    
    printf("\n=== 初始化完成，开始时钟运行 ===\n");
    printf("WiFi状态: %s\n", wifi_connected ? "已连接" : "未连接");
    printf("时间同步: %s\n", time_synced ? "NTP同步" : "模拟时间");
    printf("==============================\n\n");

    // 主时钟循环
    uint32_t last_display_update = 0;
    uint32_t last_ntp_sync = 0;
    uint32_t last_status_print = 0;
    
    // 如果首次NTP同步成功，记录当前时间作为上次同步时间
    if (time_synced) {
        last_ntp_sync = to_ms_since_boot(get_absolute_time());
        printf("📝 记录首次NTP同步时间: %lu ms\n", last_ntp_sync);
    }
    const uint32_t DISPLAY_UPDATE_INTERVAL = 100; // 10 FPS刷新率
    const uint32_t NTP_SYNC_INTERVAL = 43200000; // 每12小时同步一次（43200秒）
    const uint32_t STATUS_PRINT_INTERVAL = 5000; // 每5秒打印一次状态
    
    printf("进入主循环...\n");
    int loop_count = 0;
    
    while (true) {
        uint32_t current_ms = to_ms_since_boot(get_absolute_time());
        loop_count++;
        
        // 每1000次循环打印一次心跳
        if (loop_count % 1000 == 0) {
            printf("主循环心跳: %d (运行时间: %lu ms)\n", loop_count, current_ms);
        }
        
        // 处理WiFi轮询（poll模式）
        if (wifi_connected) {
            cyw43_arch_poll();
        }
        
        // 定期打印状态
        if (current_ms - last_status_print >= STATUS_PRINT_INTERVAL) {
            last_status_print = current_ms;
            ClockTime current_time = getCurrentTime();
            
            // 计算距离下次NTP同步的时间
            if (time_synced && last_ntp_sync > 0) {
                uint32_t elapsed_ms = current_ms - last_ntp_sync;
                uint32_t remaining_ms = NTP_SYNC_INTERVAL - elapsed_ms;
                uint32_t remaining_hours = remaining_ms / (1000 * 3600);
                uint32_t remaining_minutes = (remaining_ms % (1000 * 3600)) / (1000 * 60);
                
                printf("时钟状态: %02d:%02d:%02d, WiFi: %s, 同步: %s, 下次NTP同步: %lu小时%lu分钟后\n", 
                       current_time.hours, current_time.minutes, current_time.seconds,
                       wifi_connected ? "连接" : "断开",
                       time_synced ? "NTP" : "模拟",
                       remaining_hours, remaining_minutes);
            } else {
                printf("时钟状态: %02d:%02d:%02d, WiFi: %s, 同步: %s\n", 
                       current_time.hours, current_time.minutes, current_time.seconds,
                       wifi_connected ? "连接" : "断开",
                       time_synced ? "NTP" : "模拟");
            }
        }
        
        // 定期重新同步NTP时间
        if (wifi_connected && time_synced && (current_ms - last_ntp_sync >= NTP_SYNC_INTERVAL)) {
            uint32_t elapsed_hours = (current_ms - last_ntp_sync) / (1000 * 3600);
            printf("\n⏰ 到达定期同步时间 (已过 %lu 小时)，重新同步NTP时间...\n", elapsed_hours);
            if (sync_ntp_time()) {
                last_ntp_sync = current_ms;
                printf("✅ NTP时间重新同步成功，下次同步将在12小时后\n\n");
            } else {
                printf("❌ NTP重新同步失败，将在下个周期重试\n\n");
            }
        }
        
        // 控制显示刷新率
        if (current_ms - last_display_update >= DISPLAY_UPDATE_INTERVAL) {
            last_display_update = current_ms;
            
            ClockTime current_time = getCurrentTime();
            
            // 清屏
            display.clearDisplay();
            display.fill(vintage_clock_config::COLOR_BACKGROUND);
            
            // 绘制表盘
            drawVintageDial(display);
            
            // 绘制装饰元素
            drawDecorations(display);
            
            // 绘制指针
            drawClockHands(display, current_time);
            
            // 绘制状态和日期信息
            drawStatusInfo(display, current_time);
            
            // 更新显示
            display.display();
        }
        
        // 短暂延时
        sleep_ms(50);
    }
    
    return 0;
} 
