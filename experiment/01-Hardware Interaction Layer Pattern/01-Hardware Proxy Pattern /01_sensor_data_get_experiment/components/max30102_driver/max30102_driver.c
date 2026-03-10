#include <stdio.h>
#include "max30102_driver.h"
#include "esp_log.h" 
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

static const char *TAG = "MAX30102";

//全局数据保存硬件读取原始数据
static uint32_t fifo_red;
static uint32_t fifo_ir;

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
    if (i2c_mutex != NULL) {
        ESP_LOGW(TAG, "I2C already initialized");
        return ESP_OK;
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = MAX_I2C_SDA,
        .scl_io_num = MAX_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 50000,
    };

    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM_0, &conf), TAG, "I2C config failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0), TAG, "I2C install failed");

    i2c_mutex = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "I2C initialized (legacy driver)");
    
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
    i2c_driver_delete(I2C_NUM_0);
    if (i2c_mutex) {
        vSemaphoreDelete(i2c_mutex);
        i2c_mutex = NULL;
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

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write Reg 0x%02X Failed: %s", reg_addr, esp_err_to_name(ret));
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

    // 写寄存器地址
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, addr, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        xSemaphoreGive(i2c_mutex);
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(5));
    
    // 读数据
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

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
        ESP_LOGE(TAG, "Failed to read pre-reset config ");
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
    
    // 先检查 FIFO 中有多少样本
    uint8_t write_ptr, read_ptr;
    max30102_read_reg(REG_FIFO_WR_PTR, &write_ptr);
    max30102_read_reg(REG_FIFO_RD_PTR, &read_ptr);
    
    uint8_t num_samples = (write_ptr - read_ptr) & 0x1F;
    
    if (num_samples == 0) {
        // 没有新数据，返回上次值或 0
        return ESP_ERR_NOT_FOUND;
    }
    
    // 只读取最新样本（跳过旧的，避免延迟）
    for (int i = 0; i < num_samples; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (MAX30102_ADDR << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, REG_FIFO_DATA, true);
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (MAX30102_ADDR << 1) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, fifo_data, 5, I2C_MASTER_ACK);
        i2c_master_read_byte(cmd, fifo_data + 5, I2C_MASTER_NACK);
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);
        
        if (ret != ESP_OK) return ret;
    }
    
    // 解析（只保留最后一次读取的）
    fifo_red = ((uint32_t)(fifo_data[0] & 0x03) << 16) | 
               ((uint32_t)fifo_data[1] << 8) | fifo_data[2];
    fifo_ir = ((uint32_t)(fifo_data[3] & 0x03) << 16) | 
              ((uint32_t)fifo_data[4] << 8) | fifo_data[5];
    
    return ESP_OK;
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