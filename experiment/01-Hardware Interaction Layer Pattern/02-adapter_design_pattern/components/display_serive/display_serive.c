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
#include "freertos/FreeRTOS.h"


static const char *TAG = "display_serive";


//lcd相关资源
static esp_lcd_panel_io_handle_t lcd_io_handle = NULL;
static esp_lcd_panel_handle_t lcd_handle = NULL;


//lvgl相关资源
static lv_color_t *lv_buf_1 = NULL;



/**************************************************************
 * @brief 获取tick
 * 
 * @param NULL
 * @return 
 *      - time_ms
 * 
 *************************************************************/
static uint32_t get_tick_ms(void) 
{
    // 返回值是微秒 (int64_t)
    int64_t time_us = esp_timer_get_time();
    // 转换为毫秒
    uint32_t time_ms = (uint32_t)(time_us / 1000);
    return time_ms;
}

/**************************************************************
 * @brief lcd缓冲区刷新回调
 * 
 * @param[in] disp 显示设备
 * @param[in] area 显示区域
 * @param[in] px_buf 即将刷新进入的数据
 * @return NULL
 * 
 *************************************************************/
static void my_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_buf)
{
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(lcd_handle, 
                            area->x1, 
                            area->y1, 
                            area->x2 + 1, 
                            area->y2 + 1, 
                            px_buf));

    lv_display_flush_ready(disp);
}


/**************************************************************
 * @brief 显示亮度初始化
 * 
 * @param NULL
 * @return NULL
 * 
 *************************************************************/
static void display_brightness_init(void)
{
    ESP_LOGI(TAG, "Initializing brightness");

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
    ESP_LOGI(TAG, "Initializing LEDC for backlight pin: %d", LCD_LEDC);

    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));
}

/**************************************************************
 * @brief 显示亮度设置
 * 
 * @param[in] brightness_percentage  亮度值百分比
 * @return NULL
 * 
 *************************************************************/
static void display_brightness_set(int brightness_percentage)
{
    ESP_LOGI(TAG, "set brightness %d%%", brightness_percentage);
    if (brightness_percentage > 100)
    {
        brightness_percentage = 100;
    }    
    else if (brightness_percentage < 0)
    {
        brightness_percentage = 0;
    }
    ESP_LOGI(TAG, "Setting backlight to %d%%", brightness_percentage);

    // LEDC resolution set to 10bits, thus: 100% = 1023
    uint32_t duty_cycle = (1023 * brightness_percentage) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}


/**************************************************************
 * @brief ili9488初始化
 * 
 * @param NULL
 * @return NULL
 * 
 *************************************************************/
static void ili9488_init(void)
{
    ESP_LOGI(TAG, "Initializing ILI9488");

    //初始化spi总线
    spi_bus_config_t bus_cfg = {
        .miso_io_num = LCD_SPI_MISO,
        .mosi_io_num = LCD_SPI_MOSI,
        .sclk_io_num = LCD_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32768,
        .flags = 0, 
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
    // 将 LCD 连接到 SPI 总线
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_SPI_DC,
        .cs_gpio_num = LCD_SPI_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL, // 如果需要异步通知，可以设置回调
        .user_ctx = NULL
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &lcd_io_handle));

    //创建lcd面板
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
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9488(lcd_io_handle, &dev_cfg, LV_BUFFER_SIZE, &lcd_handle));

    //lcd面板初始化
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_handle, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(lcd_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(lcd_handle, 0, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_handle, true));

}

/**************************************************************
 * @brief lvgl初始化
 * 
 * @param NULL
 * @return NULL
 * 
 *************************************************************/
static void lvgl_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL");
    lv_init();
    //绑定tick回调
    lv_tick_set_cb(get_tick_ms);
    //创建显示设备
    lv_display_t * display = lv_display_create(DISPLAY_HORIZONTAL_PIXELS, DISPLAY_VERTICAL_PIXELS);
    //lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565_SWAPPED);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    //创建缓冲区
    lv_buf_1 = (lv_color_t *)heap_caps_malloc(LV_BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    if (!lv_buf_1) {
        ESP_LOGE(TAG, "Failed to allocate LVGL buffer");
        return;
    }
    lv_display_set_buffers(display, lv_buf_1, NULL, LV_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
    //绑定刷新回调
    lv_display_set_flush_cb(display, my_flush_cb);
    //创建组件
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x98FF98), LV_PART_MAIN);
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello LVGL");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0x004400), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}


void display_task(void *arg)
{
    //硬件初始化
    display_brightness_init();
    display_brightness_set(0);
    ili9488_init();
    //lvgl初始化
    lvgl_init();
    //设置亮度
    display_brightness_set(100);
    
    while(1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(30));
    }

}

