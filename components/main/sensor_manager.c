#include <esp_log.h>
#include <driver/i2c_master.h>
#include <string.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_random.h>
#include "sensor_manager.h"
#include <stdio.h>
#include "driver/gpio.h"

static const char *TAG = "sensor_manager";

// Using default I2C pins for ESP32-S3 Dev Board
#define I2C_MASTER_SCL_IO           8       // GPIO for I2C SCL (clock)
#define I2C_MASTER_SDA_IO           9       // GPIO for I2C SDA (data)
#define I2C_MASTER_NUM              0       // I2C master port number
#define I2C_MASTER_FREQ_HZ          10000   // I2C master clock frequency - reduced further for stability
#define I2C_MASTER_TIMEOUT_MS       1000    // I2C timeout in milliseconds
#define SEN5X_I2C_ADDR              0x69    // Sensirion SEN5x I2C address
#define I2C_MASTER_WRITE           0x00    // I2C master write
#define I2C_MASTER_READ            0x01    // I2C master read
#define MAX_READ_RETRIES           999999999       // Maximum retries for reading data

// Define the breakpoint arrays for different pollutants
static const aqi_breakpoint_t pm1_breakpoints[] = {
    {0.0, 8.0, 0, 50},
    {8.1, 25.4, 51, 100},
    {25.5, 35.4, 101, 150},
    {35.5, 50.4, 151, 200},
    {50.5, 75.4, 201, 300},
    {75.5, 500.4, 301, 500}
};

static const aqi_breakpoint_t pm25_breakpoints[] = {
    {0.0, 12.0, 0, 50},
    {12.1, 35.4, 51, 100},
    {35.5, 55.4, 101, 150},
    {55.5, 150.4, 151, 200},
    {150.5, 250.4, 201, 300},
    {250.5, 500.4, 301, 500}
};

static const aqi_breakpoint_t pm4_breakpoints[] = {
    {0.0, 35.0, 0, 50},
    {35.1, 75.4, 51, 100},
    {75.5, 125.4, 101, 150},
    {125.5, 175.4, 151, 200},
    {175.5, 250.4, 201, 300},
    {250.5, 500.4, 301, 500}
};

static const aqi_breakpoint_t pm10_breakpoints[] = {
    {0, 54, 0, 50},
    {55, 154, 51, 100},
    {155, 254, 101, 150},
    {255, 354, 151, 200},
    {355, 424, 201, 300},
    {425, 604, 301, 500}
};

static const aqi_breakpoint_t tvoc_breakpoints[] = {
    {0.0, 300, 0, 50},
    {300, 500, 51, 100},
    {500, 1000, 101, 150},
    {1000, 3000, 151, 200},
    {4000, 5000, 201, 300},
    {5000, 10000, 301, 500}
};

// Global variables to track data
static sensor_avg_data_t avg_data = {0};
static int calculated_aqi = 0;
static const int DATA_FREQ = 5;  // Number of readings to average

static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t i2c_dev = NULL;
static bool sensor_detected = false;

// Function to calculate sub-index for AQI
static int calculate_sub_index(float concentration, const aqi_breakpoint_t *breakpoint) {
    float index = ((breakpoint->ip_hi - breakpoint->ip_lo) / 
                  (breakpoint->cp_hi - breakpoint->cp_lo)) * 
                  (concentration - breakpoint->cp_lo) + 
                  breakpoint->ip_lo;
    return (int)round(index);
}

// Function to find the appropriate breakpoint for a concentration
static aqi_breakpoint_t get_breakpoint(float concentration, const aqi_breakpoint_t *breakpoints, int num_breakpoints) {
    for (int i = 0; i < num_breakpoints; i++) {
        if (concentration >= breakpoints[i].cp_lo && concentration <= breakpoints[i].cp_hi) {
            return breakpoints[i];
        }
    }
    // Return the highest breakpoint if concentration exceeds the range
    return breakpoints[num_breakpoints - 1];
}

// Calculate CRC for Sensirion sensors
static uint8_t calculate_crc(const uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;  // Initialization value
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (size_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;  // Polynomial 0x31 (x^8 + x^5 + x^4 + 1)
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

// Initialize I2C for sensor communication
esp_err_t i2c_master_init(void) {
    ESP_LOGI(TAG, "Initializing I2C with frequency %d Hz", I2C_MASTER_FREQ_HZ);
    i2c_master_bus_config_t i2c_bus_config = {
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .trans_queue_depth = 20,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .device_address = SEN5X_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &i2c_dev));

    vTaskDelay(pdMS_TO_TICKS(200)); // Increase delay after I2C initialization
    return ESP_OK;
}

// Check if SEN5x sensor is present on the I2C bus
static bool sen5x_detect(void) {
    uint8_t cmd_bytes[2] = {0xD1, 0x00}; // Get device info command
    
    ESP_LOGI(TAG, "Checking for SEN5x sensor presence...");
    esp_err_t ret = i2c_master_transmit(i2c_dev, cmd_bytes, 2, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SEN5x sensor not detected: %s", esp_err_to_name(ret));
        return false;
    }
    
    // If we get here, the device responded to the address
    ESP_LOGI(TAG, "SEN5x sensor detected!");
    return true;
}

// Write data to SEN5x sensor
static esp_err_t sen5x_write_command(uint16_t command) {
    uint8_t cmd_bytes[2] = {(command >> 8) & 0xFF, command & 0xFF};
    
    ESP_LOGD(TAG, "Writing command 0x%04X to SEN5x", command);
    esp_err_t ret = i2c_master_transmit(i2c_dev, cmd_bytes, 2, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C write command 0x%04X failed: %s", command, esp_err_to_name(ret));
    }
    
    // Add a longer delay after each command
    vTaskDelay(pdMS_TO_TICKS(50));
    return ret;
}

// Read data from SEN5x sensor with CRC validation
static esp_err_t sen5x_read_data(uint16_t command, uint8_t *data, size_t data_len) {
    uint8_t cmd_bytes[2] = {(command >> 8) & 0xFF, command & 0xFF};
    
    ESP_LOGD(TAG, "Sending read command 0x%04X to SEN5x", command);
    esp_err_t ret = i2c_master_transmit(i2c_dev, cmd_bytes, 2, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C write for read command 0x%04X failed: %s", command, esp_err_to_name(ret));
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Increase delay for command processing
    
    // Calculate how many bytes we need to read (data + CRC)
    size_t total_bytes = data_len;
    if (command != 0xD100) { // If not reading device info which has a different format
        // For normal data reads, there's 1 CRC byte for each 2 data bytes
        total_bytes = data_len + (data_len / 2);
    }
    
    uint8_t *buffer = malloc(total_bytes);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for I2C read buffer");
        return ESP_ERR_NO_MEM;
    }
    
    // Clear buffer before reading
    memset(buffer, 0, total_bytes);
    
    // Try reading with retries
    for (int retry = 0; retry < MAX_READ_RETRIES; retry++) {
        ESP_LOGD(TAG, "Reading %d bytes from SEN5x (retry %d)", total_bytes, retry);
        ret = i2c_master_receive(i2c_dev, buffer, total_bytes, I2C_MASTER_TIMEOUT_MS);
        
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "I2C read attempt %d failed: %s", retry + 1, esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(50)); // Wait before retry
            continue;
        }
        
        // Log received bytes for debugging
        ESP_LOGD(TAG, "Received bytes: ");
        for (int i = 0; i < total_bytes; i++) {
            ESP_LOGD(TAG, "  [%d]: 0x%02X", i, buffer[i]);
        }
        
        // Process the data based on command
        if (command == 0xD100) { // Reading device info
            // Device info has a simple format, just copy the data
            memcpy(data, buffer, data_len);
            free(buffer);
            return ESP_OK;
        } else {
            // For normal data reads, we need to validate CRC and extract the actual data
            bool crc_valid = true;
            size_t data_idx = 0;
            
            for (size_t i = 0; i < total_bytes; i += 3) { // Process in groups of 3 bytes (2 data + 1 CRC)
                if (i + 2 < total_bytes) { // Make sure we have complete group
                    uint8_t crc = calculate_crc(&buffer[i], 2);
                    
                    ESP_LOGD(TAG, "Group %d: Data[0]=0x%02X, Data[1]=0x%02X, CRC=0x%02X, Calculated CRC=0x%02X", 
                            i/3, buffer[i], buffer[i+1], buffer[i+2], crc);
                            
                    if (crc != buffer[i + 2]) {
                        ESP_LOGE(TAG, "CRC check failed at index %d: calculated 0x%02X, received 0x%02X", 
                                i, crc, buffer[i + 2]);
                        crc_valid = false;
                        break;
                    }
                    
                    // Copy the validated data bytes
                    if (data_idx + 1 < data_len) {
                        data[data_idx++] = buffer[i];
                        data[data_idx++] = buffer[i + 1];
                    }
                }
            }
            
            if (crc_valid) {
                free(buffer);
                return ESP_OK;
            }
            
            // If CRC failed and we have more retries, try again
            if (retry < MAX_READ_RETRIES - 1) {
                ESP_LOGW(TAG, "CRC validation failed, retrying...");
                vTaskDelay(pdMS_TO_TICKS(100)); // Wait before retry
            }
        }
    }
    
    free(buffer);
    return ESP_ERR_INVALID_CRC;
}

esp_err_t sensor_manager_init(void) {
    ESP_LOGI(TAG, "Initializing sensor manager");
    
    // Initialize I2C
    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C");
        return err;
    }
    
    // Check if the sensor is present on the I2C bus
    sensor_detected = sen5x_detect();
    
    if (!sensor_detected) {
        ESP_LOGE(TAG, "Sensor not detected");
        return ESP_FAIL;
    }

    // Initialize SEN5x sensor
    for (int attempt = 1; attempt <= 3; attempt++) {
        ESP_LOGI(TAG, "Attempting to initialize SEN5x sensor (attempt %d/3)", attempt);
        
        // Reset sensor
        ESP_LOGI(TAG, "Resetting SEN5x sensor");
        err = sen5x_write_command(0xD304); // Reset command
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to reset SEN5x: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Wait for reset to complete
        
        // Start measurement
        err = sen5x_write_command(0x0021); // Start measurement command
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start measurement: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        
        // Wait for sensor to stabilize before first measurement
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        ESP_LOGI(TAG, "SEN5x sensor initialized successfully");
        return ESP_OK;
    }
    
    ESP_LOGE(TAG, "Failed to initialize SEN5x sensor after 3 attempts");
    return ESP_FAIL;
}

esp_err_t sensor_manager_read_data(sensor_data_t* data) {
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Initialize data to default/safe values
    memset(data, 0, sizeof(sensor_data_t));
    
    // Read real data from sensor
    uint8_t read_buffer[24]; // Buffer for measured values (6 values x 4 bytes)
    esp_err_t ret = sen5x_read_data(0x03C4, read_buffer, 24);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sensor data: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Convert the read bytes to sensor values (bytewise big-endian conversion)
    // Mass concentration PM1.0 [μg/m³]
    data->pm1 = (float)((read_buffer[0] << 8) | read_buffer[1]) / 10.0f;
    // Mass concentration PM2.5 [μg/m³]
    data->pm25 = (float)((read_buffer[2] << 8) | read_buffer[3]) / 10.0f;
    // Mass concentration PM4.0 [μg/m³]
    data->pm4 = (float)((read_buffer[4] << 8) | read_buffer[5]) / 10.0f;
    // Mass concentration PM10 [μg/m³]
    data->pm10 = (float)((read_buffer[6] << 8) | read_buffer[7]) / 10.0f;
    // Humidity [%RH]
    data->humidity = (float)((read_buffer[8] << 8) | read_buffer[9]) / 100.0f;
    // Temperature [°C]
    data->temperature = (float)((read_buffer[10] << 8) | read_buffer[11]) / 200.0f;
    // VOC index
    data->voc_index = (float)((read_buffer[12] << 8) | read_buffer[13]);
    // NOx index
    data->nox_index = (float)((read_buffer[14] << 8) | read_buffer[15]);
    
    // Update the rolling average data
    avg_data.pm1_sum += data->pm1;
    avg_data.pm25_sum += data->pm25;
    avg_data.pm4_sum += data->pm4;
    avg_data.pm10_sum += data->pm10;
    avg_data.voc_sum += data->voc_index;
    
    // Update max values
    if (data->pm1 > avg_data.pm1_max) avg_data.pm1_max = (int)data->pm1;
    if (data->pm25 > avg_data.pm25_max) avg_data.pm25_max = (int)data->pm25;
    if (data->pm4 > avg_data.pm4_max) avg_data.pm4_max = (int)data->pm4;
    if (data->pm10 > avg_data.pm10_max) avg_data.pm10_max = (int)data->pm10;
    if (data->voc_index > avg_data.voc_max) avg_data.voc_max = (int)data->voc_index;
    
    avg_data.count++;
    
    // Calculate AQI after collecting enough samples
    if (avg_data.count >= DATA_FREQ) {
        // Calculate averages
        float avg_pm1 = avg_data.pm1_sum / avg_data.count;
        float avg_pm25 = avg_data.pm25_sum / avg_data.count;
        float avg_pm4 = avg_data.pm4_sum / avg_data.count;
        float avg_pm10 = avg_data.pm10_sum / avg_data.count;
        float avg_voc = avg_data.voc_sum / avg_data.count;
        
        // Calculate AQI for each pollutant
        aqi_breakpoint_t pm1_bp = get_breakpoint(avg_pm1, pm1_breakpoints, 6);
        int pm1_aqi = calculate_sub_index(avg_pm1, &pm1_bp);
        
        aqi_breakpoint_t pm25_bp = get_breakpoint(avg_pm25, pm25_breakpoints, 6);
        int pm25_aqi = calculate_sub_index(avg_pm25, &pm25_bp);
        
        aqi_breakpoint_t pm4_bp = get_breakpoint(avg_pm4, pm4_breakpoints, 6);
        int pm4_aqi = calculate_sub_index(avg_pm4, &pm4_bp);
        
        aqi_breakpoint_t pm10_bp = get_breakpoint(avg_pm10, pm10_breakpoints, 6);
        int pm10_aqi = calculate_sub_index(avg_pm10, &pm10_bp);
        
        aqi_breakpoint_t tvoc_bp = get_breakpoint(avg_voc, tvoc_breakpoints, 6);
        int tvoc_aqi = calculate_sub_index(avg_voc, &tvoc_bp);
        
        // Overall AQI is the maximum of individual AQIs
        int max_aqi = pm1_aqi;
        if (pm25_aqi > max_aqi) max_aqi = pm25_aqi;
        if (pm4_aqi > max_aqi) max_aqi = pm4_aqi;
        if (pm10_aqi > max_aqi) max_aqi = pm10_aqi;
        if (tvoc_aqi > max_aqi) max_aqi = tvoc_aqi;
        
        calculated_aqi = max_aqi;
        data->aqi = calculated_aqi;
        
        // Reset the averaging data
        memset(&avg_data, 0, sizeof(avg_data));
    } else {
        // If we haven't collected enough samples, use the previous AQI
        data->aqi = calculated_aqi;
    }
    
    ESP_LOGI(TAG, "SEN5x readings - PM1.0: %.2f, PM2.5: %.2f, PM4.0: %.2f, PM10: %.2f, Temp: %.2f, Hum: %.2f, VOC: %.2f, NOx: %.2f, AQI: %d",
             data->pm1, data->pm25, data->pm4, data->pm10, 
             data->temperature, data->humidity, 
             data->voc_index, data->nox_index, 
             data->aqi);
    
    return ESP_OK;
}

int sensor_manager_get_aqi(void) {
    return calculated_aqi;
}

const char* sensor_manager_get_aqi_category(int aqi) {
    if (aqi <= 50) {
        return "Good";
    } else if (aqi <= 100) {
        return "Moderate";
    } else if (aqi <= 150) {
        return "Unhealthy for Sensitive Groups";
    } else if (aqi <= 200) {
        return "Unhealthy";
    } else if (aqi <= 300) {
        return "Very Unhealthy";
    } else {
        return "Hazardous";
    }
}

uint32_t sensor_manager_get_aqi_color(int aqi) {
    if (aqi <= 50) {
        return 0x00FF00; // Green
    } else if (aqi <= 100) {
        return 0xFFFF00; // Yellow
    } else if (aqi <= 150) {
        return 0xFF9900; // Orange
    } else if (aqi <= 200) {
        return 0xFF0000; // Red
    } else if (aqi <= 300) {
        return 0x990099; // Purple
    } else {
        return 0x660000; // Maroon
    }
}

esp_err_t sensor_manager_deinit(void) {
    if (i2c_dev) {
        i2c_master_bus_rm_device(i2c_dev);
        i2c_dev = NULL;
    }
    if (bus_handle) {
        i2c_del_master_bus(bus_handle);
        bus_handle = NULL;
    }
    return ESP_OK;
} 