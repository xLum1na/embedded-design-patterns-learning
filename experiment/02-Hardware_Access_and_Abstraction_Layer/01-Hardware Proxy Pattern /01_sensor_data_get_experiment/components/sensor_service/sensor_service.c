#include "sensor_service.h"
#include "max30102_driver.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "algorithm.h"  // 算法库
#include "ringbuffer.h"

static const char *TAG = "SENSOR_SVC";

// 标定参数
#define CALIBRATION_OFFSET 47 

// 全局状态
static bool is_initialized = false;

// 底层数据存储数组 (Ring Buffer 的数据区)
static struct compx s1_data[FFT_N]; 
static struct compx s2_data[FFT_N];

//用于计算
static struct compx s1_fft_temp[FFT_N];
static struct compx s2_fft_temp[FFT_N];

ringbuffer_t s1_buffer;
ringbuffer_t s2_buffer;

/***********************************************************
 * @brief 初始化服务
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

    // 2. 初始化环形缓冲区 (替代 memset)
    ret = ringbuffer_init(&s1_buffer, (uint8_t *)s1_data, FFT_N * sizeof(struct compx), sizeof(struct compx));
        if (ret != ESP_OK) {
        ESP_LOGE(TAG, "RingBuffer s1 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

        // 4. 初始化环形缓冲区 s2 (你之前的代码可能漏了这个！)
    ret = ringbuffer_init(&s2_buffer, (uint8_t *)s2_data, FFT_N * sizeof(struct compx), sizeof(struct compx));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "RingBuffer s2 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    is_initialized = true;
    ESP_LOGI(TAG, "Sensor Service Ready (Optimized RingBuffer)");
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

    // 初始化输出
    out_data->is_valid = false;
    out_data->heart_rate_bpm = 0;
    out_data->spo2_percent = 0;

    // 1. 读取原始数据
    max30102_raw_data_t raw;
    esp_err_t ret = driver_max30102_read_raw(&raw);
    if (ret != ESP_OK) {
        return ESP_ERR_NOT_FOUND;
    }
    
    //ESP_LOGI(TAG, "Raw: R=%lu, IR=%lu", raw.red, raw.ir);

    // 2. 有效性检查
    if (raw.ir < 10000 || raw.ir > 0x3FFFF) {
        // 信号无效，跳过本次采集，不写入缓冲区
        return ESP_OK; 
    }

    // 3. 构造数据点
    struct compx new_s1 = {.real = (float)raw.red, .imag = 0};
    struct compx new_s2 = {.real = (float)raw.ir, .imag = 0};
    //存储原始数据进入环形缓冲区
    esp_err_t ret1_push = ringbuffer_push(&s1_buffer, &new_s1);
    esp_err_t ret2_push = ringbuffer_push(&s2_buffer, &new_s2);
    //查看缓冲区是否已满
    if ((ret1_push != ESP_ERR_NO_MEM) && (ret2_push != ESP_ERR_NO_MEM)) {
        return ESP_ERR_NOT_FOUND;
    }
    //读取缓冲区数据进行处理
    //将缓冲区数据存入计算区
    if (s1_buffer.count < FFT_N || s2_buffer.count < FFT_N) {
        // 数据还不够一帧，等待下一次采集积累
        return ESP_OK; 
    }

    bool pop_success = true;
    
    for (int i = 0; i < FFT_N; i++) {
        struct compx temp_s1, temp_s2;
        
        // 分别 Pop 到临时变量，确认成功后再存入数组
        if (ringbuffer_pop(&s1_buffer, &temp_s1) != ESP_OK) {
            pop_success = false;
            break;
        }
        if (ringbuffer_pop(&s2_buffer, &temp_s2) != ESP_OK) {
            pop_success = false;
            break;
        }
        
        // 存入工作数组
        s1_fft_temp[i] = temp_s1;
        s2_fft_temp[i] = temp_s2;
    }
    //对获取到的copmx数据进行处理
    struct compx *s1_work = s1_fft_temp;
    struct compx *s2_work = s2_fft_temp;

    // 6. 防御性检查：NaN/Inf
    bool input_valid = true;
    for(int i = 0; i < FFT_N; i++) {
        if (!isfinite(s1_work[i].real) || !isfinite(s2_work[i].real)) {
            input_valid = false;
            break;
        }
    }
    
    if (!input_valid) {
        ESP_LOGE(TAG, "⚠️ CRITICAL: Input data contains NaN/Inf!");
        // 可以选择重置缓冲区，这里简单丢弃
        return ESP_OK;
    }

    // 7. 直流分量去除 (DC Removal)
    float sum_red = 0.0f, sum_ir = 0.0f;
    for (int i = 0; i < FFT_N; i++) {
        sum_red += s1_work[i].real;
        sum_ir += s2_work[i].real;
    }
    
    float dc_red = sum_red / (float)FFT_N;
    float dc_ir = sum_ir / (float)FFT_N;

    // 防止 DC 为 0 或 NaN
    if (!isfinite(dc_red) || !isfinite(dc_ir) || dc_red <= 0 || dc_ir <= 0) {
        ESP_LOGW(TAG, "Invalid DC component detected.");
        return ESP_OK;
    }

    for (int i = 0; i < FFT_N; i++) {
        s1_work[i].real -= dc_red;
        s2_work[i].real -= dc_ir;
        s1_work[i].imag = 0.0f; 
        s2_work[i].imag = 0.0f;
    }

    // 8. 执行 FFT
    FFT(s1_work);
    FFT(s2_work);

    // 9. 检查 FFT 输出
    bool fft_valid = true;
    for(int i = 0; i < FFT_N / 2; i++) {
        if (!isfinite(s1_work[i].real) || !isfinite(s1_work[i].imag) ||
            !isfinite(s2_work[i].real) || !isfinite(s2_work[i].imag)) {
            fft_valid = false;
            break;
        }
    }
    if (!fft_valid) {
        ESP_LOGE(TAG, "⚠️ CRITICAL: FFT output contains NaN!");
        return ESP_OK;
    }

    // 10. 计算模值 (只计算前半部分频谱)
    int valid_range_end = FFT_N / 2;
    for (int i = 1; i < valid_range_end; i++) {
        float r_real = s1_work[i].real;
        float r_imag = s1_work[i].imag;
        float i_real = s2_work[i].real;
        float i_imag = s2_work[i].imag;

        float mag_red = sqrtf(fmaxf(0.0f, r_real * r_real + r_imag * r_imag));
        float mag_ir = sqrtf(fmaxf(0.0f, i_real * i_real + i_imag * i_imag));
        
        s1_work[i].real = mag_red;
        s1_work[i].imag = 0.0f;
        s2_work[i].real = mag_ir;
        s2_work[i].imag = 0.0f;
    }

    // 11. 动态计算峰值搜索范围
    // 心率范围：40 ~ 200 BPM
    // Index = Frequency * N / Fs
    // Frequency = BPM / 60
    // Max_Index = (200 / 60) * FFT_N / SAMPLES_PER_SECOND
    float min_hz = 45.0f / 60.0f;
    float max_hz = 200.0f / 60.0f;
    
    int search_min_idx = (int)(min_hz * FFT_N / SAMPLES_PER_SECOND);
    int search_max_idx = (int)(max_hz * FFT_N / SAMPLES_PER_SECOND);

    // 边界保护
    if (search_min_idx < 2) search_min_idx = 2;
    if (search_max_idx >= valid_range_end) search_max_idx = valid_range_end - 1;
    if (search_min_idx >= search_max_idx) {
        ESP_LOGE(TAG, "Invalid search range due to sample rate settings.");
        return ESP_OK;
    }

    // 12. 寻找峰值
    int max_idx_red = find_max_num_index(s1_work, search_min_idx, search_max_idx); 
    int max_idx_ir = find_max_num_index(s2_work, search_min_idx, search_max_idx);

    // 防止越界访问 (如果 find_max_num_index 失败返回 -1 或 0)
    if (max_idx_red < search_min_idx || max_idx_ir < search_min_idx) {
        ESP_LOGW(TAG, "Peak detection failed.");
        return ESP_OK;
    }

    float ac_red_peak = s1_work[max_idx_red].real;
    float ac_ir_peak = s2_work[max_idx_ir].real;

    // 13. 阈值与一致性判断
    float peak_threshold = 10.0f; // 根据实际信号幅度调整
    int diff = abs(max_idx_red - max_idx_ir);

    // 两个峰值索引必须接近，且幅值足够大
    if (diff <= 2 && 
        isfinite(ac_red_peak) && isfinite(ac_ir_peak) &&
        ac_red_peak > peak_threshold && ac_ir_peak > peak_threshold) { 
        
        // 计算心率
        float hr_float = (float)max_idx_red * SAMPLES_PER_SECOND * 60.0f / (float)FFT_N;
        
        if (hr_float >= 40.0 && hr_float <= 200.0) {
            out_data->heart_rate_bpm = (uint16_t)(hr_float + 0.5f);
            
            // 计算血氧
            // R = (AC_IR / DC_IR) / (AC_Red / DC_Red)
            float ratio_red = ac_red_peak / dc_red;
            float ratio_ir = ac_ir_peak / dc_ir;
            
            if (ratio_red > 0 && ratio_ir > 0) {
                float R = ratio_ir / ratio_red;
                
                // R 值合理性检查 (通常 0.4 ~ 3.0)
                if (R > 0.3f && R < 4.0f) {
                    float spo2 = -45.060 * R * R + 30.354 * R + 94.845;
                    
                    if (spo2 >= 0 && spo2 <= 100) {
                        out_data->spo2_percent = (uint16_t)(spo2 + 0.5f);
                        out_data->is_valid = true;
                        ESP_LOGI(TAG, "✅ SUCCESS: HR: %d, SpO2: %d%% (R=%.2f)", 
                                 out_data->heart_rate_bpm, out_data->spo2_percent, R);
                        return ESP_OK;
                    }
                } else {
                    ESP_LOGW(TAG, "R value out of range: %.2f", R);
                }
            }
        } else {
            ESP_LOGW(TAG, "Calculated HR out of range: %.1f", hr_float);
        }
    } else {
        ESP_LOGD(TAG, "Signal Mismatch: Diff=%d, RedPeak=%.1f, IrPeak=%.1f", diff, ac_red_peak, ac_ir_peak);
    }
    
    return ESP_OK;
}
