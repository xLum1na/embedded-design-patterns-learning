#include <string.h>
#include "ringbuffer.h"

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
esp_err_t ringbuffer_init(ringbuffer_t *rb, uint8_t *buffer, size_t buffer_size, size_t element_size)
{
    //检查参数有效性
    if (rb == NULL || buffer == NULL || buffer_size == 0 || element_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    //查看缓冲区是否难被存储元素整除
    if (buffer_size % element_size != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    //初始化缓冲区
    rb->buffer = buffer;
    rb->buffer_size = buffer_size;
    rb->element_size = element_size;
    rb->count = 0;
    rb->read_index = 0;
    rb->write_index = 0;

    return ESP_OK;
}

/***********************************************************
 * @brief 写数据入环形缓冲区
 * 
 * @param[out] rb 环形缓冲区
 * @param[in] data 要存入环形缓冲区的数据
 * @return 
 *      - ESP_ERR_NO_MEM 存储成功
 *      - ESP_FAIL 存储失败
 * 
 **********************************************************/
esp_err_t ringbuffer_push(ringbuffer_t *rb, const void *data)
{
    //查看缓冲区与数据是否有效
    if (rb == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    //缓冲区容量获取
    size_t capacity = rb->buffer_size / rb->element_size;
    //判断缓冲区是否为满 ESP_ERR_NO_MEM
    if (rb->count >= capacity) {
        return ESP_ERR_NO_MEM;
    }
    //计算写入缓冲区空间位置
    size_t offset = rb->write_index * rb->element_size;
    //存储数据进入缓冲区
    memcpy(rb->buffer + offset, data, rb->element_size);
    //更新写指针位置
    rb->write_index = (rb->write_index + 1) % capacity;
    rb->count++;
    //缓冲区未满 返回ESP_FAIL
    return ESP_FAIL;
}

/***********************************************************
 * @brief 读取环形缓冲区
 * 
 * @param[out] rb 环形缓冲区
 * @param[in] data 读取到的数据
 * @return 
 *      - ESP_OK 读取成功
 *      - ESP_ERR_NOT_FOUND 读取失败
 * 
 **********************************************************/
esp_err_t ringbuffer_pop(ringbuffer_t *rb, void *data)
{
    //检查参数有效性
    if (rb == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    //检查缓冲区是否为空 空返回 ESP_ERR_NOT_FOUND
    if (rb->count == 0) {
        return ESP_ERR_NOT_FOUND;
    }
    //获取缓冲区容量大小
    size_t capacity = rb->buffer_size / rb->element_size;
    //获取读指针位置
    size_t offset = rb->read_index * rb->element_size;
    //读取数据
    memcpy(data, rb->buffer + offset, rb->element_size);
    //更新读指针
    rb->read_index = (rb->read_index + 1) % capacity;
    rb->count--;
    //读取成功，缓冲区为空 返回ESP_OK
    return ESP_OK;
}