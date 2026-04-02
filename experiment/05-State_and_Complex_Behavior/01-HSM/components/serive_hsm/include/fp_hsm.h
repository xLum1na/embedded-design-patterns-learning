#pragma once


#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "driver/gpio.h"
#include "driver/uart.h"


#ifdef __cplusplus
extern "C" {
#endif

//=============== 指纹模块分层状态机 ===============//
//=============== 状态定义 ===============//
typedef enum {
    STA_FP_HSM_IDLE = -1,
    STA_FP_HSM_START_VERIFY,
    STA_FP_HSM_VERIFYING,
    STA_FP_HSM_VERIFY_SUCCESS,
    STA_FP_HSM_VERIFY_FAIL,
    STA_FP_HSM_STRAT_SET,
    STA_FP_HSM_SETTING,
    STA_FP_HSM_SET_SUCCESS,
    STA_FP_HSM_SET_FAIL,
}sta_fp_hsm_t;


//=============== 发布事件定义 ===============//
typedef enum {
    EVT_FP_VERIFY = -1,
    EVT_FP_VERIFY_SUCCESS,
    EVT_FP_VERIFY_FAIL,
    EVT_FP_SET,
    EVT_FP_SET_SUCCESS,
    EVT_FP_SET_FAIL,
}evt_fp_hsm_t;


//=============== 数据结构定义 ===============//
typedef struct {

}fp_hsm_t;


//=============== 外部API ===============//
//指纹模块状态机，通过事件驱动，订阅事件总线上的事件，观察指纹驱动层传递的事件，向总线发布对应事件
void fp_hsm(void);



#ifdef __cplusplus
}
#endif