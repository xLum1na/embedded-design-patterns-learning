#pragma once


#include "stdint.h"


#ifdef __cplusplus
extern "C" {
#endif

//定义数据类型枚举
typedef enum {
    DATA_TYPE_MPU6050 = 0,
    DATA_TYPE_TOUCH,
} data_type_t;

//定义传输的数据包结构
typedef struct {
    data_type_t type;   // 数据类型
    float roll;         // 横滚角
    float pitch;        // 俯仰角
} sensor_data_packet_t;

#ifdef __cplusplus
}
#endif
