#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp_driver_spi.h"
#include "bsp_ili9488.h"
#include "bsp_driver_gpio.h"
#include "bsp_driver_ledc.h"
#include "driver/gpio.h"

#include <esp_lcd_panel_interface.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_commands.h>


void time_delay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}


void app_main(void)
{
    bsp_ili9488_handle_t handle = NULL;
    bsp_ili9488_config_t cfg = {0};
    cfg.color = ILI9488_COLOR_RGB565;
    cfg.dir = ILI9488_DIR_0;
    cfg.scan_dir = ILI9488_SCAN_LTR_TTB;
    cfg.fps = ILI9488_FPS_30;
    cfg.user_data = NULL;

    // 配置依赖注入接口
    spi_driver_t spi_if = {0};
    spi_if.spi_read_func = bsp_spi_ili9488_read;
    spi_if.spi_write_func = bsp_spi_ili9488_write;
    
    ledc_driver_t ledc_if = {0};
    ledc_if.ledc_set_duty_func = bsp_ledc_set_duty;
    
    gpio_driver_t gpio_if = {0};
    gpio_if.gpio_set_dc_func = bsp_set_dc_level;
    gpio_if.gpio_set_reset_func = bsp_set_reset_level;
    
    sys_time_t time_if = {0};
    time_if.sys_set_delay_func = time_delay;

    // 2. 初始化顺序：底层硬件 -> 上层设备
    printf("Initializing Peripherals...\n");
    bsp_gpio_init();
    bsp_spi_init();
    bsp_ledc_init();
    
    // 3. 创建并初始化屏幕对象
    ili9488_err_t ret = ili9488_create(&cfg, &spi_if, &ledc_if, &gpio_if, &time_if, &handle);
    if (ret != ILI9488_OK || handle == NULL) {
        printf("ILI9488 Create Failed: %d\n", ret);
        return;
    }

    ret = bsp_ili9488_init(handle);
    if (ret != ILI9488_OK) {
        printf("ILI9488 Init Failed: %d\n", ret);
        ili9488_delete(&handle);
        return;
    }

    printf("ILI9488 Initialized Successfully!\n");

    // 4. 点亮背光 (可选，如果你的背光默认是灭的)
    bsp_ledc_set_duty(80); // 设置80%亮度
    bsp_ili9488_setaddrwindow(handle, 0, 0, 320, 480);
    // 5. 主循环
    while (1) {
        // 改为全屏填充红色
        bsp_ili9488_fillscreen(handle, 0xF800); // RGB565 红色
        vTaskDelay(pdMS_TO_TICKS(1000));
        // 改为全屏填充绿色
        bsp_ili9488_fillscreen(handle, 0x07E0); // RGB565 绿色
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}
