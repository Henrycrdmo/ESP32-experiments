# CMakeLists.txt for MAX30102 component

idf_component_register(
    SRCS "main.c" 
         "i2c_api.c" 
         "max30102_api.c" 
         "algorithm.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_system
)