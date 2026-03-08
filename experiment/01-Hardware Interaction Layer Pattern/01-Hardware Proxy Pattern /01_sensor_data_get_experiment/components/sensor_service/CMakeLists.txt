idf_component_register(SRCS "sensor_service.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver max30102_driver algorithm)
