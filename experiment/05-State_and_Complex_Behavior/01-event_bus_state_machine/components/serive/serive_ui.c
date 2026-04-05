#include "serive_ui.h"
#include "driver_display.h"
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "string.h"
#include "math.h"
#include "lvgl.h"

//lcdio句柄
static esp_lcd_panel_io_handle_t lcd_io_handle = NULL;
//lcd面板句柄
static esp_lcd_panel_handle_t lcd_panel_handle = NULL;
//lvgl显示设备
static lv_display_t *display = NULL;
//lvgl缓冲区
static lv_color_t *lv_buf_1 = NULL;
//定义显示界面全局指针
static lv_obj_t *s_init_screen = NULL;
static lv_obj_t *s_init_label = NULL;

//刷新缓冲区标志位
static volatile bool s_lcd_busy = false; 

static uint32_t get_tick_ms(void);
static void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_buf);
static void display_init(void);
static void display_deinit(void);
static void create_init_screen(void);
static void update_init_text(const char * text);
static void remove_init_screen(void);


//lvgltick回调
static uint32_t get_tick_ms(void) 
{
    // 返回值是微秒 (int64_t)
    int64_t time_us = esp_timer_get_time();
    // 转换为毫秒
    uint32_t time_ms = (uint32_t)(time_us / 1000);
    return time_ms;
}

static void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_buf)
{
    //等待上一帧传输完成
    while (s_lcd_busy) {
        //阻塞等待
    }
    s_lcd_busy = true; //标记忙碌
    //启动传输
    esp_err_t ret = esp_lcd_panel_draw_bitmap(lcd_panel_handle, 
                            area->x1, area->y1, 
                            area->x2 + 1, area->y2 + 1, 
                            px_buf);
    
    if (ret != ESP_OK) {
        s_lcd_busy = false; //如果失败，立即释放
        lv_display_flush_ready(disp); //通知LVGL结束
    }
}

//SPI传输完成的回调
static bool notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    //标记总线空闲
    s_lcd_busy = false;
    //安全地通知 LVGL
    //使用user_ctx如果它有效，否则回退到全局变量display
    lv_display_t *disp = (lv_display_t *)user_ctx;
    if (disp == NULL) {
        disp = display;
    }
    //双重检查，防止空指针崩溃
    if (disp != NULL) {
        lv_display_flush_ready(disp);
    }
    //返回true处理完成
    return true;
}

//显示g背光初始化
static void display_brightness_init(void)
{
    //LEDC通道配置
    const ledc_channel_config_t LCD_backlight_channel =
    {
        .gpio_num = LCD_LEDC,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_1,
        .duty = 0,
        .hpoint = 0,
        .flags = 
        {
            .output_invert = 0
        }
    };
    //LEDC时钟配置
    const ledc_timer_config_t LCD_backlight_timer =
    {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));
}

//设置背光
static void display_brightness_set(int brightness_percentage)
{
    if (brightness_percentage > 100)
    {
        brightness_percentage = 100;
    }    
    else if (brightness_percentage < 0)
    {
        brightness_percentage = 0;
    }
    uint32_t duty_cycle = (1023 * brightness_percentage) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

//显示初始化
static void display_init(void)
{
    //ledc背光初始化
    display_brightness_init();
    //设置背光为0
    display_brightness_set(0);
    //初始化spi总线
    spi_bus_config_t bus_cfg = {
        .miso_io_num = LCD_TOUCH_SPI_MISO,
        .mosi_io_num = LCD_TOUCH_SPI_MOSI,
        .sclk_io_num = LCD_TOUCH_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32768,
        .flags = 0, 
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
    //将LCD连接到SPI总线
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_SPI_DC,
        .cs_gpio_num = LCD_SPI_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = notify_flush_ready,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &lcd_io_handle));
    //创建显示面板
    esp_lcd_panel_dev_config_t dev_cfg = {
        .reset_gpio_num = LCD_RESET,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 18, // 物理接口是 18-bit
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB, // 确保 RGB 顺序
        .flags = {
            .reset_active_high = 0,
        },
        .vendor_config = NULL
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9488(lcd_io_handle, &dev_cfg, LV_BUFFER_SIZE, &lcd_panel_handle));
    //初始化显示面板
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_panel_handle, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_panel_handle, false));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(lcd_panel_handle, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(lcd_panel_handle, 0, 0));

    //绑定lvgl显示
    lv_init();
    //绑定tick回调
    lv_tick_set_cb(get_tick_ms);
    //创建显示设备
    if (lcd_panel_handle != NULL) {
        display = lv_display_create(DISPLAY_HORIZONTAL_PIXELS, DISPLAY_VERTICAL_PIXELS);
        lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
        //创建缓冲区
        lv_buf_1 = (lv_color_t *)heap_caps_malloc(LV_BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
        if (!lv_buf_1) {
            display_deinit();
            return; 
        }
        lv_display_set_buffers(display, lv_buf_1, NULL, LV_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
        //绑定刷新回调
        lv_display_set_flush_cb(display, my_flush_cb);
    }
    //设置满背光
    display_brightness_set(100);
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_panel_handle, true));
    //初始化显示lvgl界面
    //显示系统初始化ing界面
    create_init_screen();
    //刷新
    lv_refr_now(NULL);
}

//显示反初始化
static void display_deinit(void)
{
    lv_deinit();
    //释放DMA缓冲区
    if (lv_buf_1 != NULL) {
        heap_caps_free(lv_buf_1);
        lv_buf_1 = NULL;
    }
    //删除面板
    if (lcd_panel_handle != NULL) {
        esp_lcd_panel_del(lcd_panel_handle);
        lcd_panel_handle = NULL;
    }
    //删除IO
    if (lcd_io_handle != NULL) {
        esp_lcd_panel_io_del(lcd_io_handle);
        lcd_io_handle = NULL;
    }
    //释放总线
    spi_bus_free(SPI2_HOST); 
}

static void create_init_screen(void)
{
    //创建一个全屏的容器（可选，用于居中）
    s_init_screen = lv_obj_create(lv_screen_active());
    lv_obj_set_size(s_init_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_init_screen, lv_color_hex(0x000000), 0); // 黑色背景
    lv_obj_set_style_bg_opa(s_init_screen, LV_OPA_100, 0);
    lv_obj_center(s_init_screen);
    lv_obj_set_flex_flow(s_init_screen, LV_FLEX_FLOW_COLUMN); // 垂直排列
    lv_obj_set_flex_align(s_init_screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    //添加一个图标 (可选，LVGL 内置符号)
    lv_obj_t * icon = lv_label_create(s_init_screen);
    lv_label_set_text(icon, LV_SYMBOL_SETTINGS); // 使用内置符号
    lv_obj_set_style_text_color(icon, lv_color_hex(0x00FF00), 0); // 绿色
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, 0); // 大字体

    //添加文字标签
    s_init_label = lv_label_create(s_init_screen);
    lv_label_set_text(s_init_label, "System Initializing...");
    lv_obj_set_style_text_color(s_init_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_init_label, &lv_font_montserrat_14, 0);

    //添加一个动画，表示正在加载
    lv_obj_t * spinner = lv_spinner_create(s_init_screen);
    lv_obj_set_size(spinner, 50, 50);
    lv_obj_set_style_outline_color(spinner, lv_color_hex(0x00FF00), 0);
}

static void update_init_text(const char * text)
{
    if (s_init_label) {
        lv_label_set_text(s_init_label, text);
    }
}

static void remove_init_screen(void)
{
    if (s_init_screen) {
        lv_obj_del(s_init_screen);
        s_init_screen = NULL;
    }
}

// static event_bus_handle_t ui_bus_hadnle = NULL;
// typedef enum {
//     STA_UI_IDLE = 0,
//     STA_UI_INIT,
// }ui_state_t;

// typedef struct ui_t {
//     ui_state_t current_state;
// }ui_t;

// //服务api
// void ui_init(void)
// {
//     display_init();
// }


// //状态机初始化状态转换
// void ui_fsm_init(event_bus_handle_t bus, ui_t *ui_handle)
// {
//     ui_handle->current_state = STA_UI_INIT;
//     ui_bus_hadnle = bus;
//     //订阅事件
//     event_bus_subscribe(ui_bus_hadnle, ev.type, ui_fsm_process, NULL)

// }

// void ui_fsm_process(ui_t ui_handle, event_type_t *ev, void *ctx)
// {
//     //处理事件
//     switch (ui_handle.current_state) {
//         case STA_UI_IDLE:
//             ui_idle_handler(ev, ctx);
//             break;
//         default:
//             break;
//     }

// }




//测试
void ui_test_task(void *arg)
{
    //显示初始化
    display_init();

    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
}