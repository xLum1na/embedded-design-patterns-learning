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
typedef struct fp_t *fp_handle_t;

//波特率定义
typedef enum {
    BAUD_TATE_LOW_SPEED     = 4800,
    BAUD_TATE_MEDIUM_SPEED  = 9600,
    BAUD_TATE_HIGH_SPEED    = 57600,
    BAUD_TATE_SUPER_SPEED   = 115200,
} fp_baud_rate_t;

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
} fp_mode_t;


//指纹模块配置
typedef struct fp_config_t {
    uart_port_t uart_port;              //串口端口号
    gpio_num_t uart_rxd;                //串口rx（接收）
    gpio_num_t uart_txd;                //串口tx（发送）
    gpio_num_t fp_vt;                   //指纹模块有指纹时输出高电平，无指纹时输出低电平
    fp_baud_rate_t baud_rate;           //串口波特率与模块波特率
    fp_mode_t fp_mode;                  //指纹模块工作模式
    uint16_t rx_buffer_size;            //串口接收缓冲区大小
    uint16_t tx_buffer_size;            //串口发送缓冲区大小
    uint8_t dev_addr;                   //指纹模块地址
    void *user_data;                    //用户私有数据
} fp_config_t;



//指纹模块初始化
esp_err_t fp_init(const fp_config_t *config, fp_handle_t *handle);

//指纹模块反初始化
esp_err_t fp_deinit(fp_handle_t handle);

//开启关闭指纹模块
esp_err_t fp_set_enable(fp_handle_t handle, bool enable);

//开启关闭指纹灯光
esp_err_t fp_set_light_enable(fp_handle_t handle, bool enable);

//设置波特率
esp_err_t fp_set_baud_rate(fp_handle_t handle, fp_baud_rate_t baud_rate);

//设置工作模式
esp_err_t fp_set_mode(fp_handle_t handle, fp_mode_t fp_mode);

//注册指纹
esp_err_t fp_enroll(fp_handle_t handle, uint8_t fp_id);

//删除指纹
esp_err_t fp_delete(fp_handle_t handle, uint8_t fp_id);

//删除所有指纹
esp_err_t fp_delete_all(fp_handle_t handle);

//指纹验证
esp_err_t fp_verify(fp_handle_t handle, uint8_t *fp_id);





#ifdef __cplusplus
}
#endif
