#include "sensor_service.h"
#include "max30102_driver.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "algorithm.h"  //算法库

static const char *TAG = "SENSOR_SVC";

// 全局状态
static bool is_initialized = false;
static uint32_t sample_count = 0; // 当前采集点数


// 使用静态数组保存历史数据，避免每次调用都清零
static struct compx s1[FFT_N]; 
static struct compx s2[FFT_N];

// 标定参数
#define CALIBRATION_OFFSET 47 

/***********************************************************
 * @brief 初始化服务
 * 
 * @param NULL
 * @return 
 *      - ESP_OK
 *      - ESP_FAIL
 * 
 **********************************************************/
esp_err_t sensor_service_init(void)
{
    esp_err_t ret = ESP_OK;
    if (is_initialized) {
        return ret;
    }

    ESP_LOGI(TAG, "Initializing Sensor Service...");
    
    // 1. 初始化底层驱动
    ret = driver_max30102_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Driver init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 2. 清空 FFT 缓冲区
    memset(s1, 0, sizeof(s1));
    memset(s2, 0, sizeof(s2));
    sample_count = 0;

    is_initialized = true;
    ESP_LOGI(TAG, "Sensor Service Ready");
    return ret;
}

/***********************************************************
 * @brief 获取最终数据
 * 
 * @param[out] out_data 最终数据
 * @return 
 *      - ESP_OK
 *      - ESP_FAIL
 *      - ESP_ERR_INVALID_STATE
 *      - ESP_ERR_INVALID_ARG
 * 
 **********************************************************/
esp_err_t sensor_service_get_data(sensor_data_t *out_data)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (out_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 初始化输出数据结构
    out_data->is_valid = false;
    out_data->heart_rate_bpm = 0;
    out_data->spo2_percent = 0.0f;

    // 1. 读取单个原始数据点
    max30102_raw_data_t raw;
    esp_err_t ret = driver_max30102_read_raw(&raw);
    if (ret != ESP_OK) {
        return ret;
    }

    //查看原始数据
    ESP_LOGI(TAG, "Raw: R=%lu, IR=%lu", raw.red, raw.ir);

    // 2. 简单的有效性检查 (防止饱和或无手指)
    if (raw.ir < 10000 || raw.ir > 0x3FFFF) {
        // 信号太弱或饱和，不存入缓冲区，直接返回
        // 不清空缓冲区，保留之前的数据等待新信号
        return ret; 
    }

    //FFT数据存储转换
    if (sample_count < FFT_N) {
        // 填充阶段
        s1[sample_count].real = (float)raw.red;
        s1[sample_count].imag = 0;
        s2[sample_count].real = (float)raw.ir;
        s2[sample_count].imag = 0;
        sample_count++;
    } else {
        // 缓冲区已满，移位：丢弃第 0 个，后面的前移，新的放最后
        // 优化：实际项目中建议使用 ring buffer 索引，避免 memcpy 开销
        memmove(&s1[0], &s1[1], sizeof(struct compx) * (FFT_N - 1));
        memmove(&s2[0], &s2[1], sizeof(struct compx) * (FFT_N - 1));
        
        s1[FFT_N - 1].real = (float)raw.red;
        s1[FFT_N - 1].imag = 0;
        s2[FFT_N - 1].real = (float)raw.ir;
        s2[FFT_N - 1].imag = 0;
    }

    // 如果还没存满，直接返回，等待下一次调用
    if (sample_count < FFT_N) {
        return ret;
    }

    // --- 缓冲区已满，开始计算 ---

    // 4. 直流分量去除 (DC Removal)
    float sum_red = 0, sum_ir = 0;
    for (int i = 0; i < FFT_N; i++) {
        sum_red += s1[i].real;
        sum_ir += s2[i].real;
    }
    float dc_red = sum_red / FFT_N;
    float dc_ir = sum_ir / FFT_N;

    for (int i = 0; i < FFT_N; i++) {
        s1[i].real -= dc_red;
        s2[i].real -= dc_ir;
        // imag 保持为 0
    }

    // 6. 执行 FFT (确保 algorithm.h 中的 FFT 函数是原地运算)
    FFT(s1);
    FFT(s2);

    // 7. 计算模值 (Magnitude)
    float ac_red_sum = 0;
    float ac_ir_sum = 0;
    
    // 通常心率在 0.5Hz - 4Hz 之间，对应 FFT 的低频段
    // 我们只计算有效频段的能量，忽略直流 (i=0) 和高频噪声
    int valid_range_end = FFT_N / 2; // 奈奎斯特频率
    
    for (int i = 1; i < valid_range_end; i++) {
        float mag_red = sqrtf(s1[i].real * s1[i].real + s1[i].imag * s1[i].imag);
        float mag_ir = sqrtf(s2[i].real * s2[i].real + s2[i].imag * s2[i].imag);
        
        // 更新数组为模值，方便后续找峰值
        s1[i].real = mag_red;
        s1[i].imag = 0;
        s2[i].real = mag_ir;
        s2[i].imag = 0;

        ac_red_sum += mag_red;
        ac_ir_sum += mag_ir;
    }

    // 8. 寻找峰值频率 (Heart Rate Frequency)
    // 限制搜索范围：假设心率 40bpm - 200bpm
    // 40bpm @ 50Hz sample, N=256 -> index approx: (40/60)*256/50 = 3.4
    // 200bpm -> index approx: 17
    // 为了安全，搜索范围设为 2 到 60 (覆盖极高心率或谐波)
    int max_idx_red = find_max_num_index(s1, 60); 
    int max_idx_ir = find_max_num_index(s2, 60);

    // 9. 验证一致性
    // 如果红光和红外光检测到的主频率不一致，说明信号不可靠
    int diff = abs(max_idx_red - max_idx_ir);
    
    // 检查峰值是否明显 (避免在噪声中找最大值)
    // 简单判断：峰值点的值是否大于平均值的 2 倍？(此处简化处理)
    if (diff <= 2 && max_idx_red > 2) { 
        // 频率一致，计算心率
        // HR = (Index * SampleRate * 60) / FFT_N
        float hr_float = (float)max_idx_red * SAMPLES_PER_SECOND * 60.0f / (float)FFT_N;
        
        // 过滤异常心率
        if (hr_float >= 40.0 && hr_float <= 200.0) {
            out_data->heart_rate_bpm = (uint16_t)(hr_float + 0.5f);
            
            // 计算血氧 (R 值)
            // R = (AC_IR / DC_IR) / (AC_Red / DC_Red)
            // 注意：这里的 AC 应该是峰值处的幅度，或者是总交流能量。
            // 经典算法常用峰值幅度比。
            float ac_red_peak = s1[max_idx_red].real;
            float ac_ir_peak = s2[max_idx_ir].real;
            
            if (ac_red_peak > 0 && dc_red > 0 && dc_ir > 0) {
                float R = (ac_ir_peak / dc_ir) / (ac_red_peak / dc_red);
                
                // 经验公式 (不同传感器系数不同，需校准)
                float spo2 = -45.060 * R * R + 30.354 * R + 94.845;
                
                if (spo2 >= 0 && spo2 <= 100) {
                    out_data->spo2_percent = (uint16_t)(spo2 + 0.5f);
                    out_data->is_valid = true;
                    
                    ESP_LOGI(TAG, "HR: %d, SpO2: %d%% (Idx:%d, R:%.2f)", 
                             out_data->heart_rate_bpm, out_data->spo2_percent, max_idx_red, R);
                }
            }
        }
    } else {
        ESP_LOGW(TAG, "Signal Mismatch or Weak (RedIdx:%d, IrIdx:%d, Diff:%d)", 
                 max_idx_red, max_idx_ir, diff);
    }

    return ret;
}
