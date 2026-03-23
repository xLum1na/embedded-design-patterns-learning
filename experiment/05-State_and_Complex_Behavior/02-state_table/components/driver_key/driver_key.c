#include <stdio.h>
#include "driver_key.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "driver/ledc.h"


static const char *TAG = "driver_key";


//按键句柄
typedef struct key_t {
    key_config_t cfg;                   //管理按键配置
    key_state_t current_state;             //按键上一次状态记录
    int press_start_time_ms;       //按键按下开始时间戳
    int release_start_time_ms;     //按键释放开始时间
    bool is_press;                      //按键是否按下判断
    int last_click_time_ms;         // 👈 关键：记录上一次有效点击时间
    bool has_pending_click; 
}key_t;



//按键初始化
esp_err_t key_init(const key_config_t *cfg, key_handle_t *handle)
{
    ESP_LOGI(TAG, "key init start");
    esp_err_t ret;
    //参数有效判断
    if (cfg == NULL || handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    //按键句柄空间分配
    key_handle_t ret_handle = (key_handle_t)malloc(sizeof(key_t));
    if (ret_handle == NULL) {
        return ESP_ERR_NO_MEM;
    }
    gpio_config_t io_cfg = {
        .pin_bit_mask = 1ULL << cfg->key_io,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&io_cfg);
    if (ret != ESP_OK) {
        return ret;
    }
    //返回句柄
    ret_handle->cfg = *cfg;
    ret_handle->current_state = KEY_STA_IDLE;
    ret_handle->press_start_time_ms = 0;
    ret_handle->release_start_time_ms = 0;
    ret_handle->is_press = false;
    ret_handle->has_pending_click = false;
    ret_handle->last_click_time_ms = 0;
    *handle = ret_handle;

    return ESP_OK;
}

//按键反初始化
esp_err_t key_deinit(key_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    free(handle);
    return ESP_OK;
}

//按键轮询
esp_err_t key_poll(key_handle_t handle)
{
    //
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    //获取IO电平以及当前时间
    int level = gpio_get_level(handle->cfg.key_io);
    int now = esp_timer_get_time() / 1000;

    if (!level) {
        //按键按下
        if (!handle->is_press) {
            //去抖动
            handle->is_press = true;
            handle->press_start_time_ms = now;
            handle->current_state = KEY_STA_DEBOUNCE;
        } else {
            if (now - handle->press_start_time_ms >= DEBOUNCE_TIME_MS) {
                //进入按下状态
                handle->current_state = KEY_STA_PRESS;
            }
            if (now - handle->press_start_time_ms >= handle->cfg.longpress_times) {
                //进入长按状态
                handle->current_state = KEY_STA_LONG_PRESS;
                if (handle->cfg.event_cb) {
                    handle->cfg.event_cb(handle, KEY_EVENT_LONG_PRESS, handle->cfg.user_data);
                }
            }
        }

    } else {
        //按键释放
        //去抖动失败
        uint32_t press_duration = now - handle->press_start_time_ms;
        if (handle->is_press) {
            handle->is_press = false; //重置状态
            if (press_duration >= DEBOUNCE_TIME_MS) {
                if (handle->current_state != KEY_STA_LONG_PRESS) {
                    //触发点击回调
                    if (handle->cfg.event_cb) {
                        handle->cfg.event_cb(handle, KEY_EVENT_CLICK, handle->cfg.user_data);
                    }
                }
            }
            //重置按键状态
            handle->current_state = KEY_STA_IDLE;
        }
    }
    return ESP_OK;
}
