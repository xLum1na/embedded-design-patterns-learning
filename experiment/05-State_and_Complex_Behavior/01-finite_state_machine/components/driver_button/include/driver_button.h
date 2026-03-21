#pragma once

#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "driver/gpio.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct button_t *button_handle_t;

//按键状态定义
typedef enum {
    BUTTON_EVENT_IDLE = 0,          //空闲状态
    BUTTON_EVENT_PRESS,      //按下状态
    BUTTON_EVENT_RELEASE,       //释放状态
    BUTTON_EVENT_CLICK,         //点击状态
    BUTTON_EVENT_LONG_PRESS,    //长按状态
}button_event_t;

//统一事件回调
typedef void (*button_event_cb_t)(button_handle_t btn, button_event_t event, void *user_data);

//按钮配置
typedef struct {
    gpio_num_t button;          //按键io
    uint32_t long_press_ms;     //长按时间
    button_event_cb_t on_event; //统一回调
    void *user_data;            //用户私有数据
}button_config_t;

//按钮初始化
esp_err_t button_init(button_config_t *config, button_handle_t *ret_handle);

//按钮反初始化
esp_err_t button_deinit(button_handle_t ret_handle);

//按钮轮询
esp_err_t button_poll(button_handle_t handle);





#ifdef __cplusplus
}
#endif
