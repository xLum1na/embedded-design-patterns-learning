#include <stdio.h>
#include "driver_fingerprint.h"


//句柄定义
typedef struct fingerprint_t {
    fingerprint_config_t fingerprint;       //当前指纹模块配置
    fingerprint_state_t current_state;      //当前指纹模块状态
    QueueHandle_t uart_evt_queue;               //uart事件句柄
}fingerprint_t;

//=============== 指纹模块内部串口数据包数据结构定义 ===============//
//数据包协议常量定义
#define FP_FRAME_HEAD       0xFD    //帧头
#define FP_FRAME_TAIL       0xDF    //帧尾
#define FP_FRAME_MIN_LEN    5       //最小帧长度：FD + CMD + P1 + P2 + DF
#define FP_FRAME_MAX_LEN    8       //最大帧长度（预留扩展）
#define FP_MAX_DATA_LEN     4       //数据域最大长度

#define FP_ID_MIN           0x00    //指纹ID最小值
#define FP_ID_MAX           0x80    //指纹ID最大值（0-80，共81个）

//队列定义
#define FP_UART_QUEUE_SIZE  8       //可装8个uart事件

//串口命令定义
typedef enum {
    // 波特率设置
    FP_CMD_SET_BAUD     = 0xF1,     //设置波特率
    
    // 指纹管理
    FP_CMD_ENROLL       = 0xA1,     //注册指纹
    FP_CMD_DELETE       = 0xB1,     //删除指定指纹
    FP_CMD_DELETE_ALL   = 0xD0,     //删除所有指纹
    
    // 工作模式
    FP_CMD_SET_MODE     = 0xC1,     //设置工作模式
    
    // 开关控制
    FP_CMD_CONTROL      = 0xE1,     //指纹/灯光控制
    
    // 识别结果（模块主动上报）
    FP_CMD_MATCH_OK     = 0xAA,     //匹配成功（后接ID和0xBB）
    FP_CMD_MATCH_FAIL   = 0xE0,     //匹配失败（E0 E0 E0）
    
    // 错误响应
    FP_CMD_INVALID      = 0xB0,     //无效指令
} fp_cmd_t;

//模块开关命令定义
typedef enum {
    FP_CTRL_FP_OFF      = 0x00,     //关闭指纹识别
    FP_CTRL_FP_ON       = 0x01,     //开启指纹识别
    FP_CTRL_LED_OFF     = 0x02,     //关闭指纹灯
    FP_CTRL_LED_ON      = 0x03,     //开启指纹灯
} fp_ctrl_t;

//数据包结构定义
typedef struct {
    uint8_t head;                   //帧头 0xFD
    uint8_t cmd;                    //命令码
    uint8_t param1;                 //参数1
    uint8_t param2;                 //参数2
    uint8_t tail;                   //帧尾 0xDF
} fp_frame_t;

//指纹模块数据包串口状态定义
typedef enum {
    FP_RX_STA_IDLE = 0,           // 等待帧头
    FP_RX_STA_HEAD,               // 收到帧头，接收命令
    FP_RX_STA_CMD,                // 收到命令，接收参数1
    FP_RX_STA_PARAM1,             // 收到参数1，接收参数2
    FP_RX_STA_PARAM2,             // 收到参数2，接收帧尾/扩展
    FP_RX_STA_EXT,                // 接收扩展数据（如匹配成功的BB）
    FP_RX_STA_TAIL,               // 等待帧尾
    FP_RX_STA_COMPLETE,           // 帧接收完成
} fp_rx_state_t;


//=============== 指纹模块内部API ===============//
//串口初始化
static fingerprint_err_t fp_uart_init(fingerprint_handle_t handle)
{
    if (!handle) {
        return FP_ERR_INVALID_ARG;
    }
    //创建uartcx程序
    esp_err_t ret = uart_driver_install(handle->fingerprint.uart_port, 
                                        handle->fingerprint.rx_buffer_size, 
                                        handle->fingerprint.tx_buffer_size, 
                                        FP_UART_QUEUE_SIZE, &handle->uart_evt_queue, 0);
    if (ret != ESP_OK) {
        return FP_FAIL;
    }
    //配置串口参数
    uart_config_t uart_cfg = {
        .baud_rate = handle->fingerprint.baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
    };
    ret = uart_param_config(handle->fingerprint.uart_port, &uart_cfg);
    if (ret != ESP_OK) {
        return FP_FAIL;
    }
    ret = uart_set_pin(handle->fingerprint.uart_port, 
                handle->fingerprint.uart_txd, 
                handle->fingerprint.uart_rxd, 
                -1, -1);
    if (ret != ESP_OK) {
        return FP_FAIL;
    }
    //串口中断事件配置
    uart_intr_config_t uart_intr = {
        .intr_enable_mask = UART_DATA | UART_PARITY_ERR,
        .rxfifo_full_thresh = 100,
        .rx_timeout_thresh = 10,
    };
    ret = uart_intr_config(handle->fingerprint.uart_port, &uart_intr);
    if (ret != ESP_OK) {
        return FP_FAIL;
    }
    //使能接收中断
    ret = uart_enable_rx_intr(handle->fingerprint.uart_port);
    if (ret != ESP_OK) {
        return FP_FAIL;
    }
    return FP_OK;
}

//发送数据包
static fingerprint_err_t fp_send_frame(fingerprint_handle_t handle, uint8_t cmd, 
                                        uint8_t param1, uint8_t param2)
{
    //参数检测
    if (handle == NULL || cmd == NULL || param1 == NULL || param2 == NULL) {
        return FP_ERR_INVALID_ARG;
    }
    //构成数据协议包
    fp_frame_t pakect = {
        .head = FP_FRAME_HEAD,
        .cmd = cmd,
        .param1 = param1,
        .param2 = param2,
        .tail = FP_FRAME_TAIL
    };
    //发送数据包
    int tx_len = uart_write_bytes(handle->fingerprint.uart_port, &pakect, 
                                sizeof(pakect) / sizeof(pakect.cmd));
    if (tx_len != sizeof(pakect) / sizeof(pakect.cmd)) {
        return FP_ERR_COMM;
    }
    //
    if (uart_wait_tx_done(handle->fingerprint.uart_port, portMAX_DELAY) != ESP_OK) {
        return FP_ERR_COMM;
    }
    return FP_OK;
}

//接收数据包

//串口发送数据状态机
static fingerprint_err_t fp_send_frame_machine(fingerprint_handle_t handle, uint8_t *rx_data)
{
    if (handle == NULL || rx_data == NULL) {
        return FP_ERR_INVALID_ARG;
    }
    //从rxfifon中读取数据

}




//=============== 指纹模块外部API ===============//





