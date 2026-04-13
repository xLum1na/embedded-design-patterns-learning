/**************************************************
 * @file    hal_gpio.h
 * @brief   GPIO 抽象层声明
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
#pragma once

#include "stdint.h"
#include "stdbool.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO 句柄前向声明
 */
typedef struct hal_gpio_s *hal_gpio_handle_t;

/**
 * @brief GPIO 模式配置
 */
typedef enum hal_gpio_mode_t {
    GPIO_INPUT = 0,
    GPIO_OUTPUT,
} hal_gpio_mode_t;

/**
 * @brief GPIO 设备配置
 */
typedef struct hal_gpio_config_t {
    const char *name;
    uint8_t pin;
    hal_gpio_mode_t mode;
    bool pull_down;
    bool pull_up;
} hal_gpio_config_t;

/**
 * @brief GPIO 外设初始化
 * 
 * @param[in] cfg GPIO 配置
 * @param[out] handle GPIO 句柄
 * 
 * @return 
 *      - 0 初始化失败
 *      - 1 初始化成功
 */
uint8_t hal_gpio_init(const hal_gpio_config_t *cfg, hal_gpio_handle_t *handle);

/**
 * @brief GPIO 外设反初始化
 * 
 * @param[out] handle GPIO 句柄
 * 
 * @return 
 *      - 0 反初始化失败
 *      - 1 反初始化成功
 */
uint8_t hal_gpio_deinit(hal_gpio_handle_t handle); 

/**
 * @brief GPIO 外设设置电平
 * 
 * @param[out] handle GPIO 句柄
 * 
 * @return 
 *      - 0 设置失败
 *      - 1 设置成功
 */
uint8_t hal_gpio_set_level(hal_gpio_handle_t handle, uint32_t level);








#ifdef __cplusplus
}
#endif
