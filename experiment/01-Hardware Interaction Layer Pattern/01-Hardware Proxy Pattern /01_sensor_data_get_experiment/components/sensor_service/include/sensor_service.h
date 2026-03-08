#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// 【服务层通用结构】
// 上层应用只关心最终结果，不关心 IR/Red 原始值或中间变量
typedef struct {
    int heart_rate_bpm;   // 最终心率
    int spo2_percent;     // 最终血氧
    bool is_valid;        // 数据是否可信
} sensor_data_t;

// 初始化服务
esp_err_t sensor_service_init(void);

// 获取通用数据
// 注意：返回的是 sensor_data_t，而不是 max30102_raw_data_t
esp_err_t sensor_service_get_data(sensor_data_t *out_data);