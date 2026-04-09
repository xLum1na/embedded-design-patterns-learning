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
 *
 * @par Function List:
 *      - bsp_spi_init()            : 初始化spi外设与资源
 *      - bsp_spi_deinit()          : 反初始化spi外设与资源
 *      - bsp_spi_ili9488_write()   : ILI9488 SPI 写入函数
 *      - bsp_spi_ili9488_read()    : ILI9488 SPI 读取函数
 *      - bsp_spi_xpt2046_write()   : XPT2046 SPI 写入函数
 *      - bsp_spi_xpt2046_read()    : XPT2046 SPI 读取函数
 * 
 **************************************************/

/*=============== #include "bsp_driver_spi.h" ===============*/
#include "bsp_driver_spi.h"         /* 头文件导入 */
/*=============== #include "driver/spi_master.h" ===============*/
#include "driver/spi_master.h"      /* espidf spi 主机驱动导入 */
/*=============== #include "esp_log.h" ===============*/
#include "esp_log.h"                /* espidf 日志库导入*/
/*=============== #include "string.h" ===============*/
#include "string.h"                 /* 使用memcopy */
/*=============== #include "freertos/FreeRTOS.h ===============*/
#include "freertos/FreeRTOS.h"      /* 限制dma并发操作 */
/*=============== #include "freertos/semphr.h" ===============*/
#include "freertos/semphr.h"        


/*************** spi日志标志 ***************/
static const char *TAG = "bsp_driver_spi";

/*************** ili9488 spi句柄 ***************/
static spi_device_handle_t g_ili9488_spi_handle = NULL;
/*************** xpt2046 spi句柄 ***************/
static spi_device_handle_t g_xpt2046_spi_handle = NULL;

#if USE_DMA
/*************** dma缓冲区变量 ***************/
    static uint8_t *tx_dma_buf = NULL;
    static uint8_t *rx_dma_buf = NULL;
/*************** 互斥锁保护DMA缓冲区 ***************/
    static SemaphoreHandle_t g_dma_mutex = NULL;
#endif

/**
 * @brief   内部使用传输数据函数
 *
 * @param[in] 
 *
 * @return NULL
 *
 * @note 只能内部使用
 */
static bool _do_transfer(uint8_t dev_id, uint8_t *tx_data, uint8_t *rx_data, uint8_t len, int timeout_ms);

void bsp_spi_init(void)
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
    ili9488_dev_cfg.clock_speed_hz      =   BSP_ILI9488_FREQ_HZ;
    ili9488_dev_cfg.mode                =   3;
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
    xpt2046_dev_cfg.clock_speed_hz      =   BSP_XPT2046_FREQ_HZ;
    xpt2046_dev_cfg.mode                =   0;
    xpt2046_dev_cfg.spics_io_num        =   BSP_xpt2046_CS_PIN;
    xpt2046_dev_cfg.queue_size          =   QUEUE_SIZE;
    xpt2046_dev_cfg.flags               =   0;
    /* 添加 xpt2046 spi设备 */
    ret = spi_bus_add_device(BSP_SPI_HOST, &xpt2046_dev_cfg, &g_xpt2046_spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Add XPT2046 SPI device failed: %x", ret);
        return ;
    }
    /* 分配DMA缓冲区 */ 
#if USE_DMA
    tx_dma_buf = heap_caps_malloc(DMA_TX_BUFFER_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (tx_dma_buf == NULL) {
        ESP_LOGI(TAG, "tx_dma_buf malloc fail");
        return ;
    }
    rx_dma_buf = heap_caps_malloc(DMA_RX_BUFFER_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (rx_dma_buf == NULL) {
        ESP_LOGI(TAG, "rx_dma_buf malloc fail");
        return ;
    }
    if (g_dma_mutex == NULL) {
        g_dma_mutex = xSemaphoreCreateMutex();
        if (g_dma_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create DMA mutex");
            return ;
        }
    }
#endif
    /* 设备共用总线信息 */
    ESP_LOGI(TAG, "SPI Bus: MISO=%d, MOSI=%d, SCLK=%d", BSP_MISO_PIN, BSP_MOSI_PIN, BSP_SCLK_PIN);
    /* 设备信息 */
    ESP_LOGI(TAG, "ILI9488: CS=%d, FREQ=%d Hz", BSP_ILI9488_CS_PIN, BSP_ILI9488_FREQ_HZ);
    ESP_LOGI(TAG, "XPT2046: CS=%d, FREQ=%d Hz", BSP_xpt2046_CS_PIN, BSP_XPT2046_FREQ_HZ);
}

void bsp_spi_deinit(void)
{
    esp_err_t ret;
#if USE_DMA
    /* 释放dma内存 */
    if (tx_dma_buf != NULL) {
        heap_caps_free(tx_dma_buf);
        tx_dma_buf = NULL;
    }
    if (rx_dma_buf != NULL) {
        heap_caps_free(rx_dma_buf);
        rx_dma_buf = NULL;
    }
    /* 销毁互斥锁 */
    if (g_dma_mutex != NULL) {
        vSemaphoreDelete(g_dma_mutex);
        g_dma_mutex = NULL;
    }
#endif
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

bool bsp_spi_ili9488_write(uint8_t *data, uint8_t len, int timeout_ms)
{
    if (data == NULL || len == 0) {
        return false;
    }
    return _do_transfer(DEV_ID_ILI9488, data, NULL, len, timeout_ms);
}

bool bsp_spi_ili9488_read(uint8_t *data, uint8_t len, int timeout_ms)
{
    /* 参数检测 */
    if (data == NULL || len == 0) {
        return false;
    }
    if (len <= 64) {
        /* 读取小量数据使用栈数组 */
        uint8_t dummy_buf[len];
        memset(dummy_buf, 0, len);
        return _do_transfer(DEV_ID_ILI9488, dummy_buf, data, len, timeout_ms);
    } else {
        /* 读取大量数据使用堆内存 */
        uint8_t *dummy_buf = calloc(1, len);
        if (dummy_buf == NULL) {
            ESP_LOGE(TAG, "Failed to allocate dummy buffer for %d bytes", len);
            return false;
        }
        bool result = _do_transfer(DEV_ID_ILI9488, dummy_buf, data, len, timeout_ms);
        free(dummy_buf);
        return result;
    }
}

bool bsp_spi_xpt2046_write(uint8_t *data, uint8_t len, int timeout_ms)
{
    if (data == NULL || len == 0) {
        return false;
    }
    return _do_transfer(DEV_ID_xpt2046, data, NULL, len, timeout_ms);
}

bool bsp_spi_xpt2046_read(uint8_t *data, uint8_t len, int timeout_ms)
{
    /* 参数检测 */
    if (data == NULL || len == 0) {
        return false;
    }
    if (len <= 64) {
        /* 读取小量数据使用栈数组 */
        uint8_t dummy_buf[len];
        memset(dummy_buf, 0, len);
        return _do_transfer(DEV_ID_xpt2046, dummy_buf, data, len, timeout_ms);
    } else {
        /* 读取大量数据使用堆内存 */
        uint8_t *dummy_buf = calloc(1, len);
        if (dummy_buf == NULL) {
            ESP_LOGE(TAG, "Failed to allocate dummy buffer for %d bytes", len);
            return false;
        }
        bool result = _do_transfer(DEV_ID_xpt2046, dummy_buf, data, len, timeout_ms);
        free(dummy_buf);
        return result;
    }
}

static bool _do_transfer(uint8_t dev_id, uint8_t *tx_data, uint8_t *rx_data, uint8_t len, int timeout_ms)
{
    esp_err_t ret;
    TickType_t ticks_to_wait = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
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
    /*构造传输事务*/
    spi_transaction_t trans = {0};
    trans.length        = len * 8;      /* 单位为bit */
    trans.rxlength      = len * 8;      /* 接收长度 */
    trans.tx_buffer     = tx_data;      /* 发送缓冲区 */
    trans.rx_buffer     = rx_data;      /* 接收缓冲区 */
    trans.flags         = 0;
    /* 使用dma，判断参数，复制到dma内存中进行传输 */
#if USE_DMA
    /* 获取互斥锁保护DMA缓冲区 */
    if (xSemaphoreTake(g_dma_mutex, ticks_to_wait) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take DMA mutex");
        return false;
    }
    
    if (tx_data != NULL) {
        if (len > DMA_TX_BUFFER_SIZE) {
            ESP_LOGE(TAG, "TX length %d exceeds DMA buffer %d", len, DMA_TX_BUFFER_SIZE);
            xSemaphoreGive(g_dma_mutex);
            return false;
        }
        memcpy(tx_dma_buf, tx_data, len);
        trans.tx_buffer = tx_dma_buf;
    }
    if (rx_data != NULL) {
        if (len > DMA_RX_BUFFER_SIZE) {
            ESP_LOGE(TAG, "RX length %d exceeds DMA buffer %d", len, DMA_RX_BUFFER_SIZE);
            xSemaphoreGive(g_dma_mutex);
            return false;
        }
        trans.rx_buffer = rx_dma_buf;
    }
    xSemaphoreGive(g_dma_mutex);
#endif
    /* 使用队列非阻塞传输 */
    ret = spi_device_queue_trans(target_handle, &trans, ticks_to_wait);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI queue trans: %x", ret);
        return false;
    }
    /* 获取传输结果 */
    spi_transaction_t *presult_trans;
    ret = spi_device_get_trans_result(target_handle, &presult_trans, ticks_to_wait);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI get trans result failed: %x", ret);
        return false;
    }

#if USE_DMA
    /* 接收数据加锁 */
    if (rx_data != NULL && trans.rx_buffer == rx_dma_buf) {
        if (xSemaphoreTake(g_dma_mutex, ticks_to_wait) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to take DMA mutex for RX copy");
            return false;
        }
        memcpy(rx_data, rx_dma_buf, len);
        xSemaphoreGive(g_dma_mutex);
    }
#endif
    return true;
}