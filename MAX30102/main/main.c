#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/i2c.h"
#include "main.h"
#include "i2c_api.h"
#include "algorithm.h"
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

    xTaskCreatePinnedToCore(sensor_data_reader, "Data", 10240, NULL, 2, &sensor_reader_handle, 1);
}

void sensor_data_reader(void *pvParameters){
	i2c_init();
	vTaskDelay(pdMS_TO_TICKS(100));
	max30102_init(NULL);
	init_time_array();
	uint64_t ir_mean;
	uint64_t red_mean;
	float temperature;
	double r0_autocorrelation;

    for(;;){
        vTaskDelay(pdMS_TO_TICKS(100));
		fill_buffers_data();
		temperature = get_max30102_temp();
		remove_dc_part(ir_data_buffer, red_data_buffer, &ir_mean, &red_mean);
		remove_trend_line(ir_data_buffer);
		remove_trend_line(red_data_buffer);
		double pearson_correlation = correlation_datay_datax(red_data_buffer, ir_data_buffer);
		int heart_rate = calculate_heart_rate(ir_data_buffer, &r0_autocorrelation, auto_correlationated_data);

        #if DEBUG
            printf("\n");
            printf("HEART_RATE %d\n", heart_rate);
            printf("correlação %f\n", pearson_correlation);
            printf("Temperature %f\n", temperature);
        #endif

        // Both RED and IR signals must be strongly correlated
		if(pearson_correlation >= 0.7){
			double spo2 = spo2_measurement(ir_data_buffer, red_data_buffer, ir_mean, red_mean);
            #if DEBUG
			    printf("SPO2 %f\n", spo2);
            #endif
            
	        // size = asprintf(&data, "{\"mac\": \"%02x%02x%02x%02x%02x%02x\", \"spo2\":%f, \"heart_rate\":%d}",MAC2STR(sta_mac), spo2, heart_rate);
			// BLE_publish(data, size);
		}
		printf("\n");
    }
}

void fill_buffers_data()
{
	for(int i = 0; i < BUFFER_SIZE; i++){
		read_max30102_fifo(&red_data, &ir_data);
		ir_data_buffer[i] = ir_data;
		red_data_buffer[i] = red_data;
		//printf("%d\n", red_data);
		ir_data = 0;
		red_data = 0;
		vTaskDelay(pdMS_TO_TICKS(DELAY_AMOSTRAGEM));
	}
}