/**************************************************
 * @file    bsp_driver_spi.h
 * @brief   板级spi外设驱动定义
 * @author  xLumina
 * @version V1.0.0
 * @date    2026-4-8
 *
 * @par History:
 *   - V1.0.0 | 2026-4-8 | xLumina | 初始版本创建
 *
 * @par Function List:
 *      - bsp_spi_init()        : 初始化spi外设
 * 
 * 
 **************************************************/
#pragma once


#include "stdint.h"
#include "stdbool.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif


/*************** #define ***************/
#define USE_SPI                 1               /* 使用spi */
#define USE_DMA                 0               /* 不使用dma */
#define BSP_SPI_HOST            SPI1_HOST       /* 使用SPI1 */
#define BSP_MOSI_PIN            GPIO_NUM_10     /* 主机输出从机输入引脚，ili9488与xpt2046共用 */
#define BSP_MISO_PIN            GPIO_NUM_11     /* 主机输入从机输出引脚，ili9488不使用，xpt2046使用 */
#define BSP_SCLK_PIN            GPIO_NUM_12     /* 时钟引脚，ili9488与xpt2046共用 */
#define BSP_ILI9488_CS_PIN      GPIO_NUM_13     /* ili9488片选 */
#define BSP_xpt2046_CS_PIN      GPIO_NUM_14     /* xpt2046设备片选 */
#define BSP_QUADWP_PIN          -1              /* 不需要写保护功能 */
#define BSP_QUADHD_PIN          -1              /* 不需要保存功能 */
#define BSP_MAX_TRAN_SIZE       4096            /* spi最传输尺寸 */  
#define DEV_ID_ILI9488          1               /* ili9488 设备ID */
#define DEV_ID_xpt2046          2               /* xpt2046 设备ID */    



/*************** #define queue ***************/
#if USE_DMA
    #define QUEUE_SIZE          8
    #define BSP_DMA_CHAN        SPI_DMA_CH_AUTO
#else
    #define QUEUE_SIZE  1
    #define BSP_DMA_CHAN        SPI_DMA_DISABLED
#endif


/**
 * @brief   初始化spi外设
 *
 * @param[in] ili9488_clk_speed ili9488 spi设备时钟频率
 * @param[in] xpt2046_clk_speed xpt2046 spi设备时钟频率
 * @param[in] mode spi设备传输模式
 *
 * @return NULL
 *
 * @note 板级外设初始化函数，由bsp.c调用
 */
void bsp_spi_init(int ili9488_clk_speed, int xpt2046_clk_speed, uint8_t mode);

/**
 * @brief   反初始化spi外设
 *
 * @param NULL
 *
 * @return NULL
 *
 * @note  板级外设初始化函数，由os调用，注入传感器驱动指针接口
 */
void bsp_spi_deinit(void);

/**
 * @brief   spi写字节
 *
 * @param[in] dev_id 需要调用的设备id
 * @param[in] data 写入数据地址
 *
 * @return
 *      - false 读取失败
 *      - true 读取成功
 *
 * @note  板级外设初始化函数，由bsp.c调用，注入传感器驱动指针接口
 */
bool bsp_spi_write_byte(uint8_t dev_id, uint8_t addr, uint8_t data);

/**
 * @brief   spi读取字节
 *
 * @param[in] dev_id 需要调用的设备id
 * @param[out] data 读取到的数据
 *
 * @return
 *      - false 读取失败
 *      - true 读取成功
 *
 * @note  板级外设初始化函数，由bsp.c调用，注入传感器驱动指针接口
 */
bool bsp_spi_read_byte(uint8_t dev_id, uint8_t addr, uint8_t *data);




#ifdef __cplusplus
}
#endif
