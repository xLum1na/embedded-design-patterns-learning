#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver_key.h"
#include "driver_led.h"

key_handle_t key_handle = NULL;
led_handle_t led_handle = NULL;
QueueHandle_t event_queue = NULL; 
//按键事件回调
void key_event_cb(key_handle_t handle, key_event_t event, void *user_data)
{
    //发送对应事件到队列
    printf("event = %d\n",event);
    event_t current_event = {event, user_data};
    if (xQueueSend(event_queue, &current_event, pdMS_TO_TICKS(10)) != pdTRUE) {
        printf("send event fail\n");
    }
}

 
void key_task(void *arg)
{
    
    while (1) {
        key_machine(key_handle);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
}

void led_task(void *arg)
{
    
    while (1) {
        led_machine(led_handle);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
}


void app_main(void)
{
    //创建队列
    event_queue = xQueueCreate(10, sizeof(event_t));
    if (event_queue == NULL) {
        printf("Failed to create queue!\n");
        return;
    }
    //按键初始化
    int id = 1;
    key_config_t cfg = {
        .key_io = GPIO_NUM_40,
        .double_times = 300,
        .longpress_times = 1000,
        .event_cb = key_event_cb,
        .user_data = (void*)id,
    };
    key_init(&cfg, &key_handle);
    //led初始化
    led_config_t led_cfg = {
        .led_io = GPIO_NUM_4,
        .blink_time = 500,
        .breathe_time = 2000,
        .user_data = (void*)id,
    };
    led_init(&led_cfg, &led_handle);

    vTaskDelay(pdMS_TO_TICKS(50));

    xTaskCreate(key_task, "key_task", 2048 * 2, NULL, 5, NULL);
    xTaskCreate(led_task, "led_task", 2048 * 2, NULL, 3, NULL);


    vTaskDelay(pdMS_TO_TICKS(10));

    vTaskDelete(NULL);
    
}

