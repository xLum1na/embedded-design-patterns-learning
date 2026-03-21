#include <stdio.h>
#include "driver_button.h"
#include "driver/timer.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

static const char *TAG = "button";


//去抖动时间
#define BUTTON_DEBOUNCE_TIME_MS 20


//按钮句柄
typedef struct button_t{
    button_config_t button_cfg;
    button_event_t last_event;
    uint32_t press_start_time_ms;   //按下开始时间
    bool is_pressed;                //当前物理电平是否按下（低电平有效）
}button_t;


/*
 * @brief 按键初始化
 * 
 * @param[in] config 按键配置
 * @param[out] ret_handle 按键句柄
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 *      - ESP_ERR_INVALID_ARG
 *      - ESP_ERR_NO_MEM
 * 
 */
esp_err_t button_init(button_config_t *config, button_handle_t *ret_handle)
{
    ESP_LOGI(TAG, "button init");
    if (config == NULL || ret_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    button_handle_t handle = malloc(sizeof(button_t));
    if (handle == NULL) {
        return ESP_ERR_NO_MEM;
    }
    //gpio初始化
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << config->button,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&cfg);
    if (ret != ESP_OK) {
        return ESP_FAIL;
    }
    //返回句柄
    handle->button_cfg = *config;
    handle->last_event = BUTTON_EVENT_IDLE;
    handle->is_pressed = false;
    handle->press_start_time_ms = 0;
    *ret_handle = handle;
    return ESP_OK;
}

/*
 * @brief 按键反初始化
 * 
 * @param[in] handle 按键句柄
 * @return
 *      - ESP_OK
 *      - ESP_ERR_INVALID_ARG
 * 
 */
esp_err_t button_deinit(button_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    free(handle);
    return ESP_OK;
}

/*
 * @brief 按键轮询
 * 
 * @param[in] handle 按键句柄
 * @return
 *      - ESP_OK
 *      - ESP_ERR_INVALID_ARG
 * 
 */
esp_err_t button_poll(button_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    //获取io电平与时间
    int level = gpio_get_level(handle->button_cfg.button);
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS; // 获取当前毫秒时间
    if (!level) {
        //检测为按下
        if (!handle->is_pressed) {
            //按下消抖动
            handle->is_pressed = true;
            handle->press_start_time_ms = now;
        } else {
            uint32_t press_duration = now - handle->press_start_time_ms;
            //已经去抖动，判断按压事件
            if (press_duration >= BUTTON_DEBOUNCE_TIME_MS &&
                handle->last_event == BUTTON_EVENT_IDLE) {
                handle->last_event = BUTTON_EVENT_PRESS;
                if (handle->button_cfg.on_event) {
                    handle->button_cfg.on_event(handle, BUTTON_EVENT_PRESS, handle->button_cfg.user_data);
                }
            }
            //判断长按事件
            if (press_duration >= handle->button_cfg.long_press_ms 
            && (handle->last_event != BUTTON_EVENT_LONG_PRESS)){
                handle->last_event = BUTTON_EVENT_LONG_PRESS;
                if (handle->button_cfg.on_event) {
                    handle->button_cfg.on_event(handle, BUTTON_EVENT_LONG_PRESS, handle->button_cfg.user_data);
                }
            }
        }
    } else {
        //检测为释放
        uint32_t press_duration = now - handle->press_start_time_ms;
        if (handle->is_pressed) {
            handle->is_pressed = false; //重置状态
            if (press_duration >= BUTTON_DEBOUNCE_TIME_MS) {
                //有效释放触发RELEASE回调
                if (handle->button_cfg.on_event) {
                    handle->button_cfg.on_event(handle, BUTTON_EVENT_RELEASE, handle->button_cfg.user_data);
                }
                if (handle->last_event != BUTTON_EVENT_LONG_PRESS) {
                    //触发点击回调
                    if (handle->button_cfg.on_event) {
                        handle->button_cfg.on_event(handle, BUTTON_EVENT_CLICK, handle->button_cfg.user_data);
                    }
                }
            }
            //重置按键状态
            handle->last_event = BUTTON_EVENT_IDLE;
        }
    }
    return ESP_OK;
}

