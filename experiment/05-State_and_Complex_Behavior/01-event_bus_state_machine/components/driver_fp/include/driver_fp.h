#pragma once


#include "stdint.h"
#include "stdbool.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============== 前向声明 ===============//
typedef struct fp_t *fp_handle_t;

//=============== 识别状态机状态 ===============//
typedef enum {
    STA_FP_DRV_IDLE = 0,
    STA_FP_DRV_VERIFY,
    STA_FP_DRV_
}fp_sta_t;



//=============== 向外发布识别事件 ===============//
typedef enum {
    EVT_FP_DRV_NONE = 0,            //默认状态
    EVT_FP_FINGER_ON,               //手指放上开始识别
    EVT_FP_DRV_RECOGNIZE_SUCCESS,   //识别成功
    EVT_FP_DRV_RECOGNIZE_FAIL,      //识别失败
    EVT_FP_DRV_TIMEOUT,             //超时
} fp_evt_t;

typedef struct {
    fp_evt_t event;
    void *data;
    uint32_t data_len;  
}fp_event_t;


//=============== 数据结构 ===============//
//模块波特率定义
typedef enum {
    BAUD_TATE_LOW_SPEED     = 4800,
    BAUD_TATE_MEDIUM_SPEED  = 9600,
    BAUD_TATE_HIGH_SPEED    = 57600,
    BAUD_TATE_SUPER_SPEED   = 115200,
}fp_baud_rate_t;

//模块工作模式定义
typedef enum {
    REAL_TIME_MODE                  = 0x01, //实时模式
    SELF_LOCK_MODE                  = 0x02, //自锁模式
    ONE_JOG_MODE                    = 0x03, //一秒点动模式
    FIVE_JOG_MODE                   = 0x04, //五秒点动模式
    TEN_JOG_MODE                    = 0x05, //十秒点动模式
    THIRTY_JOG_MODE                 = 0x06, //三十秒点动模式
    SIXTY_JOG_MODE                  = 0x07, //六十秒点动模式
    ONE_HANDRED_AND_TWENTY_MODE     = 0x08, //一百二十秒点动模式
}fp_mode_t;

//事件回调函数
typedef void (*fp_event_callback_t)(fp_handle_t handle, fp_event_t *event);

//配置结构体
typedef struct fp_config_t {
    uint8_t dev_addr;               //设备地址
    uart_port_t uart_port;          //串口端口号
    gpio_num_t fp_rx;               //串口接收io
    gpio_num_t fp_tx;               //串口发送io
    gpio_num_t fp_vt;               //模块中断io
    fp_baud_rate_t baud;            //模块波特率
    fp_mode_t mode;                 //模块工作模式
    int rx_buf_size;                //串口接收缓冲区大小
    int tx_buf_size;                //串口发送缓冲区大小
    fp_event_callback_t on_event;   //模块事件回调
    void *user_data;                //用户私有数据
}fp_config_t;


//=============== 外部API ===============//

#ifdef __cplusplus
}
#endif
