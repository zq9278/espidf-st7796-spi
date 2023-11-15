#include <stdio.h>
#include "freertos/FreeRTOS.h"  // 引入FreeRTOS操作系统头文件
#include "freertos/task.h"      // 引入FreeRTOS任务管理的头文件
#include "esp_timer.h"          // 引入ESP32的定时器相关头文件
#include "esp_lcd_panel_io.h"   // 引入LCD面板输入输出管理头文件
#include "esp_lcd_panel_vendor.h" // 引入ESP32 LCD面板供应商相关头文件
#include "esp_lcd_panel_ops.h"  // 引入LCD面板操作相关头文件
#include "driver/gpio.h"        // 引入GPIO（通用输入输出）驱动头文件
#include "driver/spi_master.h"  // 引入SPI主设备驱动头文件
#include "esp_err.h"            // 引入ESP32错误处理头文件
#include "esp_log.h"            // 引入日志打印相关头文件
#include "lvgl.h"               // 引入LVGL图形库头文件
#include "driver/spi_master.h"  // 重复引入SPI主设备驱动头文件（应考虑去除重复）
#include "esp_lcd_st7796.h"     // 引入ST7796 LCD驱动头文件

static const char *TAG = "example";  // 定义一个日志标签

// 使用SPI2作为例子
#define LCD_HOST  SPI2_HOST // 定义LCD主机为SPI2

// 请根据您的LCD规格更新下面的配置
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (80 * 1000 * 1000)  // 定义LCD像素时钟频率为20MHz
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1                   // 定义LCD背光打开电平为1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  // 定义LCD背光关闭电平
#define EXAMPLE_PIN_NUM_SCLK           6                   // 定义SPI时钟引脚号
#define EXAMPLE_PIN_NUM_MOSI           7                   // 定义SPI主输出从输入（MOSI）引脚号
#define EXAMPLE_PIN_NUM_MISO           2                   // 定义SPI主输入从输出（MISO）引脚号
#define EXAMPLE_PIN_NUM_LCD_DC         9                   // 定义LCD数据/命令（DC）引脚号
#define EXAMPLE_PIN_NUM_LCD_RST        4                   // 定义LCD复位引脚号
#define EXAMPLE_PIN_NUM_LCD_CS         10                  // 定义LCD片选（CS）引脚号
#define EXAMPLE_PIN_NUM_BK_LIGHT       5                   // 定义背光控制引脚号


// 水平和垂直方向的像素数量
#define EXAMPLE_LCD_H_RES              320                 // 定义LCD水平分辨率
#define EXAMPLE_LCD_V_RES              480                 // 定义LCD垂直分辨率

// 用于表示命令和参数的位数
#define EXAMPLE_LCD_CMD_BITS           8                   // 定义LCD命令位数
#define EXAMPLE_LCD_PARAM_BITS         8                   // 定义LCD参数位数
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2                   // 定义LVGL的时钟周期（毫秒）

extern void example_lvgl_demo_ui(lv_disp_t *disp);  // 声明示例LVGL UI函数

// 当LVGL完成刷新时，通知函数
static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;  // 转换用户上下文为显示驱动
    lv_disp_flush_ready(disp_driver);  // 通知LVGL，刷新已经完成
    return false;
}

// LVGL的刷新回调函数
static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;  // 获取LCD面板句柄
    int offsetx1 = area->x1;  // 获取刷新区域的起始x坐标
    int offsetx2 = area->x2;  // 获取刷新区域的终止x坐标
    int offsety1 = area->y1;  // 获取刷新区域的起始y坐标
    int offsety2 = area->y2;  // 获取刷新区域的终止y坐标
    // 将缓冲区的内容复制到显示屏的指定区域
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

// 当LVGL屏幕旋转时，旋转显示和触控。当驱动参数更新时调用。
static void example_lvgl_port_update_callback(lv_disp_drv_t *drv)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;  // 获取LCD面板句柄

    switch (drv->rotated) {  // 根据旋转状态处理
    case LV_DISP_ROT_NONE:
        // 旋转LCD显示
        esp_lcd_panel_swap_xy(panel_handle, false);  // 设置是否交换x和y
        esp_lcd_panel_mirror(panel_handle, true, false);  // 设置镜像显示
        break;
    case LV_DISP_ROT_90:
        // 旋转LCD显示
        esp_lcd_panel_swap_xy(panel_handle, true);  // 交换x和y
        esp_lcd_panel_mirror(panel_handle, true, true);  // 设置镜像显示
        break;
    case LV_DISP_ROT_180:
        // 旋转LCD显示
        esp_lcd_panel_swap_xy(panel_handle, false);  // 不交换x和y
        esp_lcd_panel_mirror(panel_handle, false, true);  // 设置镜像显示
        break;
    case LV_DISP_ROT_270:
        // 旋转LCD显示
        esp_lcd_panel_swap_xy(panel_handle, true);  // 交换x和y
        esp_lcd_panel_mirror(panel_handle, false, false);  // 不设置镜像显示
        break;
    }
}

// 增加LVGL时钟
static void example_increase_lvgl_tick(void *arg)
{
    // 通知LVGL，经过了多少毫秒
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

// 主程序入口
void app_main(void)
{
    static lv_disp_draw_buf_t disp_buf;  // 包含称为绘制缓冲区的内部图形缓冲区
    static lv_disp_drv_t disp_drv;       // 包含回调函数的显示驱动

    ESP_LOGI(TAG, "Turn off LCD backlight");  // 打印日志：关闭LCD背光
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,  // 设置GPIO模式为输出
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT  // 设置背光控制引脚
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));  // 配置GPIO并检查错误
    ESP_LOGI(TAG, "Initialize SPI bus");  // 打印日志：初始化SPI总线
    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,  // 设置时钟引脚
        .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,  // 设置MOSI引脚
        .miso_io_num = EXAMPLE_PIN_NUM_MISO,  // 设置MISO引脚
        .quadwp_io_num = -1,  // 设置QUADWP引脚（不使用）
        .quadhd_io_num = -1,  // 设置QUADHD引脚（不使用）
        .max_transfer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t),  // 设置最大传输大小
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));  // 初始化SPI总线并检查错误
    ESP_LOGI(TAG, "Install panel IO");  // 打印日志：安装

    esp_lcd_panel_io_handle_t io_handle = NULL;  // 定义一个 LCD 面板的 IO 句柄，并初始化为 NULL

// 定义 SPI 通信的配置参数
esp_lcd_panel_io_spi_config_t io_config = {
    .dc_gpio_num = EXAMPLE_PIN_NUM_LCD_DC,  // DC（数据/命令）引脚号
    .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,  // CS（片选）引脚号
    .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,  // 像素时钟频率
    .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,  // LCD 命令位数
    .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,  // LCD 参数位数
    .spi_mode = 0,  // SPI 模式
    .trans_queue_depth = 10,  // 事务队列深度
    .on_color_trans_done = example_notify_lvgl_flush_ready,  // 颜色传输完成后的回调函数
    .user_ctx = &disp_drv,  // 用户上下文
};

// 将 LCD 面板通过 SPI 总线连接
ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

esp_lcd_panel_handle_t panel_handle = NULL;  // 定义一个 LCD 面板的句柄，并初始化为 NULL

// 定义 LCD 面板的配置参数
esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,  // 复位引脚号
    .rgb_endian = LCD_RGB_ENDIAN_BGR,  // RGB 数据的字节顺序
    .bits_per_pixel = 16,  // 每个像素的位数
};

// 安装并初始化 ST7796 LCD 驱动
ESP_LOGI(TAG, "Install st7796 panel driver");
ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));
ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));  // 重置 LCD 面板
ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));  // 初始化 LCD 面板
//ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));  // 设置 LCD 面板的镜像

// 在打开屏幕或背光前，用户可以向屏幕刷新预定义的图案
ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
ESP_LOGI(TAG, "Turn on LCD backlight");  // 打开 LCD 背光
gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);  // 设置背光的 GPIO 级别

// 初始化 LVGL 图形库
ESP_LOGI(TAG, "Initialize LVGL library");
lv_init();

// 为 LVGL 分配绘图缓冲区
// 建议绘图缓冲区大小至少为屏幕大小的 1/10
lv_color_t *buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
assert(buf1);
lv_color_t *buf2 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
assert(buf2);
// 初始化 LVGL 绘图缓冲区
lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * 20);

// 向 LVGL 注册显示驱动
ESP_LOGI(TAG, "Register display driver to LVGL");
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res = EXAMPLE_LCD_H_RES;  // 水平分辨率
disp_drv.ver_res = EXAMPLE_LCD_V_RES;  // 垂直分辨率
disp_drv.flush_cb = example_lvgl_flush_cb;  // 刷新回调函数
disp_drv.drv_update_cb = example_lvgl_port_update_callback;  // 驱动更新回调函数
disp_drv.draw_buf = &disp_buf;  // 绘图缓冲区
disp_drv.user_data = panel_handle;  // 用户数据
lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

// 安装 LVGL 的定时器
ESP_LOGI(TAG, "Install LVGL tick timer");
const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &example_increase_lvgl_tick,  // 定时器回调函数
    .name = "lvgl_tick"  // 定时器名称
};
esp_timer_handle_t lvgl_tick_timer = NULL;
ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

// 显示 LVGL 的仪表小部件
ESP_LOGI(TAG, "Display LVGL Meter Widget");
example_lvgl_demo_ui(disp);

// 主循环
while (1) {
    // 提高 LVGL 任务的优先级和/或减少处理器周期可以提高性能
    vTaskDelay(pdMS_TO_TICKS(10));
    // 运行 lv_timer_handler 的任务应该比运行 `lv_tick_inc` 的优先级低
    lv_timer_handler();
 // ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));  // 初始化 LCD 面板
}
}