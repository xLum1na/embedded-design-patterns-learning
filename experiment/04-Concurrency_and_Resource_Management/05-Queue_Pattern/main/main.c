#include <stdio.h>
#include "mpu6050_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "display_serive.h"
#include "sensor_serive.h"
#include "common_defs.h"

//任务句柄
TaskHandle_t display_handle = NULL;
TaskHandle_t sensor_handle = NULL;

//队列资源
QueueHandle_t xDisplayQueue = NULL;


void app_main(void)
{
    //创建队列
    xDisplayQueue = xQueueCreate(10, sizeof(sensor_data_packet_t));
    if (xDisplayQueue == NULL) {
        ESP_LOGE("MAIN", "Queue creation failed");
        return;
    }
    
    xTaskCreate(sensor_task, "sensor_task", 2048 * 2, (void *)xDisplayQueue, 3, &sensor_handle);
    xTaskCreate(display_task, "display_task", 2048 * 5, (void *)xDisplayQueue, 5, &display_handle);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }

}
