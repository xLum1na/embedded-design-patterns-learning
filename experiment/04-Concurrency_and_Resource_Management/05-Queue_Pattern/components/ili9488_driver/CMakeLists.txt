idf_component_register(SRCS "ili9488_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_driver_ledc esp_lcd esp_timer esp_lcd_touch)
