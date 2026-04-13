#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "hal_gpio.h"

static const char *TAG = "main";


void time_delay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}


void app_main(void)
{
    hal_gpio_handle_t led0 = NULL;
    hal_gpio_handle_t led1 = NULL;
    hal_gpio_config_t led0_cfg = {
        .name = "led0",
        .pin = 40,
        .mode = GPIO_OUTPUT,
        .pull_down = true, 
        .pull_up = false,
    };
    hal_gpio_init(&led0_cfg, &led0);
    hal_gpio_config_t led1_cfg = {
        .name = "led1",
        .pin = 41,
        .mode = GPIO_OUTPUT,
        .pull_down = true, 
        .pull_up = false,
    };
    hal_gpio_init(&led1_cfg, &led1);

    while (1) {
        hal_gpio_set_level(led0, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        hal_gpio_set_level(led0, 0);
        vTaskDelay(pdMS_TO_TICKS(500));

        hal_gpio_set_level(led1, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        hal_gpio_set_level(led1, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
}