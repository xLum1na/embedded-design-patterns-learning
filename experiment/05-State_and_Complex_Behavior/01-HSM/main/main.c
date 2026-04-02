// #include <stdio.h>
// #include "driver_fingerprint.h"
// #include "esp_log.h"
// #include "driver/uart.h"
// static const char *TAG = "appmain";

// void app_main(void)
// {
//     fp_config_t fp_cfg = {
//         .uart_port = UART_NUM_1,
//         .uart_txd = GPIO_NUM_16,
//         .uart_rxd = GPIO_NUM_17,
//         .rx_buffer_size = 256,
//         .tx_buffer_size = 256,
//         .baud_rate = BAUD_TATE_MEDIUM_SPEED,
//         .fp_mode = REAL_TIME_MODE,
//         .dev_addr = 0x00000000,
//         .user_data = NULL,
//     };
//     fp_handle_t handle = NULL;
//     esp_err_t ret;
//     ret = fp_init(&fp_cfg, &handle);
//     if (ret != ESP_OK) {
//         ESP_LOGW(TAG, "fp init fail\n");
//     }
//     vTaskDelay(pdMS_TO_TICKS(100)); 
//     ret = fp_open(handle);
//     if (ret != ESP_OK) {
//         ESP_LOGW(TAG, "fp open fail\n");
//     }
//     vTaskDelay(pdMS_TO_TICKS(100)); 
//     ret = fp_open_light(handle);
//     if (ret != ESP_OK) {
//         ESP_LOGW(TAG, "fp open light fail\n");
//     }
//     // vTaskDelay(pdMS_TO_TICKS(500));
//     // ret = fp_open_light(handle);
//     // if (ret != ESP_OK) {
//     //     ESP_LOGW(TAG, "fp open light fail\n");
//     // }
//     // ret = fp_set_mode(handle, ONE_JOG_MODE);
//     // if (ret != ESP_OK) {
//     //     ESP_LOGW(TAG, "fp set mode fail\n");
//     // }
//     // retfp_enroll(handle, 07);


//     while (1) {
//         uint8_t fp_id = 0;
//         ret = fp_verify(handle, &fp_id);
//         if (ret != ESP_OK) {
//             ESP_LOGW(TAG, "fp verify fail\n");
//         }
//         ESP_LOGI(TAG, "fp id is %d", fp_id);
//         vTaskDelay(pdMS_TO_TICKS(20));
//     }
    

// }
