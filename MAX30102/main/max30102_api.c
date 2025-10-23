#include "max30102_api.h"
#include "i2c_api.h"
#include "esp_log.h"

void max30102_init(max_config *configuration)
{
    ESP_LOGI("MAX30102", "Initialising and configuring sensor...");

    // Reset sensor
    write_max30102_reg(0x40, REG_MODE_CONFIG);
    vTaskDelay(pdMS_TO_TICKS(100));
    uint8_t mode;
    do {
        read_max30102_reg(REG_MODE_CONFIG, &mode, 1);
    } while (mode & 0x40);

    // Default configuration
    write_max30102_reg(0xC0, REG_INTR_ENABLE_1);  // A_FULL_EN + PPG_RDY_EN
    write_max30102_reg(0x00, REG_INTR_ENABLE_2);  // No temp interrupt
    write_max30102_reg(0x00, REG_FIFO_WR_PTR);
    write_max30102_reg(0x00, REG_OVF_COUNTER);
    write_max30102_reg(0x00, REG_FIFO_RD_PTR);
    write_max30102_reg(0x4F, REG_FIFO_CONFIG);    // Average 4 samples, rollover enable
    write_max30102_reg(0x03, REG_MODE_CONFIG);    // SpO2 mode
    write_max30102_reg(0x27, REG_SPO2_CONFIG);    // 4096nA range, 100Hz, 18-bit
    write_max30102_reg(0x24, REG_LED1_PA);        // LED1 (RED) current
    write_max30102_reg(0x24, REG_LED2_PA);        // LED2 (IR) current
    write_max30102_reg(0x7F, REG_PILOT_PA);       // Pilot LED current
    write_max30102_reg(0x21, REG_MULTI_LED_CTRL1);// Slot1: RED, Slot2: IR
    write_max30102_reg(0x00, REG_MULTI_LED_CTRL2);

    ESP_LOGI("MAX30102", "Configuration complete");
}


esp_err_t max30102_read_fifo(i2c_port_t i2c_num, int32_t *red_data, int32_t *ir_data)
{
    uint8_t fifo[6];
    read_max30102_reg(REG_FIFO_DATA, fifo, 6);

    *red_data = (fifo[0] << 16) | (fifo[1] << 8) | fifo[2];
    *ir_data  = (fifo[3] << 16) | (fifo[4] << 8) | fifo[5];

    return ESP_OK;
}

void read_max30102_reg(uint8_t reg, uint8_t *data_reg, size_t len)
{
    i2c_sensor_write_addr(MAX30102_ADDR, &reg, 1);  // send register address
    i2c_sensor_read_addr(MAX30102_ADDR, data_reg, len); // then read data
}

void write_max30102_reg(uint8_t value, uint8_t reg)
{
    uint8_t data[2] = {reg, value};
    i2c_sensor_write_addr(MAX30102_ADDR, data, 2);  // uses i2c_api function to sends address
}


float get_max30102_temp()
{
	uint8_t int_temp;
	uint8_t decimal_temp;
	float temp = 0;
	write_max30102_reg(1, REG_TEMP_CONFIG);
	read_max30102_reg(REG_TEMP_INTR, &int_temp, 1);
	read_max30102_reg(REG_TEMP_FRAC, &decimal_temp, 1);
	temp = (int_temp + ((float)decimal_temp/10));
	return temp;
}


