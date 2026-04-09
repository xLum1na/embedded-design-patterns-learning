#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp_driver_spi.h"
#include "bsp_ili9488.h"
#include "bsp_driver_gpio.h"
#include "bsp_driver_ledc.h"
#include "driver/gpio.h"

void app_main(void)
{

    while (1) {

        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
}
