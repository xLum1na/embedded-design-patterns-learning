#pragma once

#include "stdint.h"
#include "stdbool.h"
#include "event_bus.h"


#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_VERTICAL_PIXELS     480
#define DISPLAY_HORIZONTAL_PIXELS   320

#define LCD_TOUCH_SPI_MISO          GPIO_NUM_41
#define LCD_TOUCH_SPI_MOSI          GPIO_NUM_13
#define LCD_TOUCH_SPI_CS            GPIO_NUM_40
#define LCD_TOUCH_IRQ               GPIO_NUM_42
#define ESP_LCD_TOUCH_SPI_CLOCK_HZ  10000
#define LCD_CMD_BITS                8
#define LCD_PARAM_BITS              8

void touch_test_task(void *arg);

#ifdef __cplusplus
}
#endif
