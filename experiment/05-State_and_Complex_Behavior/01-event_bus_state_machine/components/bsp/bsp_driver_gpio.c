/**************************************************
 * @file    bsp_driver_gpio.c
 * @brief   板级gpio外设驱动实现
 * @author  xLumina
 * @version V1.0.0
 * @date    2026-4-9
 *
 * @par History:
 *   - V1.0.0 | 2026-4-9 | xLumina | 初始版本创建
 *
 * @par Function List:
 *      - bsp_gpio_init()           : gpio外设初始化
 *      - bsp_set_dc_level()        : gpio设置 ILI9488 DC 电平
 *      - bsp_set_reset_level()     : gpio设置 ILI9488 RESET 电平
 * 
 **************************************************/


/*=============== #include "bsp_driver_gpio.h" ===============*/
#include "bsp_driver_gpio.h"
/*=============== #include "driver/gpio.h" ===============*/
#include "driver/gpio.h"
/*=============== #include "esp_log.h" ===============*/
#include "esp_log.h"


/*************** gpio 日志标志 ***************/
static const char *TAG = "bsp_driver_gpio";


void bsp_gpio_init(void)
{
    esp_err_t ret;
    /* 初始化gpio */
    /* 配置ILI9488 DC 引脚 与 RESET 引脚*/
    gpio_config_t io_cfg = {0};
    io_cfg.pin_bit_mask     = (1ULL << BSP_ILI9488_DC_PIN) | (1ULL << BSP_ILI9488_RESET_PIN);
    io_cfg.mode             = GPIO_MODE_OUTPUT;
    io_cfg.pull_down_en     = GPIO_PULLDOWN_DISABLE;
    io_cfg.pull_up_en       = GPIO_PULLUP_DISABLE;
    io_cfg.intr_type        = GPIO_INTR_DISABLE;
    ret = gpio_config(&io_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio config failed: %x", ret);
        return ;
    }
    ESP_LOGI(TAG, "gpio initialized: DC=%d, RESET=%d", 
            BSP_ILI9488_DC_PIN, BSP_ILI9488_RESET_PIN);
    return ;
}

bool bsp_set_dc_level(uint32_t level)
{
    esp_err_t ret;
    //ESP_LOGI(TAG, "gpio dc test");
    ret = gpio_set_level(BSP_ILI9488_DC_PIN, level);
    if (ret != ESP_OK) {
        return false;
    }
    return true;
}


bool bsp_set_reset_level(uint32_t level)
{
    esp_err_t ret;
    ret = gpio_set_level(BSP_ILI9488_RESET_PIN, level);
    if (ret != ESP_OK) {
        return false;
    }
    return true;
}