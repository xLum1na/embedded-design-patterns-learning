#include "display_serive.h"
#include "ili9488_driver.h"
#include "lvgl.h"
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>

// --- 1. 配置屏幕参数 ---
#define SCREEN_WIDTH  320  // 根据你的屏幕实际分辨率修改
#define SCREEN_HEIGHT 480
#define BUFFER_SIZE   (SCREEN_WIDTH * SCREEN_HEIGHT / 10) // 缓冲区大小：屏幕的1/10

// --- 2. 定义全局变量 ---
static lv_display_t * lvgl_disp; // LVGL v9 的显示对象指针
static lv_color_t draw_buf[BUFFER_SIZE]; // 像素绘图缓冲区 (裸数组)


void ili9488_init(void)
{

    
}


// --- 3. 刷新回调函数 (关键) ---
// 这个函数负责将LVGL渲染好的数据推送到ILI9488屏幕上
// 修改后的刷新回调函数 (适配 LVGL v9)
void my_disp_flush(lv_display_t * disp, const lv_area_t * area, void * px_map,  void * user_data)
{
    // 1. 获取要刷新的区域宽度和高度
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    // 2. 调用ILI9488驱动的绘图函数
    // 注意：这里需要根据你具体的 ili9488.c 驱动接口进行调整
    // 示例伪代码 (请替换为你驱动中的实际函数):
    // ili9488_flush(area->x1, area->y1, w, h, (uint16_t*)px_map);

    // 3. 通知LVGL刷新完成 (必须调用，否则卡死)
    lv_display_flush_ready(disp);
}

// --- 4. 主函数 ---
void app_main(void)
{
    // 1. 初始化底层硬件 (SPI总线、GPIO等)
    // ili9488_init() 应该包含SPI句柄的初始化和屏幕的复位
    ili9488_init(); 

    // 2. 初始化LVGL库
    lv_init();

    // 3. 创建并配置显示对象 (LVGL v9 核心步骤)
    
    // 3.1 创建显示对象
    lvgl_disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // 3.2 设置颜色格式 (通常为RGB565)
    lv_display_set_color_format(lvgl_disp, LV_COLOR_FORMAT_RGB565);
    
    // 3.3 设置刷新回调函数
    lv_display_set_flush_cb(lvgl_disp, my_disp_flush);
    
    // 3.4 设置缓冲区 (替代了旧版的 lv_disp_draw_buf_t)
    // 参数: 显示对象, 缓冲区1, 缓冲区2(双缓冲), 缓冲区大小, 渲染模式
    lv_display_set_buffers(lvgl_disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // --- 5. 创建UI界面 ---
    // 创建一个标签并显示文字
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello ILI9488!\nLVGL v9 Running.");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0); // 居中对齐

    // --- 6. 启动LVGL任务循环 ---
    // 这个循环必须持续运行
    while (1) {
        lv_timer_handler(); // v9 中推荐使用 lv_timer_handler() 处理任务
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}