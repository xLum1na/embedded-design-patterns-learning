#pragma once

#include <stdint.h>
#include <stdbool.h>

#define FFT_N 			128    	// 傅里叶变换点数
#define START_INDEX 	2  		// 心率40bpm对应的最低索引 (40/60*256/50≈3.4)
#define END_INDEX 		8 		// 心率200bpm对应的最高索引 (200/60*256/50≈17)

// 采样率定义
#define SAMPLES_PER_SECOND 100

struct compx {
	float real;
	float imag;
};  

typedef struct {
	float w;
	int init;
	float a;
} DC_FilterData;

typedef struct {
	float v0;
	float v1;
} BW_FilterData;

// 基础数学函数
double my_floor(double x);
double my_fmod(double x, double y);
double XSin(double x);
double XCos(double x);
int qsqrt(int a);

// 复数与FFT
struct compx EE(struct compx a, struct compx b);
void FFT(struct compx *xin);

// 查找峰值（带范围限制）
int find_max_num_index(struct compx *data, int start_idx, int end_idx);

// 滤波器
int dc_filter(int input, DC_FilterData *df);
int bw_filter(int input, BW_FilterData *bw);
