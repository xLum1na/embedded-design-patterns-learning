#include "fp_hsm.h"
#include "esp_timer.h"
#include "driver_fp.h"
#include "esp_log.h"
#include "string.h"

static sys_event_bus_handle_t g_fp_bus_handle = NULL;
static fp_handle_t g_fp_handle = NULL;
//对应底层驱动回调，通过对应底层事件发布对应的事件
void fp_drv_handler(fp_handle_t handle, fp_event_t *event)
{
    fp_event_t *ev = event;
    switch (ev->event) {
    case EVT_FP_DRV_NONE:
        //不用向事件总线发送任何事件
        break;
    case EVT_FP_FINGER_ON:
        //发布触摸事件
        event_bus_publish(g_fp_bus_handle, EVT_FINGER_TOUCH, ev->data, ev->data_len);
        break;
    case EVT_FP_DRV_RECOGNIZE_SUCCESS:
        //发布识别成功事件
        event_bus_publish(g_fp_bus_handle, EVT_FINGER_SUCCESS, ev->data, ev->data_len);
        break;
    case EVT_FP_DRV_RECOGNIZE_FAIL:
        //发布识别失败事件
        event_bus_publish(g_fp_bus_handle, EVT_FINGER_FAIL, ev->data, ev->data_len);
        break;
    case EVT_FP_DRV_TIMEOUT:
        //发布识别i失败事件
        event_bus_publish(g_fp_bus_handle, EVT_FINGER_FAIL, ev->data, ev->data_len);
        break;
    default:
        break;
    }


}


//处理对应命令事件回调
static void fp_event_handler(const sys_event_t *ev, void *ctx)
{
    switch (ev->type)
    {
    case EVT_FINGER_CMD_ENROLL:
        //由于指纹模块只订阅了命令事件，所以就只需要进行命令事件的处理
        //更具对应命令事件中传递的数据来判断要进行什么设置
        if ((string)ctx == "set_buad_rate") {
            //调用底层接口
            fp_set_buad_rate(g_fp_handle, (fp_baud_rate_t)ev->payload);
        } else if (ev->payload == "set_mode") {
            //调用底层接口
            fp_set_mode(g_fp_handle, (fp_mode_t)ev->payload);
        } else if (ev->payload == "enroll_fp") {
            //调用底层注册指纹接口
            fp_enroll(g_fp_handle, (uint8_t)ev->payload);
        }
        
        
        break;
    default:
        break;
    }
}



//初始化硬件及订阅事件
void fp_hsm_init(sys_event_bus_handle_t bus)
{
    g_fp_bus_handle = bus;
    fp_machine(g_fp_handle);
    fp_init();
    fp_machine(g_fp_handle);
    //订阅对应事件
    event_bus_subscribe(g_fp_bus_handle, EVT_FINGER_CMD_ENROLL, fp_event_handler, NULL); 
    
}






