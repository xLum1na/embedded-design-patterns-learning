#include <stdio.h>
#include "driver_button.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//按键句柄
button_handle_t but_handle = NULL;
button_handle_t but1_handle = NULL;

//事件信息传递队列
QueueHandle_t g_button_event_queue = NULL;

//事件信息
typedef struct {
    int btn_id;
    button_event_t event;
} button_event_msg_t;

/*
 * @brief 按键回调
 * 
 * @param[in] btn 按键句柄
 * @param[in] event 事件
 * @param[in] user_data 用户私有数据
 * @return NULL
 * 
 */
void button_poll_callback(button_handle_t btn, button_event_t event, void *user_data)
{
    int id = (int)(uintptr_t)user_data;
    button_event_msg_t msg = {
        .btn_id = id,
        .event = event
    };

    xQueueSend(g_button_event_queue, &msg, 0); 

}

/*
 * @brief 按键事件处理任务
 * 
 * @param[in] arg
 * @return NULL
 * 
 */
void button_event_handler_task(void *arg)
{
    button_event_msg_t msg;
    while (1) {
        if (xQueueReceive(g_button_event_queue, &msg, portMAX_DELAY) == pdTRUE) {
            switch (msg.event) {
                case BUTTON_EVENT_CLICK:
                    printf("Button %d clicked!\n", msg.btn_id);
                    break;
                case BUTTON_EVENT_LONG_PRESS:
                    printf("Button %d long press!\n", msg.btn_id);
                    break;
                case BUTTON_EVENT_RELEASE:
                    printf("Button %d release!\n", msg.btn_id);
                    break;
                case BUTTON_EVENT_PRESS:
                    printf("Button %d press!\n", msg.btn_id);
                    break;
                default:
                    printf("Button %d err!\n", msg.btn_id);
                    break;
            }
        }

    }
}

/*
 * @brief 按键任务
 * 
 * @param[in] arg
 * @return NULL
 * 
 */
void poll_task(void *arg)
{

    while (1) {
        button_poll(but_handle);
        button_poll(but1_handle);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{

    //创建队列
    g_button_event_queue = xQueueCreate(10, sizeof(button_event_msg_t));
    if (!g_button_event_queue) {
        printf("Queue create failed!\n");

    }

    //初始化按钮
    button_config_t cfg = {
        .button = GPIO_NUM_40,
        .long_press_ms = 1000,
        .on_event = button_poll_callback,
        .user_data = (void*)(uintptr_t)1
    };
    esp_err_t err = button_init(&cfg, &but_handle);
    if (err != ESP_OK) {
        printf("Button1 init failed! (%d)\n", err);
    }
        //初始化按钮
    button_config_t cfg1 = {
        .button = GPIO_NUM_41,
        .long_press_ms = 1000,
        .on_event = button_poll_callback,
        .user_data = (void*)(uintptr_t)2
    };
    err = button_init(&cfg1, &but1_handle);
    if (err != ESP_OK) {
        printf("Button2 init failed! (%d)\n", err);
    }

    //创建任务
    TaskHandle_t evt_handle = NULL;
    TaskHandle_t buton_handle = NULL;
    xTaskCreate(button_event_handler_task, "btn_evt", 2048 * 2, NULL, 3, &evt_handle);
    xTaskCreate(poll_task, "btn_poll", 2048, NULL, 5, &buton_handle);

    vTaskDelay(pdMS_TO_TICKS(50));

    vTaskDelete(NULL);

}