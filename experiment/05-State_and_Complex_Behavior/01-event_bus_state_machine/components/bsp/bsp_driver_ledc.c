/**************************************************
 * @file    bsp_driver_ledc.c
 * @brief   板级ledc外设定义
 * @author  xLumina
 * @version V1.0.0
 * @date    2026-4-9
 *
 * @par History:
 *   - V1.0.0 | 2026-4-9 | xLumina | 初始版本创建
 *
 * @par Function List:
 *      - bsp_ledc_init()           : 初始化ledc外设
 *      - bsp_ledc_set_duty()       : ledc设置占空比
 * 
 **************************************************/

/*=============== "bsp_driver_ledc.h" ===============*/
#include "bsp_driver_ledc.h"
/*=============== #include "driver/ledc.h" ===============*/
#include "driver/ledc.h"
/*=============== #include "esp_log.h" ===============*/
#include "esp_log.h"  

/*************** ledc日志标志 ***************/
static const char *TAG = "bsp_driver_ledc";


void bsp_ledc_init(void)
{
    esp_err_t ret;
    /* ledc 定时器配置*/
    ledc_timer_config_t timer_cfg = {0};
    timer_cfg.timer_num         = BSP_LEDC_TIMER;
    timer_cfg.duty_resolution   = BSP_LEDC_DUTY_RES;
    timer_cfg.speed_mode        = BSP_LEDC_SPEED_MODE;
    timer_cfg.freq_hz           = BSP_LEDC_FREQ_HZ;
    timer_cfg.clk_cfg           = LEDC_AUTO_CLK;
    ret = ledc_timer_config(&timer_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc timer config failed: %x", ret);
        return ;
    }
    /* ledc 通道配置*/
    ledc_channel_config_t chan_cfg = {0};
    chan_cfg.speed_mode = BSP_LEDC_SPEED_MODE;
    chan_cfg.channel = BSP_LEDC_CHAN;
    chan_cfg.timer_sel = BSP_LEDC_TIMER;
    chan_cfg.intr_type = LEDC_INTR_DISABLE;
    chan_cfg.gpio_num = BSP_LEDC_PIN;
    chan_cfg.duty = 0;
    chan_cfg.hpoint = 0;
    ret = ledc_channel_config(&chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc channel config failed: %x", ret);
        return ;
    }
    ESP_LOGI(TAG, "LEDC initialized (Pin: %d, Freq: %dHz)", BSP_LEDC_PIN, BSP_LEDC_FREQ_HZ);
    return ;
}


bool bsp_ledc_set_duty(uint8_t duty)
{
    if (duty > 100) {
        ESP_LOGE(TAG, "Invalid duty value: %d", duty);
        return false;
    }
    /* 占空比简单线性缩放 */
    uint32_t max_duty = (1 << BSP_LEDC_DUTY_RES) - 1;      /* 8196 */
    uint32_t du = (uint32_t)duty * max_duty / 100;
    esp_err_t ret;
    /* 设置分辨率 */
    ret = ledc_set_duty(BSP_LEDC_SPEED_MODE, BSP_LEDC_CHAN, du);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc set duty failed: %x", ret);
        return false;
    }
    ret = ledc_update_duty(BSP_LEDC_SPEED_MODE, BSP_LEDC_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc update duty failed: %x", ret);
        return false;
    }
    return true;
}
