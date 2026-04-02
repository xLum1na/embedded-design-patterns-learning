#pragma once


#include "stdint.h"
#include "stdbool.h"
#include "event_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

//指纹状态机状态定义
typedef enum {
    STA_FP_IDLE = 0,        //空闲状态
    STA_FP_VERIFY,          //验证状态
    STA_FP_SET,             //设置状态
    STA_FP_ENROLL,          //注册删除状态
}fp_sta_t;

//指纹状态机数据结构


//指纹状态机外部API
void fp_hsm_init(sys_event_bus_handle_t bus);

//发布事件
void fp_hsm_process(void);





#ifdef __cplusplus
}
#endif