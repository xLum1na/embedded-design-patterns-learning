idf_component_register(SRCS "display_serive.c"
                    INCLUDE_DIRS "include"
                    REQUIRES ili9488_driver lvgl__lvgl common_defs)
