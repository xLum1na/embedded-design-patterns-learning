#include <stdio.h>
#include "sensor_serive.h"
#include "mpu6050_driver.h"
#include "driver/i2c_master.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "common_defs.h"


static const char *TAG = "sensor_serive";

//i2c相关资源
static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t dev_handle = NULL;

//mpu6050相关资源
static mpu6050_handle_t mpu6050_handle = NULL;

/**********************************************
 * @brief I2C初始化
 * 
 * 
 ***********************************************/
static void mpu6050_i2c_init(void)
{
    ESP_LOGI(TAG, "init i2c dev and bus");
    //初始化i2c总线
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = MPU6050_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = MPU6050_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = MPU6050_I2C_FREQ,
        .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL
    };

    ESP_ERROR_CHECK(i2c_param_config(MPU6050_I2C_PORT, &i2c_conf));

    ESP_ERROR_CHECK(i2c_driver_install(MPU6050_I2C_PORT, i2c_conf.mode, 0, 0, 0));

}

/**********************************************
 * @brief I2C反初始化初始化
 * 
 * 
 ***********************************************/
static void mpu6050_i2c_deinit(void)
{
    ESP_LOGI(TAG, "rm i2c dev and bus");
    if (dev_handle != NULL) {
        ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));
    }
    if (bus_handle != NULL) {
        ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    }
}

/**********************************************
 * @brief mpu6050初始化
 * 
 * 
 * 
 * 
 * 
 ***********************************************/
static void mpu6050_init(void)
{
    ESP_LOGI(TAG, "sensor init");
    //初始化总线
    mpu6050_i2c_init();
    //创建mpu6050设备
    if (mpu6050_handle == NULL) {
        mpu6050_handle = mpu6050_create(MPU6050_I2C_PORT, MPU6050_I2C_ADDRESS);
    }
    //初始化设备
    uint8_t dev_id;
    ESP_ERROR_CHECK(mpu6050_config(mpu6050_handle, ACCE_FS_4G, GYRO_FS_500DPS));
    ESP_ERROR_CHECK(mpu6050_get_deviceid(mpu6050_handle, &dev_id));
    ESP_LOGI(TAG, "sensor id is %d", dev_id);
    ESP_ERROR_CHECK(mpu6050_wake_up(mpu6050_handle));
}

/**********************************************
 * @brief mpu6050反初始化
 * 
 * 
 * 
 * 
 * 
 ***********************************************/
static void mpu6050_deinit(void)
{
    ESP_LOGI(TAG, "sensor init");
    //删除mpu6050设备
    if (mpu6050_handle != NULL) {
        mpu6050_delete(mpu6050_handle);
    }
    
}


void sensor_task(void *arg)
{
    QueueHandle_t queue = (QueueHandle_t)arg;
    
    //传感器初始化
    mpu6050_init();
    
    //数据记录变量
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;
    complimentary_angle_t angles = {0}; 
    
    //定义发送包
    sensor_data_packet_t packet; 

    while (1) {
        //读取传感器数据
        //只有当加速度计和陀螺仪都读取成功时，才进行后续处理
        if (mpu6050_get_acce(mpu6050_handle, &acce) == ESP_OK &&
            mpu6050_get_gyro(mpu6050_handle, &gyro) == ESP_OK) {

            //执行互补滤波姿态解算
            esp_err_t result = mpu6050_complimentory_filter(mpu6050_handle, 
                                                           &acce, 
                                                           &gyro, 
                                                           &angles);
            
            if (result == ESP_OK) {
                //填充有效数据
                packet.type = DATA_TYPE_MPU6050;
                packet.roll = angles.roll;
                packet.pitch = angles.pitch;

                //发送到队列
                //如果队列满了，等待最多 10ms。如果还是发不出去，说明显示端处理太慢
                //这里选择阻塞一小会儿，防止数据积压，也可以设为 0 直接丢弃最新一帧
                if (xQueueSend(queue, &packet, pdMS_TO_TICKS(10)) != pdTRUE) {
                    ESP_LOGW(TAG, "Queue Full, dropping frame");
                }
            } else {
                ESP_LOGE(TAG, "Attitude calculation failed");
            }

        } else {
            ESP_LOGE(TAG, "Failed to read sensor data");
        }
        //20ms对应50Hz的采样率，适合姿态解算
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}

