#pragma once

#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "driver/gpio.h"



#ifdef __cplusplus
extern "C" {
#endif

#define DEBOUNCE_TIME_MS 20

//
typedef struct key_t *key_handle_t;

//按键状态
typedef enum {
    KEY_STA_IDLE = 0,       //空闲状态
    KEY_STA_DEBOUNCE,       //去抖动状态
    KEY_STA_PRESS,          //按下状态
    KEY_STA_LONG_PRESS,     //长按状态
    KEY_STA_WAIT_RELEASE,   //等待全部释放状态
    KEY_STA_WAIT_DOUBLE,    //等待双击状态
}key_state_t;

//按键事件
typedef enum {  
    KEY_EVENT_CLICK = 0,            //按键点击事件
    KEY_EVENT_LONG_PRESS,       //按键长按事件
    KEY_EVENT_DOUBLE_CLICK,     //按键双击事件
}key_event_t;

//按键回调
typedef void (*key_event_cb_t)(key_handle_t handle, key_event_t event, void *user_data);

//按键配置结构体
typedef struct key_config_t{
    gpio_num_t key_io;          //按键io
    int longpress_times;        //长按判断时间设置
    int double_times;           //双击判断时间设置
    key_event_cb_t event_cb;    //按键事件回调
    void *user_data;            //用户私有数据
}key_config_t;


//按键初始化
esp_err_t key_init(const key_config_t *cfg, key_handle_t *handle);

//按键反初始化
esp_err_t key_deinit(key_handle_t handle);

//按键轮询
esp_err_t key_machine(key_handle_t handle);

#ifdef __cplusplus
}
#endif
