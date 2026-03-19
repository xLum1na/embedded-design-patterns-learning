#pragma once 

#include "driver/gpio.h"


#ifdef __cplusplus
extern "C" {
#endif

#define MPU6050_I2C_PORT    I2C_NUM_0
#define MPU6050_I2C_SDA     GPIO_NUM_4
#define MPU6050_I2C_SCL     GPIO_NUM_5
#define MPU6050_I2C_FREQ    100000




void sensor_task(void *arg);

#ifdef __cplusplus
}
#endif