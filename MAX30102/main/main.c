#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/i2c.h"
#include "main.h"
#include "i2c_api.h"
#include "max30102_api.h"

static const char *TAG = "PPG_MAIN";

void app_main(void){
    ESP_LOGI(TAG, "Starting MAX30102 sensor...");

    // Initialise the I2C bus
    esp_err_t ret = i2c_init();
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "I2C initialised successfully");

    // Initialise MAX30102 (configs handled inside the MAX driver)
    max30102_init(NULL);
    ESP_LOGI(TAG, "MAX30102 initialised and configured");

    // 3. Read FIFO and log data
    int32_t red = 0, ir = 0;

    for(;;)
    {
        if (max30102_read_fifo(I2C_NUM_0, &red, &ir) == ESP_OK){   // Checks I2C connection
            ESP_LOGI(TAG, "RED: %ld | IR: %ld", red, ir);		   // Reads data
        } else {
            ESP_LOGW(TAG, "Failed to read FIFO");
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay (reads every second)
    }
}
