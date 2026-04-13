/**************************************************
 * @file    hal_spi.c
 * @brief   SPI 抽象层实现
 * @author  xLumina
 * @version V1.0.0
 * @date    2026-4-13
 *
 * @par History:
 *   - V1.0.0 | 2026-4-13 | xLumina | 初始版本创建
 *
 * @par Function List:
 * 
 **************************************************/

#include "hal_spi.h"
#include "driver/spi_master.h"
#include "esp_log.h"

static const char *TAG = "hal_spi";


/**
 * @brief SPI 总线句柄
 */
typedef struct hal_spi_bus_s {
    hal_spi_bus_config_t    cfg;
    bool                    is_inited;
} hal_spi_bus_s;

/**
 * @brief SPI 设备句柄
 */
typedef struct hal_spi_dev_s {
    hal_spi_dev_config_t    cfg;
    spi_device_handle_t     spi_handle;
    bool                    is_inited;
} hal_spi_dev_s;



uint8_t hal_spi_bus_init(const hal_spi_bus_config_t *cfg, hal_spi_bus_handle_t *handle)
{
    if (cfg == NULL || handle == NULL) {
        ESP_LOGW(TAG, "Init spi bus init fail:%d", ESP_ERR_INVALID_ARG);
        return 0;
    }
    hal_spi_bus_handle_t ret_handle = malloc(sizeof(hal_spi_bus_s));
    if (ret_handle == NULL) {
        ESP_LOGE(TAG, "Malloc fail");
        return 0;
    }
    esp_err_t ret;
    spi_bus_config_t bus_cfg = {0};
    bus_cfg.miso_io_num = cfg->miso_pin;
    bus_cfg.mosi_io_num = cfg->mosi_pin;
    bus_cfg.sclk_io_num = cfg->sclk_pin;
    if (cfg->max_trans_size < 0 || cfg->max_trans_size > 4096) {
        ESP_LOGE(TAG, "Size err, [size=%d]", cfg->max_trans_size);
        free(ret_handle);
        return 0;
    }
    bus_cfg.max_transfer_sz = cfg->max_trans_size;
    ret = spi_bus_initialize(cfg->bus_host, &bus_cfg, cfg->dma_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Init spi bus fail %d", ret);
        free(ret_handle);
        return 0;
    }
    ret_handle->cfg = *cfg;
    ret_handle->is_inited = true;
    *handle = ret_handle;
    ESP_LOGE(TAG, "Init spi bus success,[MOSI=%d],[MISO=%d],[SCLK=%d]",
            cfg->mosi_pin,cfg->miso_pin, cfg->sclk_pin);
    return 1;
}

uint8_t hal_spi_bus_deinit(hal_spi_bus_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Deinit spi bus fail:%d", ESP_ERR_INVALID_ARG);
        return 0;
    }
    esp_err_t ret;
    ret = spi_bus_free(handle->cfg.bus_host);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Denit spi bus fail %d", ret);
        return 0;
    }
    free(handle);
    return 1;
}

uint8_t hal_spi_dev_init(hal_spi_bus_handle_t bus_handle, 
                        const hal_spi_dev_config_t *cfg, 
                        hal_spi_dev_handle_t *handle)
{
    if (bus_handle == NULL || cfg == NULL || handle == NULL) {
        ESP_LOGW(TAG, "Init spi dev init fail:%d", ESP_ERR_INVALID_ARG);
        return 0;
    }
    if (!bus_handle->is_inited) {
        ESP_LOGE(TAG, "Not init spi bus:%d", ESP_ERR_INVALID_ARG);
        return 0;
    }
    hal_spi_dev_handle_t ret_handle = malloc(sizeof(hal_spi_dev_s));
    if (ret_handle == NULL) {
        ESP_LOGE(TAG, "Malloc fail");
        return 0;
    }
    esp_err_t ret;
    spi_device_interface_config_t dev_cfg = {0};
    dev_cfg.spics_io_num = cfg->cs_pin;
    dev_cfg.clock_speed_hz = cfg->clk_speed_hz;
    dev_cfg.mode = cfg->mode;
    dev_cfg.queue_size = cfg->queue_size;
    ret = spi_bus_add_device(bus_handle->cfg.bus_host, &dev_cfg, ret_handle->spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Init spi dev fail:%d", ret);
        free(ret_handle);
        return 0;
    }

    ret_handle->cfg = *cfg;
    ret_handle->is_inited = true;
    *handle = ret_handle;
    ESP_LOGE(TAG, "Init add spi dev success,[CS=%d],[FREQ=%d],[mode=%d],[QUEUESIZE=%d]",
            cfg->cs_pin, cfg->clk_speed_hz, cfg->mode, cfg->queue_size);
    return 1;
}

uint8_t hal_spi_dev_deinit(hal_spi_dev_handle_t handle)
{
    

}