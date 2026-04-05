idf_component_register(SRCS "esp_lcd_touch.c" "driver_touch.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_driver_ledc esp_lcd esp_timer)
