#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/***********************************************************
 * @brief 代理转换传感器原始数据后最终数据
 **********************************************************/
typedef struct sensor_data_t {
    uint16_t heart_rate_bpm;   // 最终心率
    uint16_t spo2_percent;     // 最终血氧
    bool is_valid;        // 数据是否可信
} sensor_data_t;

/***********************************************************
 * 环形缓冲区数据结构定义
 **********************************************************/
typedef struct ringbuffer_t {
    uint8_t *buffer;        //缓冲区存储空间
    size_t buffer_size;     //缓冲区存储空间大小
    size_t element_size;    //单个存储元素大小
    size_t *write_index;    //写指针
    size_t *read_index;     //读指针
    size_t count;           //储存计数
}ringbuffer_t;


/***********************************************************
 * @brief 初始化服务
 * 
 * @param NULL
 * @return 
 *      - ESP_OK
 *      - ESP_FAIL
 * 
 **********************************************************/
// 初始化服务
esp_err_t sensor_service_init(void);

/***********************************************************
 * @brief 获取最终数据
 * 
 * @param[out] out_data 最终数据
 * @return 
 *      - ESP_OK
 *      - ESP_FAIL
 * 
 **********************************************************/
esp_err_t sensor_service_get_data(sensor_data_t *out_data);
