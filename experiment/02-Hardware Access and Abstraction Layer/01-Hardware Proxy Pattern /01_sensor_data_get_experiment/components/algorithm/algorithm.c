#include "algorithm.h"
#include <math.h> // 使用标准库，更安全

// 如果 ESP32 环境没有标准的 sin/cos 或为了极致优化保留查表，请使用下面的修正版
// 这里我们采用标准 math.h 以确保精度，除非你有严格的性能限制

struct compx EE(struct compx a, struct compx b)
{
    struct compx c;
    // 确保所有中间计算使用 float，避免 double 转 float 的截断误差累积
    c.real = a.real * b.real - a.imag * b.imag;
    c.imag = a.real * b.imag + a.imag * b.real;
    return c;
}

void FFT(struct compx *xin)
{
    int f, m, nv2, nm1, i, k, l, j = 0;
    struct compx u, w, t;

    // 安全检查：确保传入的数组大小匹配宏定义
    // 如果 FFT_N 不是 2 的幂，此算法会失败
    if ((FFT_N & (FFT_N - 1)) != 0) {
        return; 
    }

    nv2 = FFT_N / 2;
    nm1 = FFT_N - 1;

    // 1. 位反转排序 (Bit-Reversal Permutation)
    for (i = 0; i < nm1; i++) {
        if (i < j) {
            t = xin[j];
            xin[j] = xin[i];
            xin[i] = t;
        }
        k = nv2;
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }

    // 2. 蝶形运算 (Butterfly Operation)
    // 计算级数 l = log2(FFT_N)
    for (l = 0, f = FFT_N; f > 1; l++) {
        f >>= 1;
    }

    for (m = 1; m <= l; m++) {
        int le = 1 << m;       // 当前级蝴蝶跨度
        int lei = le >> 1;     // 半跨度
        
        // 计算旋转因子 W_N^k = cos(2*pi*k/le) - j*sin(2*pi*k/le)
        // 注意：原代码用的是 PI/lei，即 2*PI/le，逻辑正确
        float angle = 3.14159265358979323846f / (float)lei;
        
        u.real = 1.0f;
        u.imag = 0.0f;
        
        // 使用标准 math.h 的 cos/sin，精度更有保障
        // 如果必须用查表，请确保查表函数返回 float 且无 NaN
        w.real = cosf(angle); 
        w.imag = -sinf(angle);

        for (j = 0; j < lei; j++) {
            for (i = j; i < FFT_N; i += le) {
                int ip = i + lei;
                
                // 复数乘法 t = xin[ip] * u
                t.real = xin[ip].real * u.real - xin[ip].imag * u.imag;
                t.imag = xin[ip].real * u.imag + xin[ip].imag * u.real;

                // 蝶形运算
                xin[ip].real = xin[i].real - t.real;
                xin[ip].imag = xin[i].imag - t.imag;
                xin[i].real = xin[i].real + t.real;
                xin[i].imag = xin[i].imag + t.imag;
            }
            
            // 更新旋转因子 u = u * w
            float temp_real = u.real * w.real - u.imag * w.imag;
            u.imag = u.real * w.imag + u.imag * w.real;
            u.real = temp_real;
            
            // 防止误差累积导致模长偏离 1 太远 (可选优化)
            // float mag = sqrtf(u.real*u.real + u.imag*u.imag);
            // if(mag > 0) { u.real /= mag; u.imag /= mag; }
        }
    }
}

int find_max_num_index(struct compx *data, int start_idx, int end_idx)
{
    if (start_idx < 0) start_idx = 0;
    if (end_idx >= FFT_N) end_idx = FFT_N - 1;
    if (start_idx > end_idx) return start_idx;

    int max_idx = start_idx;
    float max_val = data[start_idx].real;

    for (int i = start_idx + 1; i <= end_idx; i++) {
        // 增加 NaN 检查，防止污染最大值
        if (!isfinite(data[i].real)) continue;
        
        if (data[i].real > max_val) {
            max_val = data[i].real;
            max_idx = i;
        }
    }
    return max_idx;
}

// 滤波器实现保持逻辑不变，但增加类型安全
int dc_filter(int input, DC_FilterData *df)
{
    if (!df->init) {
        df->w = (float)input;
        df->init = 1;
        return 0;
    }
    
    // 系数 a 需要在初始化时设置，例如 0.95
    float new_w = (float)input + df->w * df->a;
    float result_f = new_w - df->w;
    
    // 限制输出范围，防止溢出
    if (result_f > 32767.0f) result_f = 32767.0f;
    if (result_f < -32768.0f) result_f = -32768.0f;
    
    df->w = new_w;
    return (int16_t)result_f;
}

int bw_filter(int input, BW_FilterData *bw)
{
    bw->v0 = bw->v1;
    bw->v1 = (1.241106190967544882e-2f * (float)input) + 
             (0.97517787618064910582f * bw->v0);
    
    float res = bw->v0 + bw->v1;
    if (res > 32767.0f) res = 32767.0f;
    if (res < -32768.0f) res = -32768.0f;
    
    return (int)res;
}