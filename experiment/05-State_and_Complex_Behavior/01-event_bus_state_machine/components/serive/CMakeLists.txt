idf_component_register(SRCS "serive_ui.c" "serive_touch.c" "serive.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver lvgl__lvgl esp_timer driver_touch event_bus driver_display)
