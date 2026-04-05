idf_component_register(SRCS "driver_display.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_timer esp_driver_ledc esp_lcd)
