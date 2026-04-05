#include "serive_touch.h"
#include "driver_touch.h"
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <driver/spi_master.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "string.h"
#include "math.h"
#include "lvgl.h"

static const char *TAG = "TOUCH";

static esp_lcd_panel_io_handle_t touch_io_handle = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;
static int hh = 0;

//屏幕触摸中断服务程序
static void IRAM_ATTR touch_isr_handler(void* arg)
{
    hh += 1;
}


static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    //默认状态：未按下
    data->state = LV_INDEV_STATE_RELEASED;
    //基础检查
    if (touch_io_handle == NULL || touch_handle == NULL) {
        return;
    }
    //读取底层数据
    esp_err_t read_ret = esp_lcd_touch_read_data(touch_handle);
    if (read_ret != ESP_OK) {
        return;
    }
    //获取坐标
    uint16_t x_arr[1];
    uint16_t y_arr[1];
    uint8_t touch_cnt = 0;
    bool pressed = esp_lcd_touch_get_coordinates(
        touch_handle, 
        x_arr, 
        y_arr, 
        NULL, 
        &touch_cnt, 
        1
    );
    //处理有效触摸
    if (pressed && touch_cnt > 0) {
        uint16_t raw_x = x_arr[0];
        uint16_t raw_y = y_arr[0];
        //边界检查 (防止非法坐标导致 LVGL 崩溃)
        if (raw_x >= DISPLAY_HORIZONTAL_PIXELS || raw_y >= DISPLAY_VERTICAL_PIXELS) {
            return;
        }
        //更新 LVGL 输入数据并显示
        ESP_LOGI("TOUCH_STATUS","Pressed:%s | Count:%d | Raw:X=%d Y=%d", 
            pressed ? "YES" : "NO", 
            touch_cnt, 
            x_arr[0], 
            y_arr[0]);
        data->point.x = raw_x;
        data->point.y = raw_y;
        data->state = LV_INDEV_STATE_PRESSED;
    }
}

// 触摸初始化
static void touch_init(void)
{
    //创建gpio中断
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << LCD_TOUCH_IRQ,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,         
    };
    gpio_config(&io_conf);
    //注册中断服务
    gpio_install_isr_service(0);
    gpio_isr_handler_add(LCD_TOUCH_IRQ, touch_isr_handler, NULL);

    // 1. 添加 SPI 设备
    esp_lcd_panel_io_spi_config_t touch_io_config = {
        .dc_gpio_num = GPIO_NUM_NC, // XPT2046 不需要 DC
        .cs_gpio_num = LCD_TOUCH_SPI_CS,
        .pclk_hz = ESP_LCD_TOUCH_SPI_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0, // XPT2046 通常是 Mode 0
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL, 
        .user_ctx = NULL
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &touch_io_config, &touch_io_handle));

    // 2. 创建触摸设备
    esp_lcd_touch_config_t touch_cfg = {
        .x_max = DISPLAY_HORIZONTAL_PIXELS, // 320
        .y_max = DISPLAY_VERTICAL_PIXELS,   // 480
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC, // 如果有中断引脚，建议连接
        .levels = { .reset = 0, .interrupt = 0 },
        .flags = { 
            // 【修正】方向配置
            // 如果你的屏幕是竖屏 (swap_xy=true)，触摸通常也要 swap_xy=true
            // 如果点不准，请尝试修改这里的 swap_xy 和 mirror_x
            .swap_xy = 0,   // 尝试改为 1
            .mirror_x = 0,  // 尝试改为 0 或 1
            .mirror_y = 0, 
        },
    };

    // 3. 初始化 XPT2046 驱动
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(touch_io_handle, &touch_cfg, &touch_handle));

    // 4. 绑定 LVGL 输入设备
    if (touch_handle != NULL) {
        lv_indev_t * indev = lv_indev_create();
        if (indev) {
            lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
            lv_indev_set_read_cb(indev, touchpad_read);
        }
    } 
}

static void touch_deinit(void)
{
    if (touch_handle != NULL) {
        esp_lcd_touch_del(touch_handle);
        touch_handle = NULL;
    }
    if (touch_io_handle != NULL) {
        esp_lcd_panel_io_del(touch_io_handle);
        touch_io_handle = NULL;
    }
}

void touch_test_task(void *arg)
{
    touch_init();
    // 任务不需要做任何事，因为 LVGL 会在自己的任务中通过回调读取触摸
    while (1) {
        printf("%d\n", hh);
        vTaskDelay(pdMS_TO_TICKS(1000)); // 降低 CPU 占用
    }
}