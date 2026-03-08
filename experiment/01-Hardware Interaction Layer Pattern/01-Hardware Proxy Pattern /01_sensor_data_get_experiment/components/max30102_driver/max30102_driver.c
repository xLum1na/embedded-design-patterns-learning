#include <stdio.h>
#include "max30102_driver.h" // 确保此头文件中定义了寄存器地址宏和 MAX30102_ADDR
#include "driver/i2c_master.h"
#include "esp_log.h" 
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAX30102";

// 全局变量存储最新数据，建议在多任务环境下加锁，或通过参数返回
// 这里为了保持与你原代码风格一致，暂保留全局，但推荐通过 out_raw 返回
static uint32_t fifo_red = 0;
static uint32_t fifo_ir = 0; 

static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t dev_handle = NULL;

/**
 * @brief 初始化 I2C 总线及设备
 */
static esp_err_t max30102_i2c_init(void)
{
    if (bus_handle != NULL) {
        ESP_LOGW(TAG, "I2C Bus already initialized");
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 9,
        .sda_io_num = 8,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_mst_config, &bus_handle), TAG, "I2C New Master Bus Failed");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MAX30102_ADDR, // 确保宏定义为 0x57
        .scl_speed_hz = 100000,          // 建议提升至 400kHz 以提高采样率
        .scl_wait_us = 0,
        .flags.disable_ack_check = false,
    };

    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle), TAG, "I2C Add Device Failed");


    return ESP_OK;
}

/**
 * @brief 反初始化 I2C
 */
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

/**
 * @brief 写入寄存器 (修复了返回值忽略问题)
 * @return 0: 失败, 1: 成功
 */
static uint8_t max30102_write_reg(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    TickType_t timeout_ticks = pdMS_TO_TICKS(50); 
    
    esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, 2, timeout_ticks);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write Reg 0x%02X Failed: %s", reg_addr, esp_err_to_name(ret));
        return 0;
    }
    return 1;
}

/**
 * @brief 读取寄存器
 * @return 读取到的数据，若失败返回 0 (注意：无法区分数据0和错误，需结合上下文)
 */
static uint8_t max30102_read_reg(uint8_t addr)
{
    uint8_t data = 0;
    esp_err_t ret = i2c_master_transmit_receive(
        dev_handle, 
        &addr, 1, 
        &data, 1, 
        pdMS_TO_TICKS(50)
    );
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read Reg 0x%02X Failed: %s", addr, esp_err_to_name(ret));
        return 0; // 失败返回0
    }
    
    return data;
}

/**
 * @brief 软复位
 */
static uint8_t max30102_reset(void)
{
    ESP_LOGI(TAG, "Attempting MAX30102 reset...");
    
    // 读取复位前的模式配置
    uint8_t pre_reset = max30102_read_reg(REG_MODE_CONFIG);
    ESP_LOGI(TAG, "Pre-reset MODE_CONFIG: 0x%02X", pre_reset);
    
    // 写入复位命令
    if (!max30102_write_reg(REG_MODE_CONFIG, 0x40)) {
        ESP_LOGE(TAG, "Failed to write reset command");
        return 0;
    }
    ESP_LOGI(TAG, "Reset command written successfully");
    
    // 等待复位完成
    vTaskDelay(pdMS_TO_TICKS(100)); // 增加等待时间
    
    // 读取复位后的模式配置
    uint8_t post_reset = max30102_read_reg(REG_MODE_CONFIG);
    ESP_LOGI(TAG, "Post-reset MODE_CONFIG: 0x%02X", post_reset);
    
    // 检查复位是否成功（复位位应该自动清除）
    if (post_reset & 0x40) {
        ESP_LOGE(TAG, "Reset bit still set, reset may have failed");
        return 0;
    }
    
    // 验证设备是否响应
    uint8_t part_id = max30102_read_reg(REG_PART_ID);
    ESP_LOGI(TAG, "Part ID after reset: 0x%02X", part_id);
    
    if (part_id != 0x15) {
        ESP_LOGE(TAG, "Invalid part ID after reset");
        return 0;
    }
    
    ESP_LOGI(TAG, "Reset successful");
    return 1;
}

/**
 * @brief 配置传感器寄存器
 */
void max30102_config(void)
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
}

/**
 * @brief 读取 FIFO 数据并清除中断标志
 */
void max30102_read_fifo(void)
{
    uint8_t fifo_data[6];
    uint8_t reg_addr = REG_FIFO_DATA; // 通常为 0x07
    
    // 原子操作：发送地址 + 读取6字节
    esp_err_t ret = i2c_master_transmit_receive(
        dev_handle,             
        &reg_addr, 1,           
        fifo_data, 6,           
        pdMS_TO_TICKS(10)       // 缩短超时时间
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C Read FIFO Failed: %s", esp_err_to_name(ret));
        fifo_red = 0;
        fifo_ir = 0;
        // 即使读取失败，也建议尝试清除中断标志，防止中断风暴
        max30102_write_reg(REG_INTR_STATUS_1, 0xFF); 
        return;
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
}

/**
 * @brief 驱动入口初始化
 */
esp_err_t driver_max30102_init(void)
{
    esp_err_t ret = max30102_i2c_init();
    if (ret != ESP_OK) {
        return ret;
    }
    
    ESP_LOGI(TAG, "MAX30102 I2C init success"); 
    
    vTaskDelay(pdMS_TO_TICKS(10)); 



    if (!max30102_reset()) {
        ESP_LOGE(TAG, "MAX30102 Reset Failed");
        max30102_i2c_deinit(); // 清理资源
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "MAX30102 reset success");
    }
    
    max30102_config();
    
    // 验证 Part ID
    // 注意：如果通信失败，read_reg 返回 0，这里会捕获到
    uint8_t part_id = max30102_read_reg(0xFF);
    
    if (part_id != 0x15) {
        ESP_LOGE(TAG, "Part ID Mismatch! Read: 0x%02X, Expected: 0x15", part_id);
        ESP_LOGE(TAG, "Please check wiring, pull-up resistors, or power supply.");
        // 可以选择返回失败，强制停止
        return ESP_ERR_NOT_FOUND; 
    }
    
    ESP_LOGI(TAG, "MAX30102 Initialized Successfully (ID: 0x%02X)", part_id);
    return ESP_OK;
}

/**
 * @brief 读取原始数据接口
 */
esp_err_t driver_max30102_read_raw(max30102_raw_data_t *out_raw)
{
    if (out_raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 读取 FIFO 会更新全局变量 fifo_red/fifo_ir
    max30102_read_fifo();
    
    out_raw->red = fifo_red;
    out_raw->ir = fifo_ir;
    
    // 可选：如果数据为 0 且非饱和，可能意味着没有新数据，可根据 INT 状态判断
    // 但简单轮询模式下，直接返回即可
    
    return ESP_OK;
}