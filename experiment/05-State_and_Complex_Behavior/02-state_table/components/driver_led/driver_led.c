#include <stdio.h>
#include "driver_led.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_attr.h" 


//led句柄
typedef struct led_t {
    led_config_t led;
    led_state_t current_state;
    bool is_on;
    int64_t last_blink_time;
    led_breathe_state_t breathe_state;
}led_t;


//led点亮
static void led_on(led_handle_t handle) 
{
    if (!handle) {
        return;
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

//led关闭
static void led_off(led_handle_t handle) 
{
    if (!handle) {
        return;
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

//led闪烁
static void led_blink(led_handle_t handle) 
{
    if (!handle) {
        return;
    }
    int64_t now = esp_timer_get_time() / 1000;
    if (now - handle->last_blink_time >= handle->led.blink_time) {
        if (handle->is_on ) {
            led_off(handle);
        } else {
            led_on(handle);
        }
        handle->is_on = !handle->is_on; 
        handle->last_blink_time = now;
    }
}

//led呼吸灯
static void led_breathe(led_handle_t handle)
{
    if (!handle) return;
    
    //初始化渐变
    handle->breathe_state = LED_BREATHE_STATE_IN;
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 
                          8191, handle->led.breathe_time);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
}

//状态表
const transition_table_t tran_t[LED_STA_MAX] = {
    [LED_STA_OFF] = {LED_STA_OFF, led_off},
    [LED_STA_ON] = {LED_STA_ON, led_on},
    [LED_STA_BLINK] = {LED_STA_BLINK, led_blink},
    [LED_STA_BREATHE] = {LED_STA_BREATHE, led_breathe},
};



//渐变回调
static bool IRAM_ATTR led_fade_cb(const ledc_cb_param_t *param, void *user_arg)
{
    led_t *led = (led_t *)user_arg;
    
    if (param->event == LEDC_FADE_END_EVT) {
        // 渐变结束，切换方向
        if (led->breathe_state == LED_BREATHE_STATE_IN) {
            led->breathe_state = LED_BREATHE_STATE_OUT;
            ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 
                                  0, led->led.breathe_time);
        } else {
            led->breathe_state = LED_BREATHE_STATE_IN;
            ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 
                                  8191, led->led.breathe_time);
        }
        ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
    }
    return false;
}

//led初始化
esp_err_t led_init(const led_config_t *cfg, led_handle_t *handle)
{
    esp_err_t ret;
    if (cfg == NULL || handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    //返回句柄
    led_handle_t ret_handle = malloc(sizeof(led_t));
    if (!ret_handle) {
        return ESP_ERR_NO_MEM;
    }
    //ledc初始化
    ledc_timer_config_t ledc_timer_cfg = {
        .timer_num = LEDC_TIMER_0,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .freq_hz = LEDC_FREQ_HZ,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ret = ledc_timer_config(&ledc_timer_cfg);
    if (ret != ESP_OK) {
        return ret;
    }
    ledc_channel_config_t ledc_chan_cfg = {
        .channel    = LEDC_CHANNEL_0,
        .duty       = 0,
        .gpio_num   = cfg->led_io,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0,
        .flags.output_invert = 0
    };
    ret = ledc_channel_config(&ledc_chan_cfg);
    if (ret != ESP_OK) {
        return ret;
    }
    //渐变功能
    ledc_fade_func_install(0);
    ledc_cbs_t cb = {
        .fade_cb = led_fade_cb,
    };
    ret = ledc_cb_register(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, &cb, (void *)ret_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    //初始化
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    //返回句柄
    ret_handle->led = *cfg;
    ret_handle->last_blink_time = 0;
    ret_handle->is_on = false;
    ret_handle->current_state = LED_STA_OFF;
    *handle = ret_handle;

    return ESP_OK;
}

//led反初始化
esp_err_t led_deinit(led_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    free(handle);
    return ESP_OK;
}

//led状态机
esp_err_t led_machine(led_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    //获取队列消息
    event_t received_evt;
    if (xQueueReceive(event_queue, &received_evt, pdMS_TO_TICKS(10)) == pdTRUE) {
        // 3. 收到消息，处理状态跳转逻辑
        printf("LED Received Event: %d\n", received_evt.event);
        
        switch (received_evt.event) {
            case KEY_EVENT_CLICK: 
                //只管led开与关
                if (handle->current_state == LED_STA_OFF) {
                    handle->current_state = LED_STA_ON;
                } else if (handle->current_state == LED_STA_ON) {
                    handle->current_state = LED_STA_OFF;
                }
                break;
            case KEY_EVENT_LONG_PRESS:
                //管理led闪烁
                if (handle->current_state == LED_STA_BLINK) {
                     handle->current_state = LED_STA_OFF;
                } else if (handle->current_state == LED_STA_OFF) {
                    handle->current_state = LED_STA_BLINK;
                }
                break;
            case KEY_EVENT_DOUBLE_CLICK:
                //管理led呼吸灯
                if (handle->current_state ==LED_STA_BREATHE) {
                     handle->current_state = LED_STA_OFF;
                } else if (handle->current_state == LED_STA_OFF) {
                    handle->current_state = LED_STA_BREATHE;
                }
                break;
            default:
                break;
        }
    }

    // 4. 查表执行当前状态的动作
    if (handle->current_state < LED_STA_MAX) {
        tran_t[handle->current_state].action(handle);
    }

    return ESP_OK;
}
