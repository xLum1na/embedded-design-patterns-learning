idf_component_register(SRCS "serive_hsm.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_timer event_bus driver_fp)
