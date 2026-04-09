/**************************************************
 * @file    bsp_driver_spi.h
 * @brief   板级spi外设驱动声明
 * @author  xLumina
 * @version V1.0.0
 * @date    2026-4-8
 *
 * @par History:
 *   - V1.0.0 | 2026-4-8 | xLumina | 初始版本创建
 *
 * @par Function List:
 *      - bsp_spi_init()            : 初始化spi外设与资源
 *      - bsp_spi_deinit()          : 反初始化spi外设与资源
 *      - bsp_spi_ili9488_write()   : ILI9488 SPI 写入函数
 *      - bsp_spi_ili9488_read()    : ILI9488 SPI 读取函数
 *      - bsp_spi_xpt2046_write()   : XPT2046 SPI 写入函数
 *      - bsp_spi_xpt2046_read()    : XPT2046 SPI 读取函数
 * 
 **************************************************/
#pragma once


#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif


/*************** #define ***************/
#define USE_SPI                 1               /* 使用spi */
#define USE_DMA                 1               /* 不使用dma */
#define BSP_SPI_HOST            SPI1_HOST       /* 使用SPI1 */
#define BSP_MOSI_PIN            10              /* 主机输出从机输入引脚，ili9488与xpt2046共用 */
#define BSP_MISO_PIN            11              /* 主机输入从机输出引脚，ili9488不使用，xpt2046使用 */
#define BSP_SCLK_PIN            12              /* 时钟引脚，ili9488与xpt2046共用 */
#define BSP_ILI9488_CS_PIN      13              /* ili9488片选 */
#define BSP_xpt2046_CS_PIN      14              /* xpt2046设备片选 */
#define BSP_QUADWP_PIN          -1              /* 不需要写保护功能 */
#define BSP_QUADHD_PIN          -1              /* 不需要保存功能 */
#define BSP_MAX_TRAN_SIZE       4096            /* spi最传输尺寸 */  
#define DEV_ID_ILI9488          1               /* ili9488 设备ID */
#define DEV_ID_xpt2046          2               /* xpt2046 设备ID */ 
#define BSP_ILI9488_FREQ_HZ     20000000        /* ili9488时钟频率 */
#define BSP_XPT2046_FREQ_HZ     1000000         /* xpt2046时钟频率 */


/*************** #define DMA ***************/
#if USE_DMA
    #define QUEUE_SIZE              8
    #define BSP_DMA_CHAN            SPI_DMA_CH_AUTO
    #define DMA_TX_BUFFER_SIZE      4096
    #define DMA_RX_BUFFER_SIZE      1024
#else
    #define QUEUE_SIZE  1
    #define BSP_DMA_CHAN        SPI_DMA_DISABLED
#endif


/**
 * @brief   初始化spi外设
 *
 * @param NULL
 *
 * @return NULL
 *
 * @note 板级外设初始化函数，由bsp.c调用
 */
void bsp_spi_init(void);

/**
 * @brief   反初始化spi外设
 *
 * @param NULL
 *
 * @return NULL
 *
 * @note  板级外设初始化函数，由bsp.c调用
 */
void bsp_spi_deinit(void);

/**
 * @brief   ILI9488 SPI 写字节
 *
 * @param[in] data 写入数据缓冲区
 * @param[in] len 写入数据大小 byte
 * @param[in] timeout_ms 传输超时值
 * 
 * @return
 *      - false 读取失败
 *      - true 读取成功
 *
 * @note  ILI9488 SPI 写入函数，由上层驱动调用
 */
bool bsp_spi_ili9488_write(uint8_t *data, uint8_t len, int timeout_ms);

/**
 * @brief   ILI9488 SPI 读取字节
 *
 * @param[out] data 读取数据缓冲区
 * @param[in] len 读取数据大小 byte
 * @param[in] timeout_ms 传输超时值
 *
 * @return
 *      - false 读取失败
 *      - true 读取成功
 *
 * @note  ILI9488 SPI 读取函数，由上层驱动调用
 */
bool bsp_spi_ili9488_read(uint8_t *data, uint8_t len, int timeout_ms);

/**
 * @brief   XPT2046 SPI 写字节
 *
 * @param[in] data 写入数据缓冲区
 * @param[in] len 写入数据大小 byte
 * @param[in] timeout_ms 传输超时值
 * 
 * @return
 *      - false 读取失败
 *      - true 读取成功
 *
 * @note  XPT2046 SPI 写入函数，由上层驱动调用
 */
bool bsp_spi_xpt2046_write(uint8_t *data, uint8_t len, int timeout_ms);

/**
 * @brief   XPT2046 SPI 读取字节
 *
 * @param[out] data 读取数据缓冲区
 * @param[in] len 读取数据大小 byte
 * @param[in] timeout_ms 传输超时值
 *
 * @return
 *      - false 读取失败
 *      - true 读取成功
 *
 * @note  XPT2046 SPI 读取函数，由上层驱动调用
 */
bool bsp_spi_xpt2046_read(uint8_t *data, uint8_t len, int timeout_ms);


#ifdef __cplusplus
}
#endif
