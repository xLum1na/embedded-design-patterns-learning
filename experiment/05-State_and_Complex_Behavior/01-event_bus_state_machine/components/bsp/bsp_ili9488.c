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
    sys_time_t              *time_if;           /* os 延时驱动接口 */
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
    uint8_t buffer[4];
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
                                spi_driver_t *spi_driver_if,
                                ledc_driver_t *ledc_driver_if,
                                gpio_driver_t *gpio_driver_if,
                                sys_time_t *time_drivr_if,
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
    if (time_drivr_if == NULL || 
        time_drivr_if->sys_set_delay_func == NULL) {
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
    ret_handle->time_if = time_drivr_if;
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
    if (handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    /* 硬件复位 */
    bsp_ili9488_reset(handle);
    /* 软件复位 */
    bsp_ili9488_writecommand(handle, 0x01, ILI9488MAXDELAY);
    if (handle->time_if && handle->time_if->sys_set_delay_func) {
            handle->time_if->sys_set_delay_func(5);
    } else {
        return ILI9488_ERR_NOT_INIT;
    }

    /* 退出睡眠模式*/
    bsp_ili9488_sleepout(handle);
    handle->time_if->sys_set_delay_func(120);
    /* 设置像素格式 */
    bsp_ili9488_writecommand(handle, 0x3A, ILI9488MAXDELAY); 
    bsp_ili9488_writedata8(handle, handle->dev_cfg.color, ILI9488MAXDELAY);
    handle->time_if->sys_set_delay_func(10);
    /* 设置显示方向 */
    bsp_ili9488_setdirection(handle, handle->dev_cfg.dir);
    /* 电源控制 */
    bsp_ili9488_writecommand(handle, 0xC0, ILI9488MAXDELAY); 
    bsp_ili9488_writedata8(handle, 0x11, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x09, ILI9488MAXDELAY);

    bsp_ili9488_writecommand(handle, 0xC1, ILI9488MAXDELAY); 
    bsp_ili9488_writedata8(handle, 0x41, ILI9488MAXDELAY);
    /* VCOM控制 */
    bsp_ili9488_writecommand(handle, 0xC5, ILI9488MAXDELAY); 
    bsp_ili9488_writedata8(handle, 0x00, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x12, ILI9488MAXDELAY);
    /* VDVI控制 */
    bsp_ili9488_writecommand(handle, 0xC6, ILI9488MAXDELAY); 
    bsp_ili9488_writedata8(handle, 0x33, ILI9488MAXDELAY);   
    /* GAMMA校正 */
    bsp_ili9488_writecommand(handle, 0xE0, ILI9488MAXDELAY); 
    bsp_ili9488_writedata8(handle, 0x0F, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x26, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x24, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x0B, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x0E, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x08, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x54, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0xA6, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x3C, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x0A, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x11, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x08, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x0A, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x1F, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x0F, ILI9488MAXDELAY);

    bsp_ili9488_writecommand(handle, 0xE1, ILI9488MAXDELAY); 
    bsp_ili9488_writedata8(handle, 0x00, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x19, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x1B, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x04, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x01, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x07, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x2B, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x59, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x43, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x05, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x0E, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x07, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x05, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x20, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, 0x0F, ILI9488MAXDELAY);

    /* 开启显示 */
    bsp_ili9488_displayon(handle);
    handle->time_if->sys_set_delay_func(60);

    /* 清屏 */
    bsp_ili9488_fillscreen(handle, 0x0000);
    return ILI9488_OK;
}

ili9488_err_t bsp_ili9488_deinit(bsp_ili9488_handle_t handle)
{
    if (handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    /* 关闭背光 */
    if (handle->ledc_if && handle->ledc_if->ledc_set_duty_func) {
        handle->ledc_if->ledc_set_duty_func(0); 
    } else {
        return ILI9488_ERR_NOT_INIT;
    }
    /* 进入睡眠模式 */
    bsp_ili9488_sleepin(handle);
    /* 更新状态 */
    handle->is_sleeping = true;

    return ILI9488_OK;
}

ili9488_err_t bsp_ili9488_reset(bsp_ili9488_handle_t handle)
{
    if (handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    if (handle->gpio_if && handle->gpio_if->gpio_set_reset_func) {
        handle->gpio_if->gpio_set_reset_func(1);
    } else {
        return ILI9488_ERR_NOT_INIT;
    }
    if (handle->time_if && handle->time_if->sys_set_delay_func) {
        handle->time_if->sys_set_delay_func(10);
    } else {
        return ILI9488_ERR_NOT_INIT;
    }
    handle->gpio_if->gpio_set_reset_func(0);
    handle->time_if->sys_set_delay_func(10);
    handle->gpio_if->gpio_set_reset_func(1);
    handle->time_if->sys_set_delay_func(120);
    return ILI9488_OK;
}

ili9488_err_t bsp_ili9488_setaddrwindow(bsp_ili9488_handle_t handle, uint16_t x1, 
                                        uint16_t y1, uint16_t x2, uint16_t y2)
{
    if (handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    /* 设置列地址 */
    bsp_ili9488_writecommand(handle, 0x2A, ILI9488MAXDELAY); 
    bsp_ili9488_writedata16(handle, x1, ILI9488MAXDELAY);
    bsp_ili9488_writedata16(handle, x2, ILI9488MAXDELAY);

    /* 设置地址 */
    bsp_ili9488_writecommand(handle, 0x2B, ILI9488MAXDELAY); 
    bsp_ili9488_writedata16(handle, y1, ILI9488MAXDELAY);
    bsp_ili9488_writedata16(handle, y2, ILI9488MAXDELAY);
    /* 准备写入显存 */
    bsp_ili9488_writecommand(handle, 0x2C, ILI9488MAXDELAY); 
    return ILI9488_OK;
}

ili9488_err_t bsp_ili9488_drawpixel(bsp_ili9488_handle_t handle, uint16_t x, 
                                    uint16_t y, uint16_t color)
{
    if (handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    if (x >= ILI9488_WIDTH || y >= ILI9488_HEIGHT) {
             return ILI9488_ERR_INVALID_ARG;
    }

    bsp_ili9488_setaddrwindow(handle, x, y, x, y);
    bsp_ili9488_writedata16(handle, color, ILI9488MAXDELAY);

    return ILI9488_OK;
}

ili9488_err_t bsp_ili9488_fillscreen(bsp_ili9488_handle_t handle, uint16_t color)
{
    if (handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }

    uint16_t width = ILI9488_WIDTH;
    uint16_t height = ILI9488_HEIGHT;
    
    bsp_ili9488_setaddrwindow(handle, 0, 0, width - 1, height - 1);

    uint32_t pixel_count = (uint32_t)width * height;
    static uint16_t s_fill_color; 
    s_fill_color = color;
    bsp_ili9488_writedata16_bulk(handle, &s_fill_color, pixel_count, ILI9488MAXDELAY);

    return ILI9488_OK;
}

ili9488_err_t bsp_ili9488_fillrect(bsp_ili9488_handle_t handle, uint16_t x, 
                                    uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }

    uint16_t x1 = x;
    uint16_t y1 = y;
    uint16_t x2 = x + w - 1;
    uint16_t y2 = y + h - 1;
    uint32_t pixel_count = w * h;
    
    if (x1 >= ILI9488_WIDTH || y1 >= ILI9488_HEIGHT) return ILI9488_ERR_INVALID_ARG;
    if (x2 >= ILI9488_WIDTH) x2 = ILI9488_WIDTH - 1;
    if (y2 >= ILI9488_HEIGHT) y2 = ILI9488_HEIGHT - 1;
    
    bsp_ili9488_setaddrwindow(handle, x1, y1, x2, y2);
    
    for (uint32_t i = 0; i < pixel_count; i++) {
        bsp_ili9488_writedata16(handle, color, ILI9488MAXDELAY);
    }
    return ILI9488_OK;
}


ili9488_err_t bsp_ili9488_setdirection(bsp_ili9488_handle_t handle, ili9488_direction_t diretion)
{
    if (handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }

    uint8_t madctl_value;
    
    switch (diretion) {
        case ILI9488_DIR_0:
            madctl_value = 0x48;
            break;
        case ILI9488_DIR_90:
            madctl_value = 0x88;
            break;
        case ILI9488_DIR_180:
            madctl_value = 0xE8;
            break;
        case ILI9488_DIR_270:
            madctl_value = 0x28;
            break;
        default:
            madctl_value = 0x48;
            break;
    }
    bsp_ili9488_writecommand(handle, 0x36, ILI9488MAXDELAY);
    bsp_ili9488_writedata8(handle, madctl_value, ILI9488MAXDELAY);
    return ILI9488_OK;
}

ili9488_err_t bsp_ili9488_displayon(bsp_ili9488_handle_t handle)
{
    if (handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    return bsp_ili9488_writecommand(handle, 0x29, ILI9488MAXDELAY);
}

ili9488_err_t bsp_ili9488_displayoff(bsp_ili9488_handle_t handle)
{
    if (handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    return bsp_ili9488_writecommand(handle, 0x28, ILI9488MAXDELAY);
}

ili9488_err_t bsp_ili9488_sleepin(bsp_ili9488_handle_t handle)
{
    if (handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    bsp_ili9488_writecommand(handle, 0x10, ILI9488MAXDELAY);
    handle->time_if->sys_set_delay_func(120);
    return ILI9488_OK;
}

ili9488_err_t bsp_ili9488_sleepout(bsp_ili9488_handle_t handle)
{
    if (handle == NULL) {
        return ILI9488_ERR_INVALID_ARG;
    }
    bsp_ili9488_writecommand(handle, 0x11, ILI9488MAXDELAY);
    handle->time_if->sys_set_delay_func(120);
    return ILI9488_OK;
}