#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor_service.h"
#include "esp_log.h"

static const char *TAG = "main";

void app_main(void)
{
    // 初始化传感器服务（不关心底层是 MAX30102 还是其他芯片）
    esp_err_t ret = sensor_service_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Sensor service init failed");
        return;
    }

    sensor_data_t data;
    
    while (1) {
        // 获取通用传感器数据（不关心 IR/Red 原始值）
        ret = sensor_service_get_data(&data);
        
        if (ret == ESP_OK && data.is_valid) {
            printf("Heart Rate: %d bpm, SpO2: %d%%\n", 
                   data.heart_rate_bpm, 
                   data.spo2_percent);
        } else {
            printf("No valid data\n");
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));   // 10Hz 采样
    }
}