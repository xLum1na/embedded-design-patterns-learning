idf_component_register(SRCS "bsfp_drv_spi.c" "bsp_driver_gpio.c" "bsp_ili9488.c" "bsp_xpt2046.c" "bsp_driver_ledc.c" "bsp_driver_spi.c" "bsp.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_timer esp_driver_ledc)
