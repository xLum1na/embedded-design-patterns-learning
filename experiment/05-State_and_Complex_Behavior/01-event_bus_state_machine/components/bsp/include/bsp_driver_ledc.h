#pragma once


#include "stdint.h"
#include "stdbool.h"


#ifdef __cplusplus
extern "C" {
#endif



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
 * @brief   初始化ledc外设
 *
 * @param[in] 设置占空比
 *
 * @return
 *      - false 读取失败
 *      - true 读取成功
 *
 * @note 板级外设初始化函数，由bsp.c调用，注入传感器驱动指针接口
 */
bool bsp_ledc_set_duty(uint8_t duty);


#ifdef __cplusplus
}
#endif
