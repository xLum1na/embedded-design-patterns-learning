idf_component_register(SRCS "fp_hsm.c" "serive_hsm.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver event esp_timer driver_fingerprint)
