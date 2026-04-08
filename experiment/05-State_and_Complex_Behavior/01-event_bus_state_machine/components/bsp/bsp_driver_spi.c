/**************************************************
 * @file    bsp_driver_spi.c
 * @brief   板级spi外设驱动实现
 * @author  xLumina
 * @version V1.0.0
 * @date    2026-4-8
 *
 * @par History:
 *   - V1.0.0 | 2026-4-8 | xLumina | 初始版本创建
 *
 * @par Function List:
 *      - bsp_spi_init()        : 初始化spi外设
 * 
 * 
 **************************************************/

/*=============== #include "bsp_driver_spi.h" ===============*/
#include "bsp_driver_spi.h"         /* 头文件导入 */
/*=============== #include "driver/spi_master.h" ===============*/
#include "driver/spi_master.h"      /* espidf spi 主机驱动导入 */
/*=============== #include "esp_log.h" ===============*/
#include "esp_log.h"                /* espidf 日志库导入*/


/*************** spi日志标志 ***************/
static const char *TAG = "bsp_driver_spi";

/*************** ili9488 spi句柄 ***************/
static spi_device_handle_t g_ili9488_spi_handle = NULL;
/*************** xpt2046 spi句柄 ***************/
static spi_device_handle_t g_xpt2046_spi_handle = NULL;


void bsp_spi_init(int ili9488_clk_speed, int xpt2046_clk_speed, uint8_t mode)
{
    esp_err_t ret;
    /* 配置spi主机 */
    spi_bus_config_t bus_cfg = {0};
    bus_cfg.mosi_io_num         =   BSP_MOSI_PIN;
    bus_cfg.miso_io_num         =   BSP_MISO_PIN;
    bus_cfg.sclk_io_num         =   BSP_SCLK_PIN;
    bus_cfg.quadwp_io_num       =   BSP_QUADWP_PIN;
    bus_cfg.quadhd_io_num       =   BSP_QUADHD_PIN;
    bus_cfg.max_transfer_sz     =   BSP_MAX_TRAN_SIZE;
    /* spi主机初始化 */
    ret = spi_bus_initialize(BSP_SPI_HOST, &bus_cfg, BSP_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %x", ret);
        return ;
    }
    /* 配置 ili9488 spi设备 */
    spi_device_interface_config_t ili9488_dev_cfg = {0};
    ili9488_dev_cfg.clock_speed_hz      =   ili9488_clk_speed;
    ili9488_dev_cfg.mode                =   mode;
    ili9488_dev_cfg.spics_io_num        =   BSP_ILI9488_CS_PIN;
    ili9488_dev_cfg.queue_size          =   QUEUE_SIZE;
    ili9488_dev_cfg.flags               =   0;
    /* 添加 ili9488 spi设备 */
    ret = spi_bus_add_device(BSP_SPI_HOST, &ili9488_dev_cfg, &g_ili9488_spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Add ILI9488 SPI device failed: %x", ret);
        return ;
    }
    /* 配置 xpt2046 spi设备 */
    spi_device_interface_config_t xpt2046_dev_cfg = {0};
    xpt2046_dev_cfg.clock_speed_hz      =   xpt2046_clk_speed;
    xpt2046_dev_cfg.mode                =   mode;
    xpt2046_dev_cfg.spics_io_num        =   BSP_xpt2046_CS_PIN;
    xpt2046_dev_cfg.queue_size          =   QUEUE_SIZE;
    xpt2046_dev_cfg.flags               =   0;
    /* 添加 xpt2046 spi设备 */
    ret = spi_bus_add_device(BSP_SPI_HOST, &xpt2046_dev_cfg, &g_xpt2046_spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Add XPT2046 SPI device failed: %x", ret);
        return ;
    }
    ESP_LOGI(TAG, "SPI init done. LCD: %d Hz, Touch: %d Hz", ili9488_clk_speed, xpt2046_clk_speed);
}

void bsp_spi_deinit(void)
{
    esp_err_t ret;
    /* 移除 ili9488 设备 */
    if (g_ili9488_spi_handle != NULL) {
        ret = spi_bus_remove_device(g_ili9488_spi_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to remove ILI9488 device: %x", ret);
        } else {
            g_ili9488_spi_handle = NULL; /* 清空句柄指针 */
            ESP_LOGI(TAG, "ILI9488 device removed");
        }
    }
    /* 移除 XPT2046 设备 */
    if (g_xpt2046_spi_handle != NULL) {
        ret = spi_bus_remove_device(g_xpt2046_spi_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to remove XPT2046 device: %x", ret);
        } else {
            g_xpt2046_spi_handle = NULL; /* 清空句柄指针 */
            ESP_LOGI(TAG, "XPT2046 device removed");
        }
    }
    /* 释放spi总线 */
    ret = spi_bus_free(BSP_SPI_HOST);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to free SPI bus: %x", ret);
    } else {
        ESP_LOGI(TAG, "SPI Bus freed");
    }
}

bool bsp_spi_write_byte(uint8_t dev_id, uint8_t addr, uint8_t data)
{
    /* 错误码定义 */
    esp_err_t ret;
    /* 用于设备句柄切换 */
    spi_device_handle_t target_handle = NULL;
    /* 根据传入设备ID进行句柄转换 */
    switch (dev_id) {
        case DEV_ID_ILI9488:
            target_handle = g_ili9488_spi_handle;
            break;
        case DEV_ID_xpt2046:
            target_handle = g_xpt2046_spi_handle;
            break;
        default:
            ESP_LOGE(TAG, "Unknown device ID: 0x%x", dev_id);
            return false;
    }
    /* 构造发送缓冲区 */
    uint8_t tx_buf[] = {addr, data};
    /* 传输事务配置 */
    spi_transaction_t t = {0};
    t.length = 8 * 2;           /* 传输2个字节 */
    t.tx_buffer = tx_buf;       /* 发送缓冲区 */
    t.rx_buffer = NULL;         /* 不接收 */
    t.flags = 0;    
    /* 开始传输 （阻塞式传输） */
    ret = spi_device_polling_transmit(target_handle, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI write failed: %x", ret);
        return false;
    }
    return true;
}

bool bsp_spi_read_byte(uint8_t dev_id, uint8_t addr, uint8_t *data)
{
    /* 参数检测 */
    if (data == NULL) {
        return false;
    }
    /* 错误码定义 */
    esp_err_t ret;
    /* 用于设备句柄切换 */
    spi_device_handle_t target_handle = NULL;
    /* 根据传入设备ID进行句柄转换 */
    switch (dev_id) {
        case DEV_ID_ILI9488:
            target_handle = g_ili9488_spi_handle;
            break;
        case DEV_ID_xpt2046:
            target_handle = g_xpt2046_spi_handle;
            break;
        default:
            ESP_LOGE(TAG, "Unknown device ID: 0x%x", dev_id);
            return false;
    }
    /* 构造传输缓冲区 */
    uint8_t tx_buf[] = {addr, 0xFF};    /* 发送缓冲区 */
    uint8_t rx_buf[] = {0, 0};          /* 接收缓冲区 */
    /* 构造传输事务 */
    spi_transaction_t t = {0};
    t.length = 8 * 2;                   /* 传输2个字节 */
    t.tx_buffer = tx_buf;               /* 发送 */
    t.rx_buffer = rx_buf;               /* 接收 */
    /* 开始传输 （阻塞式传输） */
    ret = spi_device_polling_transmit(target_handle, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI read failed: %x", ret);
        return false;
    }
    /* 获取接收结果 */
    *data = rx_buf[1];

    return true;
}