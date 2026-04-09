/**************************************************
 * @file    bsp_driver_gpio.h
 * @brief   板级gpio外设驱动声明
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
#pragma once


#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif


/*************** #define ***************/
#define BSP_ILI9488_RESET_PIN           7   /* ILI9488 复位引脚 */
#define BSP_ILI9488_DC_PIN              6   /* ILI9488 数据命令传输选择线 */


 /**
 * @brief   初始化gpio外设
 *
 * @param NULL
 *
 * @return NULL
 *
 * @note 板级外设初始化函数，由bsp.c调用
 */
void bsp_gpio_init(void);

 /**
 * @brief   设置ILI9488 DC 电平
 *
 * @param[in] level 设置IO电平 (0: 低电平, 1: 高电平)
 *
 * @return 
 *      - true 设置成功
 *      - fasle 设置失败
 *
 * @note 设置 ILI9488 DC 引脚电平函数，由上层调用
 */
bool bsp_set_dc_level(uint32_t level);

 /**
 * @brief   设置ILI9488 RESET 电平
 *
 * @param[in] level 设置IO电平 (0: 低电平, 1: 高电平)
 *
 * @return 
 *      - true 设置成功
 *      - fasle 设置失败
 *
 * @note 设置 ILI9488 RESET 引脚电平函数，由上层调用
 */
bool bsp_set_reset_level(uint32_t level);


#ifdef __cplusplus
}
#endif
