#include <stdio.h>
#include "max30102_driver.h"
#include "driver/i2c_master.h"
#include "esp_log.h" 
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAX30102";

//全局数据保存硬件读取原始数据
static uint32_t fifo_red;
static uint32_t fifo_ir;

//全局I2C句柄
static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t dev_handle = NULL;
static SemaphoreHandle_t i2c_mutex = NULL;


/***********************************************************
 * @brief 初始化 I2C 总线及设备
 * 
 * @param NULL
 * @return 
 *      - ESP_OK
 *      - ESP_ERR_INVALID_ARG
 *      - ESP_ERR_NO_MEM
 * 
 **********************************************************/
static esp_err_t max30102_i2c_init(void)
{
    if (bus_handle != NULL) {
        ESP_LOGW(TAG, "I2C Bus already initialized");
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = MAX_I2C_SCL,
        .sda_io_num = MAX_I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_mst_config, &bus_handle), TAG, "I2C New Master Bus Failed");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MAX30102_ADDR,    //从设备地址
        .scl_speed_hz = 100000,             //采样率100khz
        .scl_wait_us = 0,
        .flags.disable_ack_check = false,
    };

    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle), TAG, "I2C Add Device Failed");

    i2c_mutex = xSemaphoreCreateMutex();
    
    return ESP_OK;
}

/***********************************************************
 * @brief I2C反初始化
 * 
 * @param NULL
 * @return NULL
 * 
 * 
 **********************************************************/
static void max30102_i2c_deinit(void)
{
    if (dev_handle) {
        i2c_master_bus_rm_device(dev_handle);
        dev_handle = NULL;
    }
    if (bus_handle) {
        i2c_del_master_bus(bus_handle);
        bus_handle = NULL;
    }
}

/***********************************************************
 * @brief 写寄存器
 * 
 * @param[in] reg_addr 要写入的寄存器地址
 * @param[in] data 写入数据
 * @return 
 *      - ESP_OK
 *      - ESP_FAIL
 * 
 **********************************************************/
static esp_err_t max30102_write_reg(uint8_t reg_addr, uint8_t data)
{
    xSemaphoreTake(i2c_mutex, portMAX_DELAY);

    uint8_t write_buf[2] = {reg_addr, data};
    TickType_t timeout_ticks = pdMS_TO_TICKS(50); 

    esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, 2, timeout_ticks);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write Reg 0x%02X Failed: %s", reg_addr, esp_err_to_name(ret));
        return ret;
    }

    xSemaphoreGive(i2c_mutex);
    
    return ret;
}

/***********************************************************
 * @brief 读取寄存器值
 * 
 * @param[in] addr 要读取的寄存器地址
 * @param[out] data 获取的数据
 * @return 
 *      - ESP_OK
 *      - ESP_FAIL
 * 
 **********************************************************/
static esp_err_t max30102_read_reg(uint8_t addr, uint8_t *data)
{
    xSemaphoreTake(i2c_mutex, portMAX_DELAY);

    esp_err_t ret = i2c_master_transmit_receive(dev_handle, &addr, 1, data, 1, pdMS_TO_TICKS(100));
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read Reg 0x%02X Failed: %s", addr, esp_err_to_name(ret));
        return ret;
    }

    xSemaphoreGive(i2c_mutex);

    return ret;
}

/***********************************************************
 * @brief max30102软件复位
 * 
 * @param NULL
 * @return 
 *      - ESP_OK
 *      - ESP_FAIL
 * 
 **********************************************************/
static esp_err_t max30102_reset(void)
{
    ESP_LOGI(TAG, "Attempting MAX30102 reset...");
    
    // 读取复位前的模式配置
    uint8_t data;
    esp_err_t ret = max30102_read_reg(REG_MODE_CONFIG, &data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read pre-reset config");
        return ret;
    }
    ESP_LOGI(TAG, "Pre-reset MODE_CONFIG: 0x%02X", data);
    
    // 写入复位命令
    ret = max30102_write_reg(REG_MODE_CONFIG, 0x40);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write reset command");
        return ret;
    }
    ESP_LOGI(TAG, "Reset command written successfully");
    
    // 等待复位完成
    vTaskDelay(pdMS_TO_TICKS(100)); // 增加等待时间
    
    // 读取复位后的模式配置
    ret = max30102_read_reg(REG_MODE_CONFIG, &data);
    ESP_LOGI(TAG, "Post-reset MODE_CONFIG: 0x%02X", data);
    
    // 检查复位是否成功（复位位应该自动清除）
    if (data & 0x40) {
        ESP_LOGE(TAG, "Reset bit still set, reset may have failed");
        return ESP_FAIL;
    }
    
    // 验证设备是否响应
    ret = max30102_read_reg(REG_PART_ID, &data);
    ESP_LOGI(TAG, "Part ID after reset: 0x%02X", data);
    
    if (data != 0x15) {
        ESP_LOGE(TAG, "Invalid part ID after reset");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Reset successful");
    return ret;
}

/***********************************************************
 * @brief 配置max30102寄存器
 * 
 * @param NULL
 * @return 
 *      - ESP_OK
 *      - ESP_FAIL
 * 
 **********************************************************/
static esp_err_t max30102_config(void)
{
    // 1. 清除中断标志 (防止配置过程中产生误中断)
    max30102_write_reg(REG_INTR_STATUS_1, 0xFF);
    max30102_write_reg(REG_INTR_STATUS_2, 0xFF);

    // 2. 中断使能
    // 0xC0 = FIFO Almost Full (Bit 7) + FIFO New Data (Bit 6)
    max30102_write_reg(REG_INTR_ENABLE_1, 0xC0); 
    max30102_write_reg(REG_INTR_ENABLE_2, 0x00);

    // 3. 重置 FIFO 指针 (确保从干净状态开始)
    max30102_write_reg(REG_FIFO_WR_PTR, 0x00);
    max30102_write_reg(REG_OVF_COUNTER, 0x00);
    max30102_write_reg(REG_FIFO_RD_PTR, 0x00);
    
    // 4. FIFO 配置
    // 0x0F: Sample Average = 1 (No averaging), FIFO Rollover = 0 (Stop on overflow), Almost Full Level = 17
    max30102_write_reg(REG_FIFO_CONFIG, 0x0F); 
    
    // 5. 模式配置
    // 0x03: SpO2 Mode (Red + IR LED)
    max30102_write_reg(REG_MODE_CONFIG, 0x03); 
    
    // 6. SpO2 配置
    // 0x27: ADC Range = 4096nA, Sample Rate = 50Hz, Pulse Width = 411us (近似 400us)
    max30102_write_reg(REG_SPO2_CONFIG, 0x27); 

    // 7. LED 电流配置
    // 0x32 = 50 * 0.2mA = 10mA (根据实际硬件调整，过大会发热，过小信噪比低)
    max30102_write_reg(REG_LED1_PA, 0x32); 
    max30102_write_reg(REG_LED2_PA, 0x32); 
    // Pilot LED (通常用于接近检测，SpO2模式下可设为0或较小值)
    max30102_write_reg(REG_PILOT_PA, 0x00); 

    return ESP_OK;
}

/***********************************************************
 * @brief 读取 FIFO 数据并清除中断标志
 * 
 * @param NULL
 * @return 
 *      - ESP_OK
 *      - ESP_FAIL
 * 
 **********************************************************/
static esp_err_t max30102_read_fifo(void)
{
    uint8_t fifo_data[6];
    uint8_t reg_addr = REG_FIFO_DATA;
    
    // 原子操作：发送地址 + 读取6字节
    esp_err_t ret = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, fifo_data, 6, pdMS_TO_TICKS(10));

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C Read FIFO Failed: %s", esp_err_to_name(ret));
        fifo_red = 0;
        fifo_ir = 0;
        // 即使读取失败，也建议尝试清除中断标志，防止中断风暴
        max30102_write_reg(REG_INTR_STATUS_1, 0xFF); 
        return ret;
    }

    // --- 数据解析 ---
    // MAX30102 数据格式：[RED_H:2bit][RED_M:8bit][RED_L:8bit] [IR_H:2bit][IR_M:8bit][IR_L:8bit]
    uint32_t raw_red = ((uint32_t)(fifo_data[0] & 0x03) << 16) | 
                       ((uint32_t)fifo_data[1] << 8) | 
                       ((uint32_t)fifo_data[2]);
    
    uint32_t raw_ir = ((uint32_t)(fifo_data[3] & 0x03) << 16) | 
                      ((uint32_t)fifo_data[4] << 8) | 
                      ((uint32_t)fifo_data[5]);

    // 有效性检查 (饱和值 0x3FFFF = 524287)
    // 如果读到全 1 或异常大值，视为无效
    if (raw_red >= 0x3FFFF || raw_ir >= 0x3FFFF) {
        // 可能是手指未放好或溢出，视应用层需求决定是否清零
        // 这里选择保留原始值供调试，或者按需清零
        // fifo_red = 0; fifo_ir = 0; 
        ESP_LOGD(TAG, "Saturation detected: R=%lu, IR=%lu", raw_red, raw_ir);
    }
    
    fifo_red = raw_red;
    fifo_ir = raw_ir;

    // --- 关键步骤：清除中断标志 ---
    // 读取 FIFO 后，必须向 INT_STATUS 寄存器写 1 来清除对应的中断位
    // 否则 INT 引脚将一直保持低电平
    max30102_write_reg(REG_INTR_STATUS_1, 0xFF); 
    max30102_write_reg(REG_INTR_STATUS_2, 0xFF);
    return ret;
}

/***********************************************************
 * @brief 硬件初始化
 * 
 * @param NULL
 * @return 
 *      - ESP_OK
 *      - ESP_FAIL
 * 
 **********************************************************/
esp_err_t driver_max30102_init(void)
{
    // I2C初始化
    esp_err_t ret = max30102_i2c_init();
    if (ret != ESP_OK) {
        return ret;
    }
    ESP_LOGI(TAG, "MAX30102 I2C init success"); 
    vTaskDelay(pdMS_TO_TICKS(10)); 

    //设备复位
    ret = max30102_reset();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MAX30102 Reset Failed");
        max30102_i2c_deinit(); // 清理资源
        return ret;
    }
    ESP_LOGI(TAG, "MAX30102 reset success");

    //配置设备
    ret = max30102_config();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MAX30102 config Failed");
        max30102_i2c_deinit(); // 清理资源
        return ret;
    }
    ESP_LOGI(TAG, "MAX30102 config success");
    
    ESP_LOGI(TAG, "MAX30102 Initialized Successfully");
    return ret;
}

/***********************************************************
 * @brief 读取原始数据
 * 
 * @param[out] out_raw 获取到的原始数据
 * @return 
 *      - ESP_OK
 *      - ESP_FAIL
 * 
 **********************************************************/
esp_err_t driver_max30102_read_raw(max30102_raw_data_t *out_raw)
{
    if (out_raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 读取 FIFO 会更新全局变量 fifo_red/fifo_ir
    max30102_read_fifo();
    
    out_raw->red = fifo_red;
    out_raw->ir = fifo_ir;
    
    return ESP_OK;
}