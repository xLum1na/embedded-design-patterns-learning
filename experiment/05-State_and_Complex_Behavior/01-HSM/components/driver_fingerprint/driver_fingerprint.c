#include <stdio.h>
#include "driver_fingerprint.h"
#include "esp_log.h"



static const char *TAG = "fp_driver";


//句柄定义
typedef struct fp_t {
    fp_config_t fingerprint;        //当前指纹模块配置

}fp_t;

//=============== 指纹模块内部串口数据包数据结构定义 ===============//
//数据包协议常量定义
#define FP_FRAME_HEAD       0xFD    //帧头
#define FP_FRAME_TAIL       0xDF    //帧尾
#define FP_FRAME_MIN_LEN    5       //最小帧长度：FD + CMD + P1 + P2 + DF

#define FP_ID_MIN           0x01    //指纹ID最小值
#define FP_ID_MAX           0x80    //指纹ID最大值（1-80，共80个）

//数据包结构定义
typedef struct {
    uint8_t head;                   //帧头 0xFD
    uint8_t cmd;                    //命令码
    uint8_t param1;                 //参数1
    uint8_t param2;                 //参数2
    uint8_t tail;                   //帧尾 0xDF
} fp_frame_t;

//=============== 指纹模块内部API ===============//
//发送数据包
static esp_err_t fp_send_packet(fp_handle_t handle, uint8_t cmd, 
                                uint8_t param1, uint8_t param2)
{
    //参数检测
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    //构成数据协议包
    fp_frame_t packet = {
        .head = FP_FRAME_HEAD,
        .cmd = cmd,
        .param1 = param1,
        .param2 = param2,
        .tail = FP_FRAME_TAIL
    };
    //发送数据包
    int tx_len = uart_write_bytes(handle->fingerprint.uart_port, (void*)&packet, 
                                sizeof(packet));
    if (tx_len != sizeof(packet)) {
        return ESP_ERR_INVALID_SIZE;
    }
    //阻塞等待串口发送完成
    esp_err_t ret = uart_wait_tx_done(handle->fingerprint.uart_port, portMAX_DELAY);
    if (ret != ESP_OK) {
        return ret;
    }
    return ESP_OK;
}

static esp_err_t fp_receive_packet(fp_handle_t handle, uint8_t *data)
{
    if (handle == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t rx_buffer[10];
    //非阻塞读取FIFO中数据
    //阻塞等待返回数据
    int len = uart_read_bytes(handle->fingerprint.uart_port, 
                        rx_buffer, sizeof(rx_buffer), 
                        pdMS_TO_TICKS(100));
    //解包
    if (len >= 5) {
        *data = rx_buffer[2]; 
        //处理完数据后，开启中断
        return ESP_OK;
    } else if (len == 1) {
        *data = rx_buffer[0];
        return ESP_OK;
    }
    //超时
    return ESP_ERR_TIMEOUT;
}

// GPIO 中断处理函数
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    //有指纹开始识别产生中断，中断通知识别开始向外发送开始识别事件，触发后上层调用开始验证函数，通过验证函数来判断是否成功
    
}

//=============== 指纹模块外部API ===============//
//串口初始化
esp_err_t fp_init(const fp_config_t *config, fp_handle_t *handle)
{
    if (config == NULL || handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    //创建返回句柄
    fp_handle_t ret_handle = malloc(sizeof(fp_t));
    if (!ret_handle) {
        return ESP_ERR_NO_MEM;
    }
    //初始化uart
    //创建uart程序
    esp_err_t ret = uart_driver_install(config->uart_port, config->rx_buffer_size,  
                                        config->tx_buffer_size, 0, NULL, 0);

    if (ret != ESP_OK) {
        return ESP_FAIL;
    }
    //配置串口参数
    uart_config_t uart_cfg = {
        .baud_rate = config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
    };
    
    ret = uart_param_config(config->uart_port, &uart_cfg);
    if (ret != ESP_OK) {
        uart_driver_delete(config->uart_port);
        return ESP_FAIL;
    }
    //设置串口IO
    ret = uart_set_pin(config->uart_port, 
                    config->uart_txd, 
                    config->uart_rxd, 
                    UART_PIN_NO_CHANGE, // RTS
                    UART_PIN_NO_CHANGE); // CTS
    if (ret != ESP_OK) {
        uart_driver_delete(config->uart_port);
        return ESP_FAIL;
    }
    //初始化输入中断IO
    gpio_config_t io_cfg = {
        .pin_bit_mask = 1ULL << config->fp_vt,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&io_cfg);
    //注册中断函数
    gpio_install_isr_service(0);
    gpio_isr_handler_add(config->fp_vt, gpio_isr_handler, (void*)config->fp_vt);

    //返回句柄
    ret_handle->fingerprint = *config;
    *handle = ret_handle;

    return ESP_OK;
}

//指纹模块反初始化
esp_err_t fp_deinit(fp_handle_t handle)
{
    //删除串口端口
    if (!handle->fingerprint.uart_port || !handle) {
        return ESP_ERR_INVALID_ARG;
    }
    uart_driver_delete(handle->fingerprint.uart_port);
    free(handle);
    return ESP_OK;
}

//开启关闭指纹模块
esp_err_t fp_set_enable(fp_handle_t handle, bool enable)
{
    if (!handle) {
         ESP_LOGW(TAG, "fp arg fail\n");
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    uint8_t data;
    //判断是发送开启还是关闭
    if (enable) {
        //开启指令
        ret = fp_send_packet(handle, 0xE1, 0x00, 0x01);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "fp send fail\n");
            return ret;
        }
        ret = fp_receive_packet(handle, &data);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "fp receive fail\n");
            return ret;
        }
        ESP_LOGI(TAG, "fp receive packet success ");
        //判断成功或者失败 0xe1 -> 成功     0xb0 -> 无效指令
        if (data == 0xE1) {
            return ESP_OK;
        }
    } else {
        //发送关闭数据包{0xFD 0xE1 0x00 0x00 0xDF}
        ret = fp_send_packet(handle, 0xE1, 0x00, 0x00);
        if (ret != ESP_OK) {
            return ret;
        }
        //接收返回数据包
        ret = fp_receive_packet(handle, &data);
        if (ret != ESP_OK) {
            return ret;
        }
        //判断成功或者失败 0xe1 -> 成功     0xb0 -> 无效指令
        if (data == 0xE1) {
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

//开启关闭指纹灯光
esp_err_t fp_set_light_enable(fp_handle_t handle, bool enable)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    uint8_t data;
    if (enable) {
        //发送开启灯光数据包{0xFD 0xE1 0x00 0x03 0xDF}
        ret = fp_send_packet(handle, 0xE1, 0x00, 0x03);
        if (ret != ESP_OK) {
            return ret;
        }
        //接收返回数据包
        ret = fp_receive_packet(handle, &data);
        if (ret != ESP_OK) {
            return ret;
        }
        //判断成功或者失败 0xe1 -> 成功     0xb0 -> 无效指令
        if (data == 0xE1) {
            return ESP_OK;
        }
    } else {
        //发送关闭灯光数据包{0xFD 0xE1 0x00 0x02 0xDF}
        ret = fp_send_packet(handle, 0xE1, 0x00, 0x02);
        if (ret != ESP_OK) {
            return ret;
        }
        //接收返回数据包
        ret = fp_receive_packet(handle, &data);
        if (ret != ESP_OK) {
            return ret;
        }
        //判断成功或者失败 0xe1 -> 成功     0xb0 -> 无效指令
        if (data == 0xE1) {
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

//设置波特率
esp_err_t fp_set_baud_rate(fp_handle_t handle, fp_baud_rate_t baud_rate)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    //发送设置波特率数据包{0xFD 0xF1 0x00 0x0x 0xDF}
    switch (baud_rate) {
        case BAUD_TATE_LOW_SPEED:
            //{0xFD 0xF1 0x00 0x01 0xDF}   4800
            ret = fp_send_packet(handle, 0xF1, 0x00, 0x01);
            if (ret != ESP_OK) {
                return ret;
            }
            break;
        case BAUD_TATE_MEDIUM_SPEED:
            //{0xFD 0xF1 0x00 0x02 0xDF}   9600
            ret = fp_send_packet(handle, 0xF1, 0x00, 0x02);
            if (ret != ESP_OK) {
                return ret;
            }
            break;
        case BAUD_TATE_HIGH_SPEED:
            //{0xFD 0xF1 0x00 0x03 0xDF}   57600
            ret = fp_send_packet(handle, 0xF1, 0x00, 0x03);
            if (ret != ESP_OK) {
                return ret;
            }
            break;
        case BAUD_TATE_SUPER_SPEED:
            //{0xFD 0xF1 0x00 0x04 0xDF}   115200
            ret = fp_send_packet(handle, 0xF1, 0x00, 0x04);
            if (ret != ESP_OK) {
                return ret;
            }
            break;
        default:
            break;
    }
    //接收返回数据包
    uint8_t data;
    ret = fp_receive_packet(handle, &data);
    if (ret != ESP_OK) {
        return ret;
    }
    //判断成功或者失败 0xF1 -> 成功     0xB0 -> 无效指令
    if (data == 0xF1) {
        return ESP_OK;
    }
    //设置失败
    return ESP_FAIL;
}

//设置工作模式
esp_err_t fp_set_mode(fp_handle_t handle, fp_mode_t fp_mode)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    switch (fp_mode) {
        case REAL_TIME_MODE:
            //{0xFD 0xC1 0x00 0x01 0xDF}   实时模式
            ret = fp_send_packet(handle, 0xC1, 0x00, 0x01);
            if (ret != ESP_OK) {
                return ret;
            }
            break;
        case SELF_LOCK_MODE:
            //{0xFD 0xC1 0x00 0x02 0xDF}   自锁模式
            ret = fp_send_packet(handle, 0xC1, 0x00, 0x01);
            if (ret != ESP_OK) {
                return ret;
            }
            break;
        case ONE_JOG_MODE:
            //{0xFD 0xC1 0x00 0x03 0xDF}   1秒点动模式
            ret = fp_send_packet(handle, 0xC1, 0x00, 0x03);
            if (ret != ESP_OK) {
                return ret;
            }
            break;
        case FIVE_JOG_MODE:
            //{0xFD 0xC1 0x00 0x04 0xDF}   5秒点动模式
            ret = fp_send_packet(handle, 0xC1, 0x00, 0x04);
            if (ret != ESP_OK) {
                return ret;
            }
            break;
        case TEN_JOG_MODE:
            //{0xFD 0xC1 0x00 0x05 0xDF}   十秒点动模式
            ret = fp_send_packet(handle, 0xC1, 0x00, 0x05);
            if (ret != ESP_OK) {
                return ret;
            }
            break;
        case THIRTY_JOG_MODE:
            //{0xFD 0xC1 0x00 0x06 0xDF}   三十秒点动模式
            ret = fp_send_packet(handle, 0xC1, 0x00, 0x06);
            if (ret != ESP_OK) {
                return ret;
            }
            break;
        case SIXTY_JOG_MODE:
            //{0xFD 0xC1 0x00 0x07 0xDF}   六十秒点动模式
            ret = fp_send_packet(handle, 0xC1, 0x00, 0x07);
            if (ret != ESP_OK) {
                return ret;
            }
            break;
        case ONE_HANDRED_AND_TWENTY_MODE:
            //{0xFD 0xC1 0x00 0x08 0xDF}   一百二十秒点动模式
            ret = fp_send_packet(handle, 0xC1, 0x00, 0x08);
            if (ret != ESP_OK) {
                return ret;
            }
            break;
        default:
            break;
    }
    //接收数据包
    uint8_t data;
    ret = fp_receive_packet(handle, &data);
    if (ret != ESP_OK) {
        return ret;
    }
    //判断成功或者失败 0xC1 -> 成功     0xB0 -> 无效指令
    if (data == 0xC1) {
        return ESP_OK;
    }
    //设置失败
    return ESP_FAIL;

}

//注册指纹
esp_err_t fp_enroll(fp_handle_t handle, uint8_t fp_id)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    //发送注册指纹数据包
    //{0xFD 0xA1 0x00 0xxx 0xDF}   0xxx xx为注册指纹号
    //判断是否注册过次指纹 调用识别指纹后根据输出判断
    uint8_t id;
    ret = fp_verify(handle, &id);
    if (ret != ESP_OK) {
        return ret;
    }
    if (id) {
        return ESP_FAIL;
    }
    ret = fp_send_packet(handle, 0xA1, 0x00, fp_id);
    if (ret != ESP_OK) {
        return ret;
    }
    //接收数据包
    uint8_t data;
    ret = fp_receive_packet(handle, &data);
    if (ret != ESP_OK) {
        return ret;
    }
    //判断成功或者失败 0xA1 -> 成功     0xB0 -> 无效指令
    if (data == 0xA1) {
        return ESP_OK;
    }
    //设置失败
    return ESP_FAIL;
}

//删除指纹
esp_err_t fp_delete(fp_handle_t handle, uint8_t fp_id)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    //发送删除指纹数据包
    //{0xFD 0xB1 0x00 0xxx 0xDF}   0xxx xx为删除指纹号
    ret = fp_send_packet(handle, 0xB1, 0x00, fp_id);
    if (ret != ESP_OK) {
        return ret;
    }
    //接收数据包
    uint8_t data;
    ret = fp_receive_packet(handle, &data);
    if (ret != ESP_OK) {
        return ret;
    }
    //判断成功或者失败 0xB1 -> 成功     0xB0 -> 无效指令
    if (data == 0xB1) {
        return ESP_OK;
    }
    //设置失败
    return ESP_FAIL;

}

//删除所有指纹
esp_err_t fp_delete_all(fp_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    //发送删除所有指纹数据包
    //{0xFD 0xD) 0x00 0x00 0xDF}
    ret = fp_send_packet(handle, 0xD0, 0x00, 0x00);
    if (ret != ESP_OK) {
        return ret;
    }
    //接收数据包
    uint8_t data;
    ret = fp_receive_packet(handle, &data);
    if (ret != ESP_OK) {
        return ret;
    }
    //判断成功或者失败 0xD0 -> 成功     0xB0 -> 无效指令
    if (data == 0xD0) {
        return ESP_OK;
    }
    //设置失败
    return ESP_FAIL;
}

//指纹验证
esp_err_t fp_verify(fp_handle_t handle, uint8_t *fp_id)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    //接收数据包,获取对应指纹id
    uint8_t data;
    ret = fp_receive_packet(handle, &data);
    if (ret != ESP_OK) {
        return ret;
    }
    //判断成功或者失败 失败为E0
    if (data == 0xE0) {
        return ESP_FAIL;
    }
    //设置成功
    *fp_id = data;
    return ESP_OK;
}




