#pragma once

#include "esp_lcd_panel_vendor.h"
#include "esp_idf_version.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_panel_io.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_XPT2046_Z_THRESHOLD 400
#define CONFIG_XPT2046_CONVERT_ADC_TO_COORDS 1

esp_err_t esp_lcd_touch_new_spi_xpt2046(const esp_lcd_panel_io_handle_t io,
                                        const esp_lcd_touch_config_t *config,
                                        esp_lcd_touch_handle_t *out_touch);

esp_err_t esp_lcd_touch_xpt2046_read_battery_level(const esp_lcd_touch_handle_t handle, float *out_level);

esp_err_t esp_lcd_touch_xpt2046_read_aux_level(const esp_lcd_touch_handle_t handle, float *out_level);

esp_err_t esp_lcd_touch_xpt2046_read_temp0_level(const esp_lcd_touch_handle_t handle, float *output);

esp_err_t esp_lcd_touch_xpt2046_read_temp1_level(const esp_lcd_touch_handle_t handle, float *output);


#ifdef __cplusplus
}
#endif
