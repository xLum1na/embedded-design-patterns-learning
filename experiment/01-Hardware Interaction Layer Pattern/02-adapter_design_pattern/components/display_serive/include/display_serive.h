#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"

//LCD面板定义
#define DISPLAY_VERTICAL_PIXELS   320
#define DISPLAY_HORIZONTAL_PIXELS     480
#define LV_BUFFER_SIZE (DISPLAY_HORIZONTAL_PIXELS * 25)

//LCDspi定义
#define LCD_SPI_MISO        GPIO_NUM_12
#define LCD_SPI_MOSI        GPIO_NUM_13
#define LCD_SPI_SCLK        GPIO_NUM_14
#define LCD_SPI_DC          GPIO_NUM_15
#define LCD_SPI_CS          GPIO_NUM_16
#define LCD_RESET           GPIO_NUM_17
#define LCD_LEDC            GPIO_NUM_18
#define LCD_PIXEL_CLOCK_HZ  40000000
#define LCD_CMD_BITS        8
#define LCD_PARAM_BITS      8




void display_task(void *arg);




#ifdef __cplusplus
}
#endif
