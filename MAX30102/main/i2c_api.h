#ifndef I2C_API_H
#define I2C_API_H

#include "esp_err.h"
#include "driver/i2c.h"
#include <stdint.h>
#include <stddef.h>

#define MAX30102_ADDR 0x57

#define ACK_CHECK_EN 0x1
#define I2C_MASTER_RX_BUF_DISABLE 0
#define I2C_MASTER_TX_BUF_DISABLE 0
#define ACK_VAL 0x0
#define NACK_VAL 0x1

#define SDA_PIN 33
#define SCL_PIN 32


esp_err_t i2c_init(void);
esp_err_t i2c_sensor_read(uint8_t *data_rd, size_t size);
esp_err_t i2c_sensor_write(uint8_t *data_wr, size_t size);

esp_err_t i2c_sensor_read_addr(uint8_t dev_addr, uint8_t *data_rd, size_t size);
esp_err_t i2c_sensor_write_addr(uint8_t dev_addr, uint8_t *data_wr, size_t size);

void read_max30102_fifo(int32_t *red_data, int32_t *ir_data);

#endif
