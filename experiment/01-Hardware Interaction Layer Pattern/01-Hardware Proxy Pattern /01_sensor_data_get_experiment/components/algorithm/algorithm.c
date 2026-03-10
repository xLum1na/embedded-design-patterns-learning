#include "algorithm.h"

#define XPI         3.1415926535897932384626433832795
#define XENTRY      100
#define XINCL       (XPI/2/XENTRY)
#define PI          3.1415926535897932384626433832795028841971

// 正弦查找表（保持不变）
static const double XSinTbl[] = {
	0.00000000000000000,  0.015707317311820675, 
	0.031410759078128292, 0.047106450709642665, 
	// ... 省略中间值，与原文件相同 ...
	0.99950656036573160,  0.99987663248166059, 
	1.00000000000000000  
};

double my_floor(double x)
{
	double y = x;
	if (*(((int *)&y) + 1) & 0x80000000)
		return (float)((int)x) - 1;
	else
		return (float)((int)x);
}

double my_fmod(double x, double y)
{
	if (y == 0.0) return 0.0;
	double temp = my_floor(x / y);
	double ret = x - temp * y;
	if ((x < 0.0) != (y < 0.0))
		ret = ret - y;
	return ret;
}

double XSin(double x)
{
	int s = 0, n;
	double dx, sx, cx;
	
	if (x < 0) s = 1, x = -x;
	x = my_fmod(x, 2 * XPI);
	if (x > XPI) s = !s, x -= XPI;
	if (x > XPI / 2) x = XPI - x;
	
	n = (int)(x / XINCL);
	dx = x - n * XINCL;
	if (dx > XINCL / 2) ++n, dx -= XINCL;
	
	sx = XSinTbl[n];
	cx = XSinTbl[XENTRY - n];
	x = sx + dx * cx - (dx * dx) * sx / 2
		- (dx * dx * dx) * cx / 6 
		+ (dx * dx * dx * dx) * sx / 24;

	return s ? -x : x;
}

double XCos(double x)
{
	return XSin(x + XPI / 2);
}

// 快速整数开方（牛顿迭代法优化）
int qsqrt(int a)
{
	if (a <= 0) return 0;
	
	uint32_t rem = 0, root = 0;
	uint16_t i;
	
	for (i = 0; i < 16; i++) {
		root <<= 1;
		rem = ((rem << 2) + (a >> 30));
		a <<= 2;
		uint32_t divisor = (root << 1) + 1;
		if (divisor <= rem) {
			rem -= divisor;
			root++;
		}
	}
	return (int)root;
}

struct compx EE(struct compx a, struct compx b)
{
	struct compx c;
	c.real = a.real * b.real - a.imag * b.imag;
	c.imag = a.real * b.imag + a.imag * b.real;
	return c;
}

void FFT(struct compx *xin)
{
	int f, m, nv2, nm1, i, k, l, j = 0;
	struct compx u, w, t;

	nv2 = FFT_N / 2;
	nm1 = FFT_N - 1;
	
	// 位反转排序
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

	// 蝶形运算
	f = FFT_N;
	for (l = 1; (f = f / 2) != 1; l++);
	
	for (m = 1; m <= l; m++) {
		int le = 2 << (m - 1);
		int lei = le / 2;
		u.real = 1.0;
		u.imag = 0.0;
		w.real = XCos(PI / lei);
		w.imag = -XSin(PI / lei);
		
		for (j = 0; j <= lei - 1; j++) {
			for (i = j; i <= FFT_N - 1; i += le) {
				int ip = i + lei;
				t = EE(xin[ip], u);
				xin[ip].real = xin[i].real - t.real;
				xin[ip].imag = xin[i].imag - t.imag;
				xin[i].real = xin[i].real + t.real;
				xin[i].imag = xin[i].imag + t.imag;
			}
			u = EE(u, w);
		}
	}
}

// 优化：带范围限制的峰值查找
int find_max_num_index(struct compx *data, int start_idx, int end_idx)
{
	if (start_idx < 0) start_idx = 0;
	if (end_idx >= FFT_N) end_idx = FFT_N - 1;
	if (start_idx > end_idx) return start_idx;
	
	int max_idx = start_idx;
	float max_val = data[start_idx].real;
	
	for (int i = start_idx + 1; i <= end_idx; i++) {
		if (data[i].real > max_val) {
			max_val = data[i].real;
			max_idx = i;
		}
	}
	return max_idx;
}

// IIR 高通滤波器（去除直流）
int dc_filter(int input, DC_FilterData *df)
{
	if (!df->init) {
		df->w = input;
		df->init = 1;
		return 0;
	}
	
	float new_w = input + df->w * df->a;
	int16_t result = (int16_t)(new_w - df->w);
	df->w = new_w;
	
	return result;
}

// 带通滤波器
int bw_filter(int input, BW_FilterData *bw)
{
	bw->v0 = bw->v1;
	bw->v1 = (1.241106190967544882e-2 * input) + 
			 (0.97517787618064910582 * bw->v0);
	return (int)(bw->v0 + bw->v1);
}