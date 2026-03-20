#pragma once 


#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include <stdlib.h> 
#include "esp_err.h"

/***********************************************************
 * 环形缓冲区数据结构定义
 **********************************************************/
typedef struct ringbuffer_t {
    uint8_t *buffer;
    size_t buffer_size;     //缓冲区存储空间大小 buffer_size = capacity * element_size
    size_t element_size;    //单个存储元素大小
    size_t write_index;    //写指针
    size_t read_index;     //读指针
    size_t count;           //储存计数
}ringbuffer_t;

/***********************************************************
 * @brief 环形缓冲区初始化
 * 
 * @param[out] rb 环形缓冲区
 * @param[in] buffer 环形缓冲区空间
 * @param[in] buffer_size 环形缓冲区大小
 * @param[in] element_size 缓冲区存储元素大小
 * @return 
 *      - ESP_OK 缓冲区满
 *      - ESP_FAIL 缓冲区未满
 * 
 **********************************************************/
esp_err_t ringbuffer_init(ringbuffer_t *rb, uint8_t *buffer, size_t buffer_size, size_t element_size);

/***********************************************************
 * @brief 写数据入环形缓冲区
 * 
 * @param[out] rb 环形缓冲区
 * @param[in] data 要存入环形缓冲区的数据
 * @return 
 *      - ESP_OK 存储成功
 *      - ESP_FAIL 存储失败
 * 
 **********************************************************/
esp_err_t ringbuffer_push(ringbuffer_t *rb, const void *data);

/***********************************************************
 * @brief 读取环形缓冲区
 * 
 * @param[out] rb 环形缓冲区
 * @param[in] data 读取到的数据
 * @return 
 *      - ESP_OK 读取成功
 *      - ESP_FAIL 读取失败
 * 
 **********************************************************/
esp_err_t ringbuffer_pop(ringbuffer_t *rb, void *data);

