/**************************************************
 * @file    hal_spi.h
 * @brief   SPI 抽象层声明
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
#pragma once

#include "stdint.h"
#include "stdbool.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI 总线句柄前向声明
 */
typedef struct hal_spi_bus_s *hal_spi_bus_handle_t;

 /**
 * @brief SPI 设备句柄前向声明
 */
typedef struct hal_spi_dev_s *hal_spi_dev_handle_t;

/**
 * @brief SPI host
 */
typedef enum hal_spi_host_t {
    HAL_SPI1 = 0,
    HAL_SPI2 = 1,
    HAL_SPI3 = 2,
} hal_spi_host_t;

/**
 * @brief SPI DMA chan
 */
typedef enum hal_spi_dma_chan_t {
    HAL_DMA_DISABLE = 0,
    HAL_DMA_CHAN_AUTO = 3,
} hal_spi_dma_chan_t;

/**
 * @brief SPI 总线配置
 */
typedef struct hal_spi_bus_config_t {
    hal_spi_host_t      bus_host;
    hal_spi_dma_chan_t  dma_chan;
    int                 mosi_pin;
    int                 miso_pin;
    int                 sclk_pin;
    int                 max_trans_size;
} hal_spi_bus_config_t;

 
/**
 * @brief SPI 设备配置
 */
typedef struct hal_spi_dev_config_t {
    int cs_pin;
    int clk_speed_hz;
    uint8_t mode;
    int queue_size;
} hal_spi_dev_config_t;



/**
 * @brief SPI 总线初始化
 * 
 * @param[in] bus_cfg SPI 总线配置
 * @param[out] handle SPI 总线句柄
 * 
 * @return 
 *      - 0 初始化成功
 *      - 1 初始化失败
 * 
 */
uint8_t hal_spi_bus_init(const hal_spi_bus_config_t *cfg, hal_spi_bus_handle_t *handle);

/**
 * @brief SPI 总线反初始化
 * 
 * @param[out] handle SPI 总线句柄
 * 
 * @return 
 *      - 0 初始化成功
 *      - 1 初始化失败
 * 
 */
uint8_t hal_spi_bus_deinit(hal_spi_bus_handle_t handle);

/**
 * @brief SPI 设备初始化
 * 
 * @param[in] bus_cfg SPI 设备配置
 * @param[out] handle SPI 设备句柄
 * 
 * @return 
 *      - 0 初始化成功
 *      - 1 初始化失败
 * 
 */
uint8_t hal_spi_dev_init(hal_spi_bus_handle_t bus_handle, 
                        const hal_spi_dev_config_t *cfg, 
                        hal_spi_dev_handle_t *handle);

/**
 * @brief SPI 设备反初始化
 * 
 * @param[out] handle SPI 设备句柄
 * 
 * @return 
 *      - 0 初始化成功
 *      - 1 初始化失败
 * 
 */
uint8_t hal_spi_dev_deinit(hal_spi_dev_handle_t handle);

/**
 * @brief SPI 传输
 * 
 * @param[out] handle SPI 设备句柄
 * @param[in] tx_data 发送数据
 * @param[in] rx_data 接收数据
 * @param[in] tx_len 发送数据大小
 * @param[in] rx_len 接收数据大小
 * 
 * @return 
 *      - 0 初始化成功
 *      - 1 初始化失败
 * 
 */
uint8_t hal_spi_trans(hal_spi_dev_handle_t handle, 
                    uint8_t *tx_data, uint8_t *rx_data, 
                    uint32_t tx_len, uint32_t rx_len);


#ifdef __cplusplus
}
#endif