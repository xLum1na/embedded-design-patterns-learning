/**************************************************
 * @file    bsp_ili9488.h
 * @brief   板级ili9488驱动声明
 * @author  xLumina
 * @version V1.0.0
 * @date    2026-4-9
 *
 * @par History:
 *   - V1.0.0 | 2026-4-9 | xLumina | 初始版本创建
 *
 * @par Function List:
 * 
 * 
 **************************************************/

#pragma once

#include <stdio.h>   
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif


/*************** declaration ***************/
typedef struct bsp_ili9488_s *bsp_ili9488_handle_t;

/**
 * @brief   spi驱动接口
 * @details 由上层注入spi依赖，用于内部驱动通信 
 */
typedef struct spi_driver_t {
    bool (*spi_write_func)(uint8_t *data, uint8_t len, int timeout_ms); /* SPI 写数据接口 */
    bool (*spi_read_func)(uint8_t *data, uint8_t len, int timeout_ms);  /* SPI 读数据接口 */
}spi_driver_t;

/**
 * @brief   ledc驱动接口
 * @details 由上层注入ledc依赖，用于内部背光亮度驱动 
 */
typedef struct ledc_driver_t {
    bool (*ledc_set_duty_func)(uint8_t duty);   /* 设置背光占空比接口 */
}ledc_driver_t;

/**
 * @brief   gpio驱动接口
 * @details 由上层注入gpio依赖，用于内部控制数据/命令传输驱动 
 */
typedef struct gpio_driver_t {
    bool (*gpio_set_dc_func)(uint32_t level);       /* 设置命令/数据接口 */
    bool (*gpio_set_reset_func)(uint32_t level);    /* 设置复位接口 */
}gpio_driver_t;

/**
 * @brief   ILI9488 驱动错误码定义
 */
typedef enum ili9488_err_t {
    ILI9488_OK              = 0,    /* 成功 */
    ILI9488_FAIL            = 1,    /* 通用失败 (未知错误) */
    ILI9488_ERR_INVALID_ARG = 2,    /* 参数错误 */
    ILI9488_ERR_NO_MEM      = 3,    /* 内存分配失败 */
    ILI9488_ERR_TIMEOUT     = 4,    /* 操作超时 */
    ILI9488_ERR_NOT_INIT    = 5,    /* 驱动未初始化 */
    ILI9488_ERR_SPI_FAIL    = 6,    /* SPI 错误 */
    ILI9488_ERR_LEDC_FAIL   = 7,    /* LEDC 错误*/
    ILI9488_ERR_GPIO_FAIL   = 8,    /* GPIO 错误*/ 
    ILI9488_ERR_READ_ID     = 9,    /* 读取屏幕 ID 不匹配 */
    ILI9488_ERR_BUSY        = 10,   /* 设备忙 */
    ILI9488_ERR_SLEEP       = 11,   /* 设备处于休眠模式 */
} ili9488_err_t;
/**
 * @brief   ILI9488 显示颜色深度
 * @details 用于设置屏幕显示颜色深度
 */
typedef enum ili9488_color_style_t {
    ILI9488_COLOR_RGB565 = 0,   /* 16位色，R:5 G:6 B:5 */
    ILI9488_COLOR_RGB888 = 1,   /* 24位色，R:8 G:8 B:8 */
} ili9488_color_style_t;

/**
 * @brief   ILI9488 显示方向
 * @details 用于配置屏幕显示方向
 */
typedef enum ili9488_direction_t {
    ILI9488_DIR_0       = 0,    /* 0度 */
    ILI9488_DIR_90      = 1,    /* 90度 */
    ILI9488_DIR_180     = 2,    /* 180度 */
    ILI9488_DIR_270     = 3,    /* 270度 */
} ili9488_direction_t;

/**
 * @brief   ILI9488 扫描方向
 * @details 设置GRAM的写入方向，影响显示镜像
 */
typedef enum ili9488_scan_direction_t {
    ILI9488_SCAN_LTR_TTB = 0,   /* 左→右，上→下（正常） */
    ILI9488_SCAN_RTL_TTB = 1,   /* 右→左，上→下（水平镜像） */
    ILI9488_SCAN_LTR_BTT = 2,   /* 左→右，下→上（垂直镜像） */
    ILI9488_SCAN_RTL_BTT = 3,   /* 右→左，下→上（水平+垂直镜像） */
} ili9488_scan_direction_t;

/**
 * @brief   ILI9488 刷新率
 */
typedef enum ili9488_fps_t {
    ILI9488_FPS_30 = 30,        /* 30帧 */
    ILI9488_FPS_60 = 60,        /* 60帧 */
    ILI9488_FPS_90 = 90,        /* 90帧 */
} ili9488_fps_t;

/**
 * @brief   ILI9488 像素格式接口
 */
typedef enum ili9488_interface_t {
    ILI9488_IF_8BIT     = 8,        /* 8位并口 */
    ILI9488_IF_16BIT    = 16,      /* 16位并口 */
    ILI9488_IF_SPI      = 0,         /* SPI接口 */
} ili9488_interface_t;

/**
 * @brief   ILI9488 屏幕数据结构
 */
typedef struct bsp_ili9488_config_t {
    uint16_t                    width;      /* 屏幕宽度 */
    uint16_t                    height;     /* 屏幕高度 */
    ili9488_color_style_t       color;      /* 色彩格式 */
    ili9488_direction_t         dir;        /* 显示方向 */
    ili9488_scan_direction_t    scan_dir;   /* 扫描方向 */
    ili9488_fps_t               fps;        /* 刷新率 */
    ili9488_interface_t         interface;  /* 接口类型 */
    void                        *user_data; /* 用户私有数据 */
} bsp_ili9488_config_t;


/**
 * @brief   ILI9488 设备构造函数
 *
 * @param[in] ili9488_cfg ILI9488 配置
 * @param[in] spi_driver_if   注入 ILI9488 驱动所需的 SPI 接口实例
 * @param[in] ledc_driver_if  注入 ILI9488 驱动所需的 LEDC 接口实例
 * @param[in] gpio_driver_if  注入 ILI9488 驱动所需的 GPIO 接口实例
 * @param[out] ili9488_intance ILI9488 实例句柄
 *
 * @return 
 *      - ILI9488_OK
 *      - ILI9488_ERR_NO_MEM
 *      - ILI9488_ERR_INVALID_ARG
 *
 * @note 由上层调用创建设备实例
 */
ili9488_err_t ili9488_create(const bsp_ili9488_config_t *ili9488_cfg,
                                const spi_driver_t *spi_driver_if,
                                const ledc_driver_t *ledc_driver_if,
                                const gpio_driver_t *gpio_driver_if,
                                bsp_ili9488_handle_t *ili9488_handle);

/**
 * @brief   ILI9488 析构函数
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 *
 * @return 
 *      - ILI9488_OK
 *      - ILI9488_ERR_INVALID_ARG
 * 
 * @note 由上层调用删除设备实例
 */
ili9488_err_t ili9488_delete(bsp_ili9488_handle_t *ili9488_handle);


/**
 * @brief   ILI9488 初始化函数
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 *
 * @return 
 *      - ILI9488_OK
 *      - ILI9488_ERR_INVALID_ARG
 *
 * @note 由上层调用，初始化实例
 */
ili9488_err_t bsp_ili9488_init(bsp_ili9488_handle_t handle);

/**
 * @brief   ILI9488 反初始化函数
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 *
 * @return 
 *      - ILI9488_OK
 *      - ILI9488_ERR_INVALID_ARG
 *
 * @note 由上层调用，反初始化实例
 */
ili9488_err_t bsp_ili9488_deinit(bsp_ili9488_handle_t handle);

/**
 * @brief   ILI9488 软件复位函数
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 *
 * @return 
 *
 * @note 由上层调用
 */
ili9488_err_t bsp_ili9488_reset(bsp_ili9488_handle_t handle);

/**
 * @brief   ILI9488 设置显示窗口
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 * @param[in] x1 起始列
 * @param[in] y1 起始行
 * @param[in] x2 结束列
 * @param[in] y2 结束列
 *
 * @return 
 *
 * @note 由上层调用
 */
ili9488_err_t bsp_ili9488_setaddrwindow(bsp_ili9488_handle_t handle, uint16_t x1, 
                                        uint16_t y1, uint16_t x2, uint16_t y2);

/**
 * @brief   ILI9488 绘制单个像素点
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 * @param[in] x 像素x坐标
 * @param[in] y 像素y坐标
 * @param[in] color 像素颜色值
 * 
 * @return 
 *
 * @note 由上层调用
 */
ili9488_err_t bsp_ili9488_drawpixel(bsp_ili9488_handle_t handle, uint16_t x, 
                                    uint16_t y, uint16_t color);

/**
 * @brief   ILI9488 全屏填充颜色
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 * @param[in] color 填充颜色值
 * 
 * @return 
 *
 * @note 由上层调用
 */
ili9488_err_t bsp_ili9488_fillscreen(bsp_ili9488_handle_t handle, uint16_t color);

/**
 * @brief   ILI9488 填充矩形区域
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 * @param[in] x 左上角X坐标
 * @param[in] y 左上角y坐标
 * @param[in] w 填充矩形宽度
 * @param[in] h 填充矩形高度
 * @param[in] color 填充颜色值
 * 
 * @return 
 *
 * @note 由上层调用
 */
ili9488_err_t bsp_ili9488_fillrect(bsp_ili9488_handle_t handle, uint16_t x, 
                                    uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief   ILI9488 设置显示方向
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 * @param[in] direction 显示方向
 * 
 * @return 
 *
 * @note 由上层调用
 */
ili9488_err_t bsp_ili9488_setdirection(bsp_ili9488_handle_t handle, uint8_t rotation);

/**
 * @brief   ILI9488 开启显示
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 * 
 * @return 
 *
 * @note 由上层调用
 */
ili9488_err_t bsp_ili9488_displayon(bsp_ili9488_handle_t handle);

/**
 * @brief   ILI9488 关闭显示
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 * 
 * @return 
 *
 * @note 由上层调用
 */
ili9488_err_t bsp_ili9488_displayoff(bsp_ili9488_handle_t handle);

/**
 * @brief   ILI9488 进入睡眠模式
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 * 
 * @return 
 *
 * @note 由上层调用
 */
ili9488_err_t bsp_ili9488_sleepin(bsp_ili9488_handle_t handle);

/**
 * @brief   ILI9488 退出睡眠模式
 *
 * @param[in] ili9488_intance ILI9488 实例句柄
 * 
 * @return 
 *
 * @note 由上层调用
 */
ili9488_err_t bsp_ili9488_sleepout(bsp_ili9488_handle_t handle);


#ifdef __cplusplus
}
#endif
