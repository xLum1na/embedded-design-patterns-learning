// #include "sensor_service.h"
// #include "max30102_driver.h"
// #include "esp_log.h"
// #include <string.h>
// #include <math.h>
// #include "freertos/FreeRTOS.h"
// #include "algorithm.h"  //算法库

// static const char *TAG = "SENSOR_SVC";


// // 标定参数
// #define CALIBRATION_OFFSET 47 

// // 全局状态
// static bool is_initialized = false;
// static uint32_t sample_count = 0; // 当前采集点数

// //环形缓冲区配置
// static ringbuffer_t s1_buffer;
// static ringbuffer_t s2_buffer;
// static size_t s1_w_idx = 0;
// static size_t s1_r_idx = 0;
// static size_t s2_w_idx = 0;
// static size_t s2_r_idx = 0;

// // 使用静态数组保存历史数据，避免每次调用都清零
// static struct compx s1[FFT_N]; 
// static struct compx s2[FFT_N];



// /***********************************************************
//  * 环形缓冲区初始化
//  * 
//  * @param NULL
//  * @return
//  *      - ESP_OK
//  * 
//  **********************************************************/
// static esp_err_t ringbuffer_init(void)
// {
//     //s1初始化配置
//     s1_buffer.buffer = (uint8_t*)s1;
//     s1_buffer.buffer_size = FFT_N * sizeof(struct compx);
//     s1_buffer.element_size = sizeof(struct compx);
//     s1_buffer.count = 0;
//     s1_buffer.read_index = &s1_w_idx;
//     s1_buffer.write_index = &s1_r_idx;
//     *(s1_buffer.read_index) = 0;
//     *(s1_buffer.write_index) = 0;
    
//     //s2初始化配置
//     s2_buffer.buffer = (uint8_t*)s2;
//     s2_buffer.buffer_size = FFT_N * sizeof(struct compx);
//     s2_buffer.element_size = sizeof(struct compx);
//     s2_buffer.count = 0;
//     s2_buffer.read_index = &s2_w_idx;
//     s2_buffer.write_index = &s2_w_idx;
//     *(s2_buffer.read_index) = 0;
//     *(s2_buffer.write_index) = 0;

//     return ESP_OK;
// }

// /***********************************************************
//  * @brief 向环形缓冲区写入数据
//  * 
//  * @param NULL
//  * 
//  * 
//  **********************************************************/
// static esp_err_t ringbuffer_write(const float data)
// {

//     return ESP_OK;
// }

// /***********************************************************
//  * @brief 读取环形缓冲读取数据
//  * 
//  * 
//  * 
//  * 
//  **********************************************************/
// static esp_err_t ringbuffer_read(void)
// {

//     return ESP_OK;
// }


// /***********************************************************
//  * @brief 初始化服务
//  * 
//  * @param NULL
//  * @return 
//  *      - ESP_OK
//  *      - ESP_FAIL
//  * 
//  **********************************************************/
// esp_err_t sensor_service_init(void)
// {
//     esp_err_t ret = ESP_OK;
//     if (is_initialized) {
//         return ret;
//     }

//     ESP_LOGI(TAG, "Initializing Sensor Service...");
    
//     // 1. 初始化底层驱动
//     ret = driver_max30102_init();
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Driver init failed: %s", esp_err_to_name(ret));
//         return ret;
//     }

//     // 2. 清空 FFT 缓冲区
//     memset(s1, 0, sizeof(s1));
//     memset(s2, 0, sizeof(s2));
//     sample_count = 0;

//     is_initialized = true;
//     ESP_LOGI(TAG, "Sensor Service Ready");
//     return ret;
// }

// /***********************************************************
//  * @brief 获取最终数据
//  * 
//  * @param[out] out_data 最终数据
//  * @return 
//  *      - ESP_OK
//  *      - ESP_FAIL
//  *      - ESP_ERR_INVALID_STATE
//  *      - ESP_ERR_INVALID_ARG
//  * 
//  **********************************************************/
// esp_err_t sensor_service_get_data(sensor_data_t *out_data)
// {
//     if (!is_initialized) {
//         return ESP_ERR_INVALID_STATE;
//     }
//     if (out_data == NULL) {
//         return ESP_ERR_INVALID_ARG;
//     }

//     // 初始化输出数据结构
//     out_data->is_valid = false;
//     out_data->heart_rate_bpm = 0;
//     out_data->spo2_percent = 0.0f;

//     // 1. 读取单个原始数据点
//     max30102_raw_data_t raw;
//     esp_err_t ret = driver_max30102_read_raw(&raw);
//     if (ret != ESP_OK) {
//         return ESP_ERR_NOT_FOUND;
//     }

//     //查看原始数据
//     ESP_LOGI(TAG, "Raw: R=%lu, IR=%lu", raw.red, raw.ir);

//     // 2. 简单的有效性检查 (防止饱和或无手指)
//     if (raw.ir < 10000 || raw.ir > 0x3FFFF) {
//         // 信号太弱或饱和，不存入缓冲区，直接返回
//         // 不清空缓冲区，保留之前的数据等待新信号
//         return ret; 
//     }

//     //FFT数据存储转换
//     if (sample_count < FFT_N) {
//         // 填充阶段
//         s1[sample_count].real = (float)raw.red;
//         s1[sample_count].imag = 0;
//         s2[sample_count].real = (float)raw.ir;
//         s2[sample_count].imag = 0;
//         sample_count++;
//     } else {
//         // 缓冲区已满，移位：丢弃第 0 个，后面的前移，新的放最后
//         // 优化：实际项目中建议使用 ring buffer 索引，避免 memcpy 开销
//         memmove(&s1[0], &s1[1], sizeof(struct compx) * (FFT_N - 1));
//         memmove(&s2[0], &s2[1], sizeof(struct compx) * (FFT_N - 1));
        
//         s1[FFT_N - 1].real = (float)raw.red;
//         s1[FFT_N - 1].imag = 0;
//         s2[FFT_N - 1].real = (float)raw.ir;
//         s2[FFT_N - 1].imag = 0;
//     }

//     // 如果还没存满，直接返回，等待下一次调用
//     if (sample_count < FFT_N) {
//         return ret;
//     }

//     // --- 缓冲区已满，开始计算 ---

//     // 4. 直流分量去除 (DC Removal)
//     float sum_red = 0, sum_ir = 0;
//     for (int i = 0; i < FFT_N; i++) {
//         sum_red += s1[i].real;
//         sum_ir += s2[i].real;
//     }
//     float dc_red = sum_red / FFT_N;
//     float dc_ir = sum_ir / FFT_N;

//     for (int i = 0; i < FFT_N; i++) {
//         s1[i].real -= dc_red;
//         s2[i].real -= dc_ir;
//         // imag 保持为 0
//     }

//     // 6. 执行 FFT (确保 algorithm.h 中的 FFT 函数是原地运算)
//     FFT(s1);
//     FFT(s2);

//     // 7. 计算模值 (Magnitude)
//     float ac_red_sum = 0;
//     float ac_ir_sum = 0;
    
//     // 通常心率在 0.5Hz - 4Hz 之间，对应 FFT 的低频段
//     // 我们只计算有效频段的能量，忽略直流 (i=0) 和高频噪声
//     int valid_range_end = FFT_N / 2; // 奈奎斯特频率
    
//     for (int i = 1; i < valid_range_end; i++) {
//         float mag_red = sqrtf(s1[i].real * s1[i].real + s1[i].imag * s1[i].imag);
//         float mag_ir = sqrtf(s2[i].real * s2[i].real + s2[i].imag * s2[i].imag);
        
//         // 更新数组为模值，方便后续找峰值
//         s1[i].real = mag_red;
//         s1[i].imag = 0;
//         s2[i].real = mag_ir;
//         s2[i].imag = 0;

//         ac_red_sum += mag_red;
//         ac_ir_sum += mag_ir;
//     }

//     // 8. 寻找峰值频率 (Heart Rate Frequency)
//     // 限制搜索范围：假设心率 40bpm - 200bpm
//     // 40bpm @ 50Hz sample, N=256 -> index approx: (40/60)*256/50 = 3.4
//     // 200bpm -> index approx: 17
//     // 为了安全，搜索范围设为 2 到 60 (覆盖极高心率或谐波)
//     int max_idx_red = find_max_num_index(s1, 2, 60); 
//     int max_idx_ir = find_max_num_index(s2, 2, 60);

//     // 9. 验证一致性
//     // 如果红光和红外光检测到的主频率不一致，说明信号不可靠
//     int diff = abs(max_idx_red - max_idx_ir);
    
//     // 检查峰值是否明显 (避免在噪声中找最大值)
//     // 简单判断：峰值点的值是否大于平均值的 2 倍？(此处简化处理)
//     if (diff <= 2 && max_idx_red > 2) { 
//         // 频率一致，计算心率
//         // HR = (Index * SampleRate * 60) / FFT_N
//         float hr_float = (float)max_idx_red * SAMPLES_PER_SECOND * 60.0f / (float)FFT_N;
        
//         // 过滤异常心率
//         if (hr_float >= 40.0 && hr_float <= 200.0) {
//             out_data->heart_rate_bpm = (uint16_t)(hr_float + 0.5f);
            
//             // 计算血氧 (R 值)
//             // R = (AC_IR / DC_IR) / (AC_Red / DC_Red)
//             // 注意：这里的 AC 应该是峰值处的幅度，或者是总交流能量。
//             // 经典算法常用峰值幅度比。
//             float ac_red_peak = s1[max_idx_red].real;
//             float ac_ir_peak = s2[max_idx_ir].real;
            
//             if (ac_red_peak > 0 && dc_red > 0 && dc_ir > 0) {
//                 float R = (ac_ir_peak / dc_ir) / (ac_red_peak / dc_red);
                
//                 // 经验公式 (不同传感器系数不同，需校准)
//                 float spo2 = -45.060 * R * R + 30.354 * R + 94.845;
                
//                 if (spo2 >= 0 && spo2 <= 100) {
//                     out_data->spo2_percent = (uint16_t)(spo2 + 0.5f);
//                     out_data->is_valid = true;
                    
//                     ESP_LOGI(TAG, "HR: %d, SpO2: %d%% (Idx:%d, R:%.2f)", 
//                              out_data->heart_rate_bpm, out_data->spo2_percent, max_idx_red, R);
//                 }
//             }
//         }
//     } else {
//         ESP_LOGW(TAG, "Signal Mismatch or Weak (RedIdx:%d, IrIdx:%d, Diff:%d)", 
//                  max_idx_red, max_idx_ir, diff);
//     }

//     return ESP_OK;
// }


#include "sensor_service.h"
#include "max30102_driver.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "algorithm.h"  // 算法库

static const char *TAG = "SENSOR_SVC";

// 标定参数
#define CALIBRATION_OFFSET 47 

// 全局状态
static bool is_initialized = false;

// --- 环形缓冲区配置 ---
static ringbuffer_t s1_buffer;
static ringbuffer_t s2_buffer;

// 【关键】独立的索引变量存储区
// read_index 必须指向 r_idx, write_index 必须指向 w_idx
static size_t s1_w_idx = 0;
static size_t s1_r_idx = 0;
static size_t s2_w_idx = 0;
static size_t s2_r_idx = 0;

// 底层数据存储数组 (Ring Buffer 的数据区)
static struct compx s1_data[FFT_N]; 
static struct compx s2_data[FFT_N];

// 用于 FFT 计算的临时连续缓冲区 (解决环布跨边界问题)
static struct compx s1_fft_temp[FFT_N];
static struct compx s2_fft_temp[FFT_N];

/***********************************************************
 * @brief 向环形缓冲区写入数据 (滑动窗口模式)
 * 
 * 如果缓冲区已满，自动覆盖最旧的数据，并移动读指针。
 **********************************************************/
static esp_err_t ringbuffer_push(ringbuffer_t *rb, const struct compx *data)
{
    if (!rb || !data) return ESP_ERR_INVALID_ARG;

    size_t capacity = rb->buffer_size / rb->element_size;
    size_t *w_idx = rb->write_index;
    size_t *r_idx = rb->read_index;

    // 1. 计算当前写入位置的字节偏移
    size_t offset = (*w_idx) * rb->element_size;
    
    // 2. 复制数据 (注意类型转换)
    memcpy(rb->buffer + offset, data, rb->element_size);

    // 3. 更新写指针 (循环递增)
    // 假设 capacity 是 2 的幂，使用位运算优化
    *w_idx = (*w_idx + 1) & (capacity - 1);

    // 4. 更新计数和读指针
    if (rb->count < capacity) {
        rb->count++;
    } else {
        // 缓冲区已满，发生覆盖：读指针也要向前移一位，丢弃最旧数据
        *r_idx = (*r_idx + 1) & (capacity - 1);
        // count 保持为 capacity
    }

    return ESP_OK;
}

/***********************************************************
 * @brief 准备 FFT 数据 (处理回绕)
 * 
 * 将环形缓冲区中的数据整理到连续的 temp_buf 中。
 * 如果数据未跨边界，直接返回原始指针 (强转后)。
 * 如果跨边界，拷贝到 temp_buf 并返回 temp_buf 指针。
 **********************************************************/
static const struct compx* prepare_fft_data(ringbuffer_t *rb, struct compx *temp_buf)
{
    if (!rb || rb->count < (rb->buffer_size / rb->element_size)) {
        return NULL; // 数据未满，不能计算
    }

    size_t capacity = rb->buffer_size / rb->element_size;
    size_t r_idx = *(rb->read_index);

    // 在满状态下，数据从 r_idx 开始，长度为 capacity
    // 如果 r_idx == 0，数据是连续的 [0 ... N-1]
    if (r_idx == 0) {
        return (const struct compx*)rb->buffer;
    }

    // 如果 r_idx != 0，数据跨边界：[r_idx ... N-1] + [0 ... r_idx-1]
    // 需要拼接拷贝
    size_t part1_len = capacity - r_idx; // 后半段长度
    
    // 拷贝后半段到 temp_buf 开头
    memcpy(temp_buf, 
           rb->buffer + (r_idx * rb->element_size), 
           part1_len * rb->element_size);
    
    // 拷贝前半段到 temp_buf 后面
    memcpy(temp_buf + part1_len, 
           rb->buffer, 
           r_idx * rb->element_size);

    return temp_buf;
}

/***********************************************************
 * 环形缓冲区初始化
 **********************************************************/
static esp_err_t ringbuffer_init(void)
{
    // --- S1 初始化 ---
    s1_buffer.buffer = (uint8_t*)s1_data;
    s1_buffer.buffer_size = FFT_N * sizeof(struct compx);
    s1_buffer.element_size = sizeof(struct compx);
    s1_buffer.count = 0;
    
    // 【修复】正确绑定指针：read 指向 r_idx, write 指向 w_idx
    s1_buffer.read_index = &s1_r_idx;
    s1_buffer.write_index = &s1_w_idx;
    
    *(s1_buffer.read_index) = 0;
    *(s1_buffer.write_index) = 0;

    // --- S2 初始化 ---
    s2_buffer.buffer = (uint8_t*)s2_data;
    s2_buffer.buffer_size = FFT_N * sizeof(struct compx);
    s2_buffer.element_size = sizeof(struct compx);
    s2_buffer.count = 0;
    
    // 【修复】正确绑定指针
    s2_buffer.read_index = &s2_r_idx;
    s2_buffer.write_index = &s2_w_idx;
    
    *(s2_buffer.read_index) = 0;
    *(s2_buffer.write_index) = 0;

    ESP_LOGI(TAG, "RingBuffer Init Done. Capacity: %d", FFT_N);
    return ESP_OK;
}

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
    ret = ringbuffer_init();
    if (ret != ESP_OK) return ret;

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
    
    ESP_LOGI(TAG, "Raw: R=%lu, IR=%lu", raw.red, raw.ir);

    // 2. 有效性检查
    if (raw.ir < 10000 || raw.ir > 0x3FFFF) {
        // 信号无效，跳过本次采集，不写入缓冲区
        return ESP_OK; 
    }

    // 3. 构造数据点
    struct compx new_s1 = {.real = (float)raw.red, .imag = 0};
    struct compx new_s2 = {.real = (float)raw.ir, .imag = 0};

    // 4. 写入环形缓冲区 (O(1) 操作，无 memmove!)
    ringbuffer_push(&s1_buffer, &new_s1);
    ringbuffer_push(&s2_buffer, &new_s2);

    // 5. 检查是否已满
    if (s1_buffer.count < FFT_N) {
        // 还没攒够一次 FFT 的数据，直接返回
        return ESP_OK;
    }

    // 6. 准备连续数据用于 FFT
    const struct compx *fft_src_s1 = prepare_fft_data(&s1_buffer, s1_fft_temp);
    const struct compx *fft_src_s2 = prepare_fft_data(&s2_buffer, s2_fft_temp);

    if (!fft_src_s1 || !fft_src_s2) {
        return ESP_OK; 
    }

    // 【关键修复 1】强制拷贝并清零工作区，防止垃圾数据污染
    // 即使 prepare_fft_data 返回了有效指针，我们也重新拷贝到已知干净的静态数组
    static struct compx s1_work[FFT_N];
    static struct compx s2_work[FFT_N];
    
    memcpy(s1_work, fft_src_s1, sizeof(struct compx) * FFT_N);
    memcpy(s2_work, fft_src_s2, sizeof(struct compx) * FFT_N);

    // 【关键修复 2】检查输入数据是否包含 NaN 或 Inf (防御性编程)
    bool input_valid = true;
    for(int i=0; i<FFT_N; i++) {
        if (!isfinite(s1_work[i].real) || !isfinite(s2_work[i].real)) {
            input_valid = false;
            break;
        }
    }
    
    if (!input_valid) {
        ESP_LOGE(TAG, "⚠️ CRITICAL: Input data contains NaN/Inf! Resetting buffers...");
        // 简单重置索引，丢弃当前脏数据
        *(s1_buffer.read_index) = 0; *(s1_buffer.write_index) = 0; s1_buffer.count = 0;
        *(s2_buffer.read_index) = 0; *(s2_buffer.write_index) = 0; s2_buffer.count = 0;
        memset(s1_work, 0, sizeof(s1_work));
        memset(s2_work, 0, sizeof(s2_work));
        return ESP_OK;
    }

    // 7. 直流分量去除 (DC Removal)
    float sum_red = 0.0f, sum_ir = 0.0f;
    for (int i = 0; i < FFT_N; i++) {
        sum_red += s1_work[i].real;
        sum_ir += s2_work[i].real;
    }
    
    // 防止除以零
    if (FFT_N == 0) return ESP_OK; 
    
    float dc_red = sum_red / (float)FFT_N;
    float dc_ir = sum_ir / (float)FFT_N;

    // 再次检查 DC 计算结果
    if (!isfinite(dc_red) || !isfinite(dc_ir)) {
        ESP_LOGE(TAG, "⚠️ CRITICAL: DC calculation resulted in NaN!");
        return ESP_OK;
    }

    for (int i = 0; i < FFT_N; i++) {
        s1_work[i].real -= dc_red;
        s2_work[i].real -= dc_ir;
        s1_work[i].imag = 0.0f; // 确保虚部清零
        s2_work[i].imag = 0.0f;
        
        // 再次检查去直流后的数据
        if (!isfinite(s1_work[i].real) || !isfinite(s2_work[i].real)) {
             ESP_LOGE(TAG, "⚠️ CRITICAL: DC removal resulted in NaN at index %d", i);
             return ESP_OK;
        }
    }

    // 8. 执行 FFT
    // 假设 FFT 函数是 void FFT(struct compx *data)
    FFT(s1_work);
    FFT(s2_work);

    // 【关键修复 3】检查 FFT 输出是否产生 NaN
    bool fft_valid = true;
    for(int i=0; i<FFT_N/2; i++) { // 只检查前半部分
        if (!isfinite(s1_work[i].real) || !isfinite(s1_work[i].imag) ||
            !isfinite(s2_work[i].real) || !isfinite(s2_work[i].imag)) {
            fft_valid = false;
            break;
        }
    }

    if (!fft_valid) {
        ESP_LOGE(TAG, "⚠️ CRITICAL: FFT output contains NaN! Check FFT implementation or input range.");
        // 这里可以选择重置缓冲区
        return ESP_OK;
    }

    // 9. 计算模值
    int valid_range_end = FFT_N / 2;
    for (int i = 1; i < valid_range_end; i++) {
        float r_real = s1_work[i].real;
        float r_imag = s1_work[i].imag;
        float i_real = s2_work[i].real;
        float i_imag = s2_work[i].imag;

        // 防止 sqrtf(-nan) 或 sqrtf(极大数溢出)
        float mag_red = sqrtf(fmaxf(0.0f, r_real * r_real + r_imag * r_imag));
        float mag_ir = sqrtf(fmaxf(0.0f, i_real * i_real + i_imag * i_imag));
        
        s1_work[i].real = mag_red;
        s1_work[i].imag = 0.0f;
        s2_work[i].real = mag_ir;
        s2_work[i].imag = 0.0f;
    }

    // 10. 寻找峰值 (提高起始索引)
    int min_idx = 4; 
    int max_idx_red = find_max_num_index(s1_work, min_idx, 60); 
    int max_idx_ir = find_max_num_index(s2_work, min_idx, 60);

    float ac_red_peak = s1_work[max_idx_red].real;
    float ac_ir_peak = s2_work[max_idx_ir].real;

    // 打印调试 (现在应该是正常数字了)
    ESP_LOGI(TAG, "DEBUG: DC_Red=%.1f, DC_Ir=%.1f", dc_red, dc_ir);
    ESP_LOGI(TAG, "DEBUG: AC_Red_Peak=%.2f, AC_Ir_Peak=%.2f (at idx %d)", 
             ac_red_peak, ac_ir_peak, max_idx_red);

    // 阈值判断 (根据实际打印调整，比如先设为 10.0)
    float peak_threshold = 10.0f; 
    int diff = abs(max_idx_red - max_idx_ir);

    if (diff <= 2 && max_idx_red >= min_idx && 
        isfinite(ac_red_peak) && isfinite(ac_ir_peak) &&
        ac_red_peak > peak_threshold && ac_ir_peak > peak_threshold) { 
        
        // ... (后续计算心率血氧逻辑不变) ...
        float hr_float = (float)max_idx_red * SAMPLES_PER_SECOND * 60.0f / (float)FFT_N;
        
        if (hr_float >= 40.0 && hr_float <= 200.0) {
            out_data->heart_rate_bpm = (uint16_t)(hr_float + 0.5f);
            
            if (ac_red_peak > 0 && dc_red > 0 && dc_ir > 0) {
                float R = (ac_ir_peak / dc_ir) / (ac_red_peak / dc_red);
                
                if (isfinite(R)) {
                    float spo2 = -45.060 * R * R + 30.354 * R + 94.845;
                    
                    if (spo2 >= 0 && spo2 <= 100) {
                        out_data->spo2_percent = (uint16_t)(spo2 + 0.5f);
                        out_data->is_valid = true;
                        ESP_LOGI(TAG, "✅ SUCCESS: HR: %d, SpO2: %d%%", out_data->heart_rate_bpm, out_data->spo2_percent);
                        return ESP_OK;
                    }
                }
            }
        }
    }
    
    ESP_LOGW(TAG, "Signal Mismatch or Weak");
    return ESP_OK;
}