/**************************************************
 * @file    bsp_driver_spi.h
 * @brief   板级spi外设驱动定义
 * @author  xLumina
 * @version V1.0.0
 * @date    2026-4-8
 *
 * @par History:
 *   - V1.0.0 | 2026-4-8 | xLumina | 初始版本创建
 *
 * @par Function List:
 *      - bsp_spi_init()        : 初始化spi外设
 * 
 * 
 **************************************************/

/*=============== "bsp_driver_ledc.h" ===============*/
#include "bsp_driver_ledc.h"
/*=============== #include "driver/ledc.h" ===============*/
#include "driver/ledc.h"
/*=============== #include "esp_log.h" ===============*/
#include "esp_log.h"                /* espidf 日志库导入*/

/*************** spi日志标志 ***************/
static const char *TAG = "bsp_driver_spi";


void bsp_ledc_init(void)
{


}


bool bsp_ledc_set_duty(uint8_t duty)
{

    
}
