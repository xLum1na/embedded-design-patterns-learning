#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "event_bus.h"
#include "serive_ui.h"
#include "serive_touch.h"
#include "driver/gpio.h"

static event_bus_handle_t bus_handle = NULL;

void app_main(void)
{
    xTaskCreate(ui_test_task, "ui_test_task", 2048 * 4, NULL, 3, NULL);
    vTaskDelay(pdMS_TO_TICKS(300));
    xTaskCreate(touch_test_task, "touch_test_task", 2048 * 3, NULL, 5, NULL);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}
