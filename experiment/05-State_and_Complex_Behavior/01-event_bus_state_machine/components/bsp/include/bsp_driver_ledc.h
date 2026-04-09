/**************************************************
 * @file    bsp_driver_ledc.h
 * @brief   板级ledc外设声明
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
#pragma once


#include "stdint.h"
#include "stdbool.h"


#ifdef __cplusplus
extern "C" {
#endif


/*************** #define ***************/
#define BSP_LEDC_PIN            5                       /* ledc 硬件引脚 */ 
#define BSP_LEDC_TIMER          LEDC_TIMER_0            /* ledc 定时器 */
#define BSP_LEDC_SPEED_MODE     LEDC_LOW_SPEED_MODE     /* ledc 速度模式 */
#define BSP_LEDC_FREQ_HZ        4000                    /* ledc 频率 */
#define BSP_LEDC_CLK            LEDC_AUTO_CLK           /* ledc 时钟源 */
#define BSP_LEDC_DUTY_RES       LEDC_TIMER_13_BIT       /* ledc 占空比分辨率*/
#define BSP_LEDC_CHAN           LEDC_CHANNEL_0          /* ledc 通道*/


/**
 * @brief   初始化ledc外设
 *
 * @param NULL
 *
 * @return NULL
 *
 * @note 板级外设初始化函数，由bsp.c调用
 */
void bsp_ledc_init(void);

/**
 * @brief   设置ledc占空比
 *
 * @param[in] duty 设置占空比 0--100
 *
 * @return
 *      - false 设置失败
 *      - true 设置成功
 *
 * @note 板级外设初始化函数，由上层调用
 */
bool bsp_ledc_set_duty(uint8_t duty);


#ifdef __cplusplus
}
#endif
