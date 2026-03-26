#pragma once


#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "driver/gpio.h"
#include "driver/uart.h"


#ifdef __cplusplus
extern "C" {
#endif

//指纹模块驱动
//句柄
typedef struct fingerprint_t *fingerprint_handle_t;


// 指纹模块错误码定义
typedef enum {
    FP_OK                       = 0x00, //操作成功
    FP_FAIL                     = 0x01, //通用失败
    FP_ERR_INVALID_ARG          = 0x02, //参数无效
    FP_ERR_NO_MEM               = 0x03, //内存不足
    FP_ERR_TIMEOUT              = 0x04, //操作超时
    FP_ERR_COMM                 = 0x05, //串口通信错误
    FP_ERR_CHECKSUM             = 0x06, //数据包校验和错误
    FP_ERR_NOT_FOUND            = 0x07, //未找到指纹（ID不存在）
    FP_ERR_MATCH                = 0x08, //指纹比对失败（不匹配）
    FP_ERR_FULL                 = 0x09, //指纹库已满
    FP_ERR_ENROLL_FAILED        = 0x0A, //注册指纹失败
    FP_ERR_FLASH_WRITE          = 0x0B, //flash写入错误
    FP_ERR_INVALID_ID           = 0x0C, //无效的指纹ID
} fingerprint_err_t;

//波特率定义
typedef enum {
    BAUD_TATE_LOW_SPEED     = 4800,
    BAUD_TATE_MEDIUM_SPEED  = 9600,
    BAUD_TATE_HIGH_SPEED    = 57600,
    BAUD_TATE_SUPER_SPEED   = 115200,
}fingerprint_baud_rate_t;

//工作模式定义
typedef enum {
    REAL_TIME_MODE                  = 0x01, //实时模式
    SELF_LOCK_MODE                  = 0x02, //自锁模式
    ONE_JOG_MODE                    = 0x03, //一秒点动模式
    FIVE_JOG_MODE                   = 0x04, //五秒点动模式
    TEN_JOG_MODE                    = 0x05, //十秒点动模式
    THIRTY_JOG_MODE                 = 0x06, //三十秒点动模式
    SIXTY_JOG_MODE                  = 0x07, //六十秒点动模式
    ONE_HANDRED_AND_TWENTY_MODE     = 0x08, //一百二十秒点动模式
}fingerprint_mode_t;

//指纹状态
typedef enum {
    FP_STA_SHUT = -1,           //指纹模块关闭状态
    FP_STA_INIT,                //指纹初始化状态，确定波特率
    FP_STA_STANBY,              //指纹模块待机状态
    FP_STA_ENROLL,              //指纹注册状态
    FP_STA_DELETE,              //指纹删除状态
    FP_STA_VERIFY,              //指纹验证状态
    FP_STA_JOG_TIMING,          //点动模式状态
}fingerprint_state_t;

//指纹模块生产事件
typedef enum {
    FP_EVT_SHUT = -1,           //指纹模块关闭事件
    FP_EVT_OPEN,                //指纹模块开启事件
    FP_EVT_CLOSE_LIGHT,         //指纹模块关闭灯光事件
    FP_EVT_OPEN_LIGHT,          //指纹模块开启灯光事件
    FP_EVT_SET_BAUD,            //指纹模块确定波特率事件
    FP_EVT_STANBY,              //指纹模块待机事件
    FP_EVT_ENROLLINT,           //指纹模块指纹注册中事件
    FP_EVT_ENROLL_COMPLATE,     //指纹模块指纹注册完成事件
    FP_EVT_ENROLL_SUCCESS,      //指纹模块注册指纹成功事件
    FP_EVT_ENROLL_FAIL,         //指纹模块注册指纹失败事件
    FP_EVT_DELETE_SUCCESS,      //指纹模块删除指纹成功事件
    FP_EVT_DELETE_ALL_SUCCESS,  //指纹模块删除所有指纹成功事件
    FP_EVT_DELETE_FAIL,         //指纹模块删除指纹失败事件
    FP_EVT_VERIFYING,           //指纹模块指纹验证中事件
    FP_EVT_VERIFY_COMPLATE,     //指纹模块指纹验证完成事件
    FP_EVT_VERIFY_SUCCESS,      //指纹模块验证指纹成功事件
    FP_EVT_VERIFY_FAIL,         //指纹模块验证指纹失败事件
}fingerprint_event_t;

//指纹事件回调
typedef void (*fingerprint_event_cb_t)(fingerprint_handle_t handle, fingerprint_event_t event, void *user_data);

//指纹模块配置
typedef struct fingerprint_config_t {
    uart_port_t uart_port;              //串口端口号
    gpio_num_t uart_rxd;                //串口rx（接收）
    gpio_num_t uart_txd;                //串口tx（发送）
    uint32_t baud_rate;                 //串口波特率
    uint16_t rx_buffer_size;            //串口接收缓冲区大小
    uint16_t tx_buffer_size;            //串口发送缓冲区大小
    uint8_t dev_addr;                   //指纹模块地址
    fingerprint_event_cb_t event_cb;    //指纹模块事件回调
    void *user_data;                    //用户私有数据
}fingerprint_config_t;



//指纹模块初始化
fingerprint_err_t fingerprint_init(const fingerprint_config_t *config, fingerprint_handle_t *handle);

//指纹模块反初始化
fingerprint_err_t fingerprint_deinit(fingerprint_handle_t handle);

//指纹模块状态机
fingerprint_err_t fp_send_frame_machine(fingerprint_handle_t handle);




#ifdef __cplusplus
}
#endif
