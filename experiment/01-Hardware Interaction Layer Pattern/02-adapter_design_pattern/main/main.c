#include <stdio.h>
#include "mpu6050_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "display_serive.h"

TaskHandle_t display_handle = NULL;
void app_main(void)
{

    xTaskCreate(display_task, "display_task", 2048 * 5, NULL, 1, &display_handle);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }

}
