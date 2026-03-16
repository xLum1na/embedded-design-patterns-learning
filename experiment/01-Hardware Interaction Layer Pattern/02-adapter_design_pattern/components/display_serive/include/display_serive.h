#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"


#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (10 * 1000 * 1000)
#define EXAMPLE_LCD_I80_BUS_WIDTH 8
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_DATA0          1
#define EXAMPLE_PIN_NUM_DATA1          2
#define EXAMPLE_PIN_NUM_DATA2          3
#define EXAMPLE_PIN_NUM_DATA3          4
#define EXAMPLE_PIN_NUM_DATA4          6
#define EXAMPLE_PIN_NUM_DATA5          7
#define EXAMPLE_PIN_NUM_DATA6          8
#define EXAMPLE_PIN_NUM_DATA7          9
#define EXAMPLE_PIN_NUM_PCLK           5
#define EXAMPLE_PIN_NUM_CS             41
#define EXAMPLE_PIN_NUM_DC             42
#define EXAMPLE_PIN_NUM_RST            47
#define EXAMPLE_PIN_NUM_BK_LIGHT       48
// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              320
#define EXAMPLE_LCD_V_RES              480
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_LVGL_TICK_PERIOD_MS    2

#ifdef __cplusplus
}
#endif
