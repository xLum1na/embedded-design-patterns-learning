#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver_key.h"




void key_event_cb(key_handle_t handle, key_event_t event, void *user_data)
{
    key_event_t current_event = event;
    int id = (int)user_data;
    switch (current_event) {
        case KEY_EVENT_CLICK:
            printf("key %d clicked!\n", id);
            break;
        case KEY_EVENT_LONG_PRESS:
            printf("key %d long press!\n", id);
            break;
        case KEY_EVENT_DOUBLE_CLICK:
            printf("key %d double click!\n", id);
            break;
        default:
            printf("key %d err!\n", id);
            break;
    }
}


key_handle_t handle = NULL;
void key_task(void *arg)
{
    
    while (1) {
        key_poll(handle);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    

}


void app_main(void)
{
    //按键初始化
    int id = 1;
    key_config_t cfg = {
        .key_io = GPIO_NUM_41,
        .double_times = 300,
        .longpress_times = 2000,
        .event_cb = key_event_cb,
        .user_data = (void*)id,
    };
    key_init(&cfg, &handle);

    vTaskDelay(pdMS_TO_TICKS(50));

    xTaskCreate(key_task, "key_task", 2048 * 2, NULL, 3, NULL);

    vTaskDelay(pdMS_TO_TICKS(20));

    vTaskDelete(NULL);
}

