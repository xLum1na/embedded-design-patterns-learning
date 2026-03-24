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
    key_state_t current_state;          //按键上一次状态记录
    int press_start_time_ms;            //按键按下开始时间戳
    int release_start_time_ms;          //按键释放开始时间
    bool is_press;                      //按键是否按下判断
    int last_click_time_ms;             //记录上一次有效点击时间
    bool is_double_press;               //记录是否构成二次点击
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
    ret_handle->is_double_press = false;
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
esp_err_t key_machine(key_handle_t handle)
{
    //
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    //获取IO电平以及当前时间
    int level = gpio_get_level(handle->cfg.key_io);
    int now = esp_timer_get_time() / 1000;

    if (!level) {
        // ================= 检测到低电平（按键按下） =================
        switch (handle->current_state) {
            case KEY_STA_IDLE:
                //空闲 -> 记录时间，进入消抖
                handle->press_start_time_ms = now;
                handle->current_state = KEY_STA_DEBOUNCE;
                handle->is_double_press = false; // 新的一次按下，默认不是双击
                break;
            case KEY_STA_DEBOUNCE:
                //消抖中 -> 检查是否满足消抖时间
                if (now - handle->press_start_time_ms >= DEBOUNCE_TIME_MS) {
                    handle->current_state = KEY_STA_PRESS;
                }
                break;
            case KEY_STA_PRESS:
                //确认按下 -> 检查是否满足长按时间
                if (now - handle->press_start_time_ms >= handle->cfg.longpress_times) {
                    handle->current_state = KEY_STA_LONG_PRESS;
                    //进入长按
                    handle->is_double_press = false; 
                }
                break;
            case KEY_STA_WAIT_RELEASE:
                //等待释放中又按下了 -> 判定为双击流程的开始
                handle->press_start_time_ms = now;
                handle->current_state = KEY_STA_DEBOUNCE; 
                handle->is_double_press = true;
                break;
            default:
                break;       
        }

    } else {
        // ================= 检测到高电平（按键释放） =================
        switch (handle->current_state) {
            case KEY_STA_DEBOUNCE:
                //消抖期间就释放了 -> 视为干扰，回空闲
                handle->current_state = KEY_STA_IDLE;
                break;
            case KEY_STA_PRESS:
                //确认按下后释放 -> 根据标志位决定去向
                if (handle->is_double_press) {
                    //如果是双击序列的第二次释放 -> 进入等待双击确认
                    handle->current_state = KEY_STA_WAIT_DOUBLE;
                } else {
                    //如果是普通按下释放 -> 进入等待释放，看是否有第二次按下
                    handle->current_state = KEY_STA_WAIT_RELEASE;
                }
                handle->release_start_time_ms = now;
                break;
            case KEY_STA_LONG_PRESS:
                //长按释放 -> 触发长按事件，回空闲
                handle->current_state = KEY_STA_IDLE;
                handle->is_double_press = false; // 清理标志
                if (handle->cfg.event_cb) {
                    handle->cfg.event_cb(handle, KEY_EVENT_LONG_PRESS, handle->cfg.user_data);
                }
                break;
            case KEY_STA_WAIT_RELEASE:
                //普通释放后，检查是否超时
                if (now - handle->release_start_time_ms >= handle->cfg.double_times) {
                    //超时未检测到第二次按下 -> 确认为单击
                    handle->current_state = KEY_STA_IDLE;
                    if (handle->cfg.event_cb) {
                        handle->cfg.event_cb(handle, KEY_EVENT_CLICK, handle->cfg.user_data);
                    }
                }    
                break;
            case KEY_STA_WAIT_DOUBLE:
                //双击序列的第二次释放 -> 立即触发双击事件
                handle->current_state = KEY_STA_IDLE;
                handle->is_double_press = false; //清除标志位
                if (handle->cfg.event_cb) {
                    handle->cfg.event_cb(handle, KEY_EVENT_DOUBLE_CLICK, handle->cfg.user_data);
                }
                break;
            default:
                break;       
        }
    }
    return ESP_OK;
}

