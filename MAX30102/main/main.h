#ifndef MAIN_H
#define MAIN_H

#include "esp_err.h"
#include "max30102_api.h"


void sensor_data_processor(void *pvParameters);
void sensor_data_reader(void *pvParameters);
void fill_buffers_data();

#define BUFFER_SIZE 128 // 

#endif
