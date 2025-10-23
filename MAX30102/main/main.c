#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/i2c.h"
#include "main.h"
#include "i2c_api.h"
#include "max30102_api.h"

TaskHandle_t processor_handle = NULL;
TaskHandle_t sensor_reader_handle = NULL;

int32_t red_data = 0;
int32_t ir_data = 0;
int32_t red_data_buffer[BUFFER_SIZE];
int32_t ir_data_buffer[BUFFER_SIZE];
double auto_correlationated_data[BUFFER_SIZE];

char *data = NULL;

#define DELAY_AMOSTRAGEM 40

#define DEBUG true

static const char *TAG = "PPG_MAIN";

void app_main(void){
    
    #if DEBUG
        ESP_LOGI(TAG, "Initialising MAX30102 sensor...");
    #endif

    // Initialise the I2C bus
    esp_err_t ret = i2c_init();

    #if DEBUG
        if(ret != ESP_OK){
            ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(ret));
            return;
        }
        ESP_LOGI(TAG, "I2C initialised successfully");
    #endif

    // Initialise MAX30102 (configs handled inside the MAX driver)
    max30102_init(NULL);

    #if DEBUG
        ESP_LOGI(TAG, "MAX30102 initialised and configured");
    #endif

    for(;;)
    {
        if (max30102_read_fifo(I2C_NUM_0, &red_data, &ir_data) == ESP_OK){   // Checks I2C connection
            ESP_LOGI(TAG, "RED: %ld | IR: %ld", red_data, ir_data);		   // Reads data
        } else {
            ESP_LOGW(TAG, "Failed to read FIFO");
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Delay (reads every 1/10 second)
    }
}
