/**************************************************
 * @file    bsp_ili9488.h
 * @brief   板级ili9488驱动声明
 * @author  xLumina
 * @version V1.0.0
 * @date    2026-4-9
 *
 * @par History:
 *   - V1.0.0 | 2026-4-9 | xLumina | 初始版本创建
 *
 * @par Function List:
 * 
 * 
 **************************************************/


/*===============  #include "bsp_ili9488.h" ===============*/
 #include "bsp_ili9488.h"



typedef struct bsp_ili9488_s {
    /* 设备配置层 (Configuration) */
    bsp_ili9488_config_t    dev_cfg;            /* 设备配置 */
    /* 依赖注入层 (Dependency Injection) */
    spi_driver_t            *spi_if;            /* spi 驱动接口 */
    ledc_driver_t           *ledc_if;           /* ledc 驱动接口 */
    gpio_driver_t           *gpio_if;           /* gpio 驱动接口 */
    /* 运行时状态层 (Runtime State) */
    bool                    is_sleeping;        /* 休眠 */
    /* 资源管理层 (Resources) */
    uint16_t                *frame_buffer;      /* 帧缓冲区 */
    /* 扩展层 (Extension) */
    void                    *user_data;         /* 用户私有数据 */
}bsp_ili9488_s;


//=============================================//
//                  内部函数                    //
//=============================================//
/**
 * @brief   发送命令到 ILI9488
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 * @param[in] cmd 控制 ILI9488 命令
 * @param[in] timeout_ms 超时时间（毫秒）
 *
 * @return 
 *      - ILI9488_OK
 *      - ILI9488_ERR_NOT_INIT
 *      - ILI9488_ERR_GPIO_FAIL
 *      - ILI9488_ERR_SPI_FAIL
 *
 * @note 内部函数
 */
static ili9488_err_t bsp_ili9488_writecommand(bsp_ili9488_handle_t handle, uint8_t cmd, int timeout_ms);

/**
 * @brief   发送8位数据到 ILI9488
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 * @param[in] data 数据 
 * @param[in] timeout_ms 超时时间（毫秒）
 *
 * @return 
 *      - ILI9488_OK
 *      - ILI9488_ERR_NOT_INIT
 *      - ILI9488_ERR_GPIO_FAIL
 *      - ILI9488_ERR_SPI_FAIL
 * @note 内部函数
 */
static ili9488_err_t bsp_ili9488_writedata8(bsp_ili9488_handle_t handle, uint8_t data, int timeout_ms);

/**
 * @brief 批量发送数据到 ILI9488
 * 
 * @param[in] handle ILI9488 句柄
 * @param[in] data 数据缓冲区指针
 * @param[in] len 数据长度（字节数）
 * @param[in] timeout_ms 超时时间（毫秒）
 * 
 * @return 执行结果
 *      - ILI9488_OK
 *      - ILI9488_ERR_NOT_INIT
 *      - ILI9488_ERR_GPIO_FAIL
 *      - ILI9488_ERR_SPI_FAIL
 *      - ILI9488_ERR_INVALID_ARG
 * 
 * @note 内部函数
 */
static ili9488_err_t bsp_ili9488_writedata8_bulk(bsp_ili9488_handle_t handle, uint8_t *data, 
                                                size_t len, int timeout_ms);

/**
 * @brief 发送16位数据到 ILI9488
 * 
 * @param[in] handle ILI9488 句柄
 * @param[in] data 16位数据
 * @param[in] timeout_ms 超时时间（毫秒）
 * 
 * @return 执行结果
 *      - ILI9488_OK
 *      - ILI9488_ERR_NOT_INIT
 *      - ILI9488_ERR_GPIO_FAIL
 *      - ILI9488_ERR_SPI_FAIL
 * 
 * @note 内部函数
 */
static ili9488_err_t bsp_ili9488_writedata16(bsp_ili9488_handle_t handle, uint16_t data, int timeout_ms);

/**
 * @brief 批量发送16位数据
 * 
 * @param[in] handle ILI9488 句柄
 * @param[in] data 16位数据缓冲区
 * @param[in] len 数据个数（16位为单位）
 * @param[in] timeout_ms 超时时间（毫秒）
 * 
 * @return 执行结果
 *      - ILI9488_OK
 *      - ILI9488_ERR_NOT_INIT
 *      - ILI9488_ERR_GPIO_FAIL
 *      - ILI9488_ERR_SPI_FAIL
 *      - ILI9488_ERR_INVALID_ARG
 * 
 * @note 内部函数
 */
static ili9488_err_t bsp_ili9488_writedata16_bulk(bsp_ili9488_handle_t handle,
                                            uint16_t *data,
                                            size_t len,
                                            int timeout_ms);



static ili9488_err_t bsp_ili9488_writecommand(bsp_ili9488_handle_t handle, uint8_t cmd, int timeout_ms)
{
    /* 参数检查 */
    if (handle == NULL || handle->gpio_if == NULL || handle->spi_if == NULL) {
        return ILI9488_ERR_NOT_INIT;
    }
    /* 检查函数指针 */
    if (handle->gpio_if->gpio_set_dc_func == NULL || 
        handle->spi_if->spi_write_func == NULL) {
        return ILI9488_ERR_NOT_INIT;
    }
    /* DC=0 表示传输命令 */
    if (!handle->gpio_if->gpio_set_dc_func(0)) {
        return ILI9488_ERR_GPIO_FAIL;
    }
    /* 发送命令 */
    if (!handle->spi_if->spi_write_func(&cmd, sizeof(cmd), timeout_ms)) {
        return ILI9488_ERR_SPI_FAIL;
    }

    return ILI9488_OK; 
}

static ili9488_err_t bsp_ili9488_writedata8(bsp_ili9488_handle_t handle, uint8_t data, int timeout_ms)
{
    /* 参数检查 */
    if (handle == NULL || handle->gpio_if == NULL || handle->spi_if == NULL) {
        return ILI9488_ERR_NOT_INIT;
    }
    /* 检查函数指针 */
    if (handle->gpio_if->gpio_set_dc_func == NULL || 
        handle->spi_if->spi_write_func == NULL) {
        return ILI9488_ERR_NOT_INIT;
    }
    /* DC=1 表示传输数据 */
    if (!handle->gpio_if->gpio_set_dc_func(1)) {
        return ILI9488_ERR_GPIO_FAIL;
    }
    /* 发送命令 */
    if (!handle->spi_if->spi_write_func(&data, sizeof(data), timeout_ms)) {
        return ILI9488_ERR_SPI_FAIL;
    }

    return ILI9488_OK;
}

static ili9488_err_t bsp_ili9488_writedata8_bulk(bsp_ili9488_handle_t handle, uint8_t *data, 
                                                size_t len, int timeout_ms)
{
    /* 参数检查 */
    if (handle == NULL || data == NULL || len == 0) {
        return ILI9488_ERR_INVALID_ARG;
    }
    if (handle->gpio_if == NULL || handle->spi_if == NULL) {
        return ILI9488_ERR_NOT_INIT;
    }
    /* 检查函数指针 */
    if (handle->gpio_if->gpio_set_dc_func == NULL || 
        handle->spi_if->spi_write_func == NULL) {
        return ILI9488_ERR_NOT_INIT;
    }
    /* DC=1 表示传输数据 */
    if (!handle->gpio_if->gpio_set_dc_func(1)) {
        return ILI9488_ERR_GPIO_FAIL;
    }
    /* 批量发送数据 */
    if (!handle->spi_if->spi_write_func(data, len, timeout_ms)) {
        return ILI9488_ERR_SPI_FAIL;
    }

    return ILI9488_OK;
}

static ili9488_err_t bsp_ili9488_writedata16(bsp_ili9488_handle_t handle, uint16_t data, int timeout_ms)
{
    /* 参数检查 */
    if (handle == NULL || handle->gpio_if == NULL || handle->spi_if == NULL) {
        return ILI9488_ERR_NOT_INIT;
    }
    /* 检查函数指针 */
    if (handle->gpio_if->gpio_set_dc_func == NULL || 
        handle->spi_if->spi_write_func == NULL) {
        return ILI9488_ERR_NOT_INIT;
    }
    /* DC=1 表示传输数据 */
    if (!handle->gpio_if->gpio_set_dc_func(1)) {
        return ILI9488_ERR_GPIO_FAIL;
    }
    /* 数据缓冲区 */
    uint8_t buffer[2];
    buffer[0] = (data >> 8) & 0xFF;   /* 高8位 */
    buffer[1] = data & 0xFF;          /* 低8位 */
    /* 发送数据 */
    if (!handle->spi_if->spi_write_func(buffer, 2, timeout_ms)) {
        return ILI9488_ERR_SPI_FAIL;
    }

    return ILI9488_OK;
}

static ili9488_err_t bsp_ili9488_writedata16_bulk(bsp_ili9488_handle_t handle,
                                            uint16_t *data,
                                            size_t len,
                                            int timeout_ms)
{
    /* 参数检查 */
    if (handle == NULL || data == NULL || len == 0) {
        return ILI9488_ERR_INVALID_ARG;
    }
    if (handle->gpio_if == NULL || handle->spi_if == NULL) {
        return ILI9488_ERR_NOT_INIT;
    }
    /* 检查函数指针 */
    if (handle->gpio_if->gpio_set_dc_func == NULL || 
        handle->spi_if->spi_write_func == NULL) {
        return ILI9488_ERR_NOT_INIT;
    }
    /* DC=1 表示传输数据 */
    if (!handle->gpio_if->gpio_set_dc_func(1)) {
        return ILI9488_ERR_GPIO_FAIL;
    }
    /* 批量发送数据 */
    if (!handle->spi_if->spi_write_func((uint8_t*)data, len * 2, timeout_ms)) {
        return ILI9488_ERR_SPI_FAIL;
    }

    return ILI9488_OK;
}


//=============================================//
//                  外部函数                    //
//=============================================//
ili9488_err_t ili9488_create(const bsp_ili9488_config_t *ili9488_cfg,
                                const spi_driver_t *spi_driver_if,
                                const ledc_driver_t *ledc_driver_if,
                                const gpio_driver_t *gpio_driver_if,
                                bsp_ili9488_handle_t *ili9488_handle)
{
    /* 参数检查 */
    if (ili9488_cfg == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    if (ili9488_handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    if (spi_driver_if == NULL || 
        spi_driver_if->spi_read_func == NULL || 
        spi_driver_if->spi_write_func == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    if (ledc_driver_if == NULL || 
        ledc_driver_if->ledc_set_duty_func == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    if (gpio_driver_if == NULL ||
        gpio_driver_if->gpio_set_dc_func == NULL ||
        gpio_driver_if->gpio_set_reset_func == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    bsp_ili9488_handle_t ret_handle = calloc(1, sizeof(bsp_ili9488_s));
    if (ret_handle == NULL) {
        return ILI9488_ERR_NO_MEM;
    }
    /* 保存驱动接口指针 */
    ret_handle->spi_if = spi_driver_if;
    ret_handle->ledc_if = ledc_driver_if;
    ret_handle->gpio_if = gpio_driver_if;
    /* 保存 ILI9488 配置*/
    ret_handle->dev_cfg = *ili9488_cfg;
    /* 运行时状态初始化*/
    ret_handle->is_sleeping = false;
    /* 保存用户数据 */
    ret_handle->user_data = ili9488_cfg->user_data;
    /* 返回句柄 */
    *ili9488_handle = ret_handle;
    
    return ILI9488_OK;
}

ili9488_err_t ili9488_delete(bsp_ili9488_handle_t *ili9488_handle)
{
    if (ili9488_handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    if (ili9488_handle != NULL) {
        free(*ili9488_handle);
        *ili9488_handle = NULL;
    }

    return ILI9488_OK;
}

ili9488_err_t bsp_ili9488_init(bsp_ili9488_handle_t handle)
{

    return ILI9488_OK;
}

ili9488_err_t bsp_ili9488_deinit(bsp_ili9488_handle_t handle)
{

    return ILI9488_OK;
}

ili9488_err_t bsp_ili9488_reset(bsp_ili9488_handle_t handle)
{

    return ILI9488_OK;
}