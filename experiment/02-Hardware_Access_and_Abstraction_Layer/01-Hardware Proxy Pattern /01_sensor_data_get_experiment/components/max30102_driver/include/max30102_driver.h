#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"

// I2C 配置
#define MAX_I2C_SCL             GPIO_NUM_4
#define MAX_I2C_SDA             GPIO_NUM_5
#define MAX30102_ADDR           0x57

// 寄存器地址定义
#define REG_INTR_STATUS_1       0x00
#define REG_INTR_STATUS_2       0x01
#define REG_INTR_ENABLE_1       0x02
#define REG_INTR_ENABLE_2       0x03
#define REG_FIFO_WR_PTR         0x04
#define REG_OVF_COUNTER         0x05
#define REG_FIFO_RD_PTR         0x06
#define REG_FIFO_DATA           0x07
#define REG_FIFO_CONFIG         0x08 
#define REG_MODE_CONFIG         0x09
#define REG_SPO2_CONFIG         0x0A
#define REG_LED1_PA             0x0C
#define REG_LED2_PA             0x0D
#define REG_PILOT_PA            0x10
#define REG_TEMP_INT            0x1F
#define REG_TEMP_FRAC           0x20
#define REG_TEMP_CONFIG         0x21
#define REG_PART_ID             0xFF

#define SAMPLES_PER_SECOND 		100000	//检测频率


/***********************************************************
 * @brief 设备原始数据存储数据结构
 **********************************************************/
typedef struct {
    uint32_t red;
    uint32_t ir;
} max30102_raw_data_t;

/***********************************************************
 * @brief 初始化max30102设备
 * 
 * @param void
 * @return  
 *      - ESP_OK
 *      - ESP_ERR_INVALID_ARG
 *      - ESP_ERR_NO_MEM
 * 
 **********************************************************/
esp_err_t driver_max30102_init(void);

/***********************************************************
 * @brief 获取设备原始数据
 * 
 * @param[out] out_raw 获取到的原始数据
 * @return  
 *      - ESP_OK
 *      - ESP_ERR_INVALID_ARG
 *      - ESP_ERR_NO_MEM
 * 
 **********************************************************/
esp_err_t driver_max30102_read_raw(max30102_raw_data_t *out_raw);

