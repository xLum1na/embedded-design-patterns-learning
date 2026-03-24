#pragma once

#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "driver_key.h"


#ifdef __cplusplus
extern "C" {
#endif

#define LEDC_FREQ_HZ        4000

//led句柄
typedef struct led_t *led_handle_t;

//led状态定义
typedef enum {
    LED_STA_OFF = 0,        //led关闭状态
    LED_STA_ON,             //led打开状态
    LED_STA_BLINK,          //led闪烁状态
    LED_STA_BREATHE,        //led呼吸灯状态
    LED_STA_MAX             //led状态边界
}led_state_t;

typedef enum {
    LED_BREATHE_STATE_IN,  // 吸气（变亮）
    LED_BREATHE_STATE_OUT  // 呼气（变灭）
} led_breathe_state_t;


//led状态表定义
typedef struct transition_table_t {
    led_state_t next_state;                 //下一个状态
    void (*action)(led_handle_t handle);    //对应状态处理
}transition_table_t;


//led配置结构体
typedef struct led_config_t {
    gpio_num_t led_io;      //ledIO
    int64_t blink_time;     //led闪烁间隔
    int64_t breathe_time;    //led渐变时间
    void *user_data;        //用户私有数据
}led_config_t;

//全局事件队列
typedef struct event_t {
    key_event_t event;
    void *user_data;
}event_t;
extern QueueHandle_t event_queue;
//led初始化
esp_err_t led_init(const led_config_t *cfg, led_handle_t *handle);

//led反初始化
esp_err_t led_deinit(led_handle_t handle);

esp_err_t led_machine(led_handle_t handle);

#ifdef __cplusplus
}
#endif
