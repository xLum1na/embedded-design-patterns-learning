
/**************************************************
 * @file    hal_gpio.c
 * @brief   GPIO 抽象层实现
 * @author  xLumina
 * @version V1.0.0
 * @date    2026-4-13
 *
 * @par History:
 *   - V1.0.0 | 2026-4-13 | xLumina | 初始版本创建
 *
 * @par Function List:
 *      - hal_gpio_init() GPIO 外设初始化
 *      - hal_gpio_deinit() GPIO 外设反初始化
 *      - hal_gpio_set_level() GPIO 设置电平
 * 
 **************************************************/

/*=============== #include "hal_gpio.h" ===============*/
#include "hal_gpio.h"
/*=============== #include "driver/gpio.h"===============*/
#include "driver/gpio.h"
/*=============== #include "esp_log.h" ===============*/
#include "esp_log.h"

/*************** 日志 ***************/
static const char *TAG = "hal_gpio";


/**
 * @brief GPIO 设备句柄
 */
typedef struct hal_gpio_s {
    hal_gpio_config_t cfg;
    bool is_init;
}hal_gpio_s;

uint8_t hal_gpio_init(const hal_gpio_config_t *cfg, hal_gpio_handle_t *handle)
{
    if (cfg == NULL || handle == NULL) {
        ESP_LOGW(TAG, "Init fail: %d", ESP_ERR_INVALID_ARG);
        return 0;
    }
    esp_err_t ret;
    hal_gpio_handle_t ret_handle = malloc(sizeof(hal_gpio_s));
    if (ret_handle == NULL) {
        ESP_LOGE(TAG, "Malloc fail");
        return 0;
    }

    gpio_config_t io_cfg = {0};
    if (cfg->pin > GPIO_NUM_MAX) {
        ESP_LOGE(TAG, "Invalid pin number: %d", cfg->pin);
        return 0;
    }
    io_cfg.pin_bit_mask = 1ULL << cfg->pin;
    switch (cfg->mode) {
        case GPIO_INPUT:
            io_cfg.mode = GPIO_MODE_INPUT;
            break;
        case GPIO_OUTPUT:
            io_cfg.mode = GPIO_MODE_OUTPUT;
            break;
        default:
            ESP_LOGE(TAG, "Invalid GPIO mode");
            free(ret_handle);
            return 0;
            break;
    }
    io_cfg.pull_down_en = cfg->pull_down ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    io_cfg.pull_up_en = cfg->pull_up ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    ret = gpio_config(&io_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Init fail: %d", ret);
        free(ret_handle);
        return 0;
    }
    ret_handle->cfg = *cfg;
    ret_handle->is_init = true;
    *handle = ret_handle;
    ESP_LOGI(TAG, "Init [%s] gpio success", cfg->name);
    return 1;
}

uint8_t hal_gpio_deinit(hal_gpio_handle_t handle) 
{
    if (handle == NULL) {
        ESP_LOGW(TAG, "Denit fail: %d", ESP_ERR_INVALID_ARG);
        return 0;
    }
    gpio_reset_pin(handle->cfg.pin);
    free(handle);
    return 1;
}

uint8_t hal_gpio_set_level(hal_gpio_handle_t handle, uint32_t level)
{
    if (handle == NULL || !handle->is_init) {
        ESP_LOGW(TAG, "set level fail");
        return 0;
    }
    esp_err_t ret;
    int lvl = (level != 0) ? 1 : 0;
    ret = gpio_set_level(handle->cfg.pin, lvl);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "set [%s] level fail:", handle->cfg.name);
        return 0;
    }
    ESP_LOGI(TAG, "Set [%s] gpio level [%d] success", handle->cfg.name, lvl);
    return 1;
}

