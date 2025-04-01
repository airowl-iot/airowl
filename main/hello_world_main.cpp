#include <M5Unified.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"

#define I2C_MASTER_NUM           I2C_NUM_0
#define I2C_MASTER_FREQ_HZ       100000
#define I2C_MASTER_SDA_IO        2       // G2
#define I2C_MASTER_SCL_IO        1       // G1
#define SEN54_ADDR               0x69    // SEL=GND → 0x69

// SEN54 commands
#define START_MEASUREMENT        0x0021
#define READ_MEASUREMENT         0x03C4
#define START_FAN_CLEANING       0x5607
#define DEVICE_RESET             0xD304

static const char *TAG = "SEN54";

typedef struct {
    float pm1;
    float pm25;
    float pm4;
    float pm10;
    float tvoc;
    float temperature;
    float humidity;
} SensorReadings;

// Add these color functions at the top, after the includes
uint16_t get_pm_color(float pm_value) {
    if (pm_value <= 12.0f) return 0x07E0;      // Green for Good
    else if (pm_value <= 35.4f) return 0xFFE0;  // Yellow for Moderate
    else if (pm_value <= 55.4f) return 0xFD20;  // Orange for Unhealthy for Sensitive Groups
    else if (pm_value <= 150.4f) return 0xF800; // Red for Unhealthy
    else if (pm_value <= 250.4f) return 0x780F; // Purple for Very Unhealthy
    return 0x8811;                              // Maroon for Hazardous
}

uint16_t get_temp_color(float temp) {
    if (temp < 0) return 0x001F;       // Blue for very cold
    else if (temp < 18) return 0x07FF;  // Cyan for cool
    else if (temp < 24) return 0x07E0;  // Green for comfortable
    else if (temp < 30) return 0xFFE0;  // Yellow for warm
    return 0xF800;                      // Red for hot
}

uint16_t get_humidity_color(float humidity) {
    if (humidity < 30) return 0xF800;     // Red for too dry
    else if (humidity < 40) return 0xFD20; // Orange for dry
    else if (humidity < 60) return 0x07E0; // Green for comfortable
    else if (humidity < 70) return 0x07FF; // Cyan for humid
    return 0x001F;                        // Blue for very humid
}

uint16_t get_tvoc_color(float tvoc) {
    if (tvoc <= 50) return 0x07E0;      // Green for Good
    else if (tvoc <= 100) return 0xFFE0; // Yellow for Moderate
    else if (tvoc <= 150) return 0xFD20; // Orange for Unhealthy for Sensitive Groups
    return 0xF800;                       // Red for Unhealthy
}

void i2c_master_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = I2C_MASTER_FREQ_HZ,
        },
        .clk_flags = 0,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
}

esp_err_t sen54_write_cmd(uint16_t command) {
    uint8_t cmd[2] = {(uint8_t)(command >> 8), (uint8_t)(command & 0xFF)};
    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, SEN54_ADDR, cmd, sizeof(cmd), pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write command 0x%04X failed: 0x%x", command, ret);
    }
    return ret;
}

esp_err_t sen54_read_data(uint8_t *data, size_t len) {
    esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_NUM, SEN54_ADDR, data, len, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: 0x%x", ret);
    }
    return ret;
}

void initialize_sensor() {
    ESP_LOGI(TAG, "Resetting sensor");
    ESP_ERROR_CHECK(sen54_write_cmd(DEVICE_RESET));
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "Starting fan cleaning");
    ESP_ERROR_CHECK(sen54_write_cmd(START_FAN_CLEANING));
    vTaskDelay(pdMS_TO_TICKS(10000));
    
    ESP_LOGI(TAG, "Starting measurements");
    ESP_ERROR_CHECK(sen54_write_cmd(START_MEASUREMENT));
    vTaskDelay(pdMS_TO_TICKS(2000));
}

bool read_sensor_values(SensorReadings &readings) {
    uint8_t data[24] = {0};
    
    // Send read command
    if(sen54_write_cmd(READ_MEASUREMENT) != ESP_OK) {
        return false;
    }
    
    // Add small delay before reading
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Read data
    if(sen54_read_data(data, sizeof(data)) != ESP_OK) {
        return false;
    }
    
    // Parse values (SEN54 data format)
    readings.pm1 = ((data[0] << 8) | data[1]) / 10.0f;
    readings.pm25 = ((data[3] << 8) | data[4]) / 10.0f;
    readings.pm4 = ((data[6] << 8) | data[7]) / 10.0f;
    readings.pm10 = ((data[9] << 8) | data[10]) / 10.0f;
    readings.humidity = ((data[12] << 8) | data[13]) / 100.0f;
    readings.temperature = ((data[15] << 8) | data[16]) / 200.0f;
    readings.tvoc = ((data[18] << 8) | data[19]) / 10.0f;
    
    // Validate readings
    if (readings.pm1 > 1000 || readings.pm25 > 1000 || 
        readings.temperature > 100 || readings.temperature < -40 ||
        readings.humidity > 100 || readings.humidity < 0) {
        ESP_LOGE(TAG, "Invalid sensor readings");
        return false;
    }
    
    return true;
}

void update_display(const SensorReadings &readings) {
    // Create a sprite for double buffering
    static LGFX_Sprite sprite(&M5.Display);
    static bool sprite_initialized = false;
    
    if (!sprite_initialized) {
        sprite.createSprite(M5.Display.width(), M5.Display.height());
        sprite_initialized = true;
    }
    
    // Draw everything to the sprite first
    sprite.clear(TFT_BLACK);
    
    // Constants for layout
    const int STATUS_HEIGHT = 30;
    const int GRID_START = STATUS_HEIGHT;
    const int GRID_MARGIN = 8;
    const int UPPER_CELL_HEIGHT = 72;
    const int LOWER_CELL_HEIGHT = 52;
    const int CELL_WIDTH = M5.Display.width() / 2;
    const int TVOC_HEIGHT = 45;
    const int TVOC_GAP = 25;
    
    // Calculate positions
    const int PM_GRID_HEIGHT = UPPER_CELL_HEIGHT + LOWER_CELL_HEIGHT;
    const int TVOC_Y = GRID_START + PM_GRID_HEIGHT + TVOC_GAP;
    
    // Draw borders
    // Status bar border - only draw left, top, and right sides
    sprite.drawLine(0, 0, M5.Display.width()-1, 0, TFT_WHITE);  // Top
    sprite.drawLine(0, 0, 0, STATUS_HEIGHT, TFT_WHITE);         // Left
    sprite.drawLine(M5.Display.width()-1, 0, M5.Display.width()-1, STATUS_HEIGHT, TFT_WHITE);  // Right
    
    // Combined PM values and TVOC border
    sprite.drawRect(0, GRID_START, M5.Display.width(), TVOC_Y + TVOC_HEIGHT - GRID_START, TFT_WHITE);
    
    // Internal grid lines for PM values
    sprite.drawLine(CELL_WIDTH, GRID_START, CELL_WIDTH, TVOC_Y, TFT_WHITE);
    sprite.drawLine(0, GRID_START + UPPER_CELL_HEIGHT, M5.Display.width(), GRID_START + UPPER_CELL_HEIGHT, TFT_WHITE);
    
    // Horizontal line separating PM grid from TVOC
    sprite.drawLine(0, TVOC_Y, M5.Display.width(), TVOC_Y, TFT_WHITE);
    
    // Status bar content
    // Temperature (left)
    sprite.setFont(&fonts::DejaVu18);
    sprite.setTextColor(get_temp_color(readings.temperature));
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.1f", readings.temperature);
    sprite.setCursor(4, 6);
    sprite.print(tempStr);
    sprite.setFont(&fonts::DejaVu12);
    sprite.printf(" %cC", 0xB0);
    
    // Time (center)
    sprite.setTextColor(TFT_WHITE);
    sprite.setFont(&fonts::DejaVu18);
    char timeStr[9];
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    int timeWidth = sprite.textWidth(timeStr);
    int centerX = (M5.Display.width() - timeWidth) / 2;
    sprite.setCursor(centerX, 6);
    sprite.print(timeStr);
    
    // Humidity (right)
    sprite.setTextColor(get_humidity_color(readings.humidity));
    sprite.setFont(&fonts::DejaVu18);
    char humStr[16];
    snprintf(humStr, sizeof(humStr), "%.1f", readings.humidity);
    int humWidth = sprite.textWidth(humStr);
    int rhWidth = sprite.textWidth(" RH%");
    sprite.setCursor(M5.Display.width() - humWidth - rhWidth - 4, 6);
    sprite.print(humStr);
    sprite.setFont(&fonts::DejaVu12);
    sprite.print(" RH%");
    
    // Function to draw PM values with adjusted fonts and standardized positioning
    auto drawPMValue = [&](const char* label, float value, int x, int y) {
        const int LABEL_Y_OFFSET = 8;
        const int VALUE_Y_OFFSET = 35;
        const int UNIT_X_OFFSET = 6;
        
        // Draw label with standardized positioning
        sprite.setTextColor(TFT_WHITE);
        sprite.setFont(&fonts::DejaVu24);
        sprite.setCursor(x + UNIT_X_OFFSET, y + LABEL_Y_OFFSET);
        sprite.print(label);
        sprite.setFont(&fonts::DejaVu12);
        sprite.print(" ug/m³");
        
        // Draw value with standardized positioning
        sprite.setTextColor(get_pm_color(value));
        sprite.setFont(&fonts::DejaVu24);
        char valueStr[16];
        snprintf(valueStr, sizeof(valueStr), "%.1f", value);
        int valueWidth = sprite.textWidth(valueStr);
        int centerX = x + (CELL_WIDTH - valueWidth) / 2;
        sprite.setCursor(centerX, y + VALUE_Y_OFFSET);
        sprite.print(valueStr);
    };
    
    // Draw PM values
    drawPMValue("PM1.0", readings.pm1, 0, GRID_START);
    drawPMValue("PM4.0", readings.pm4, CELL_WIDTH, GRID_START);
    drawPMValue("PM2.5", readings.pm25, 0, GRID_START + UPPER_CELL_HEIGHT);
    drawPMValue("PM10.0", readings.pm10, CELL_WIDTH, GRID_START + UPPER_CELL_HEIGHT);
    
    // TVOC section
    // TVOC label
    sprite.setTextColor(TFT_WHITE);
    sprite.setFont(&fonts::DejaVu24);
    sprite.setCursor(GRID_MARGIN + 4, TVOC_Y + (TVOC_HEIGHT - 24)/2);
    sprite.print("TVOC Index");
    
    // TVOC value
    sprite.setTextColor(get_tvoc_color(readings.tvoc));
    sprite.setFont(&fonts::DejaVu24);
    char tvocStr[16];
    snprintf(tvocStr, sizeof(tvocStr), "%.1f", readings.tvoc);
    int tvocWidth = sprite.textWidth(tvocStr);
    sprite.setCursor(M5.Display.width() - tvocWidth - GRID_MARGIN - 20, TVOC_Y + (TVOC_HEIGHT - 24)/2);
    sprite.print(tvocStr);
    
    // Push the sprite to the display in one go
    sprite.pushSprite(0, 0);
}

void check_i2c_bus() {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SEN54_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Sensor found at address 0x%02X", SEN54_ADDR);
    } else {
        ESP_LOGE(TAG, "Sensor not found at address 0x%02X (error: 0x%x)", SEN54_ADDR, ret);
    }
}

bool readings_changed(const SensorReadings &old_readings, const SensorReadings &new_readings) {
    // Check if any value has changed significantly (using small epsilon for float comparison)
    const float epsilon = 0.1f;  // Tolerance for float comparison
    return (fabs(old_readings.pm1 - new_readings.pm1) > epsilon ||
            fabs(old_readings.pm25 - new_readings.pm25) > epsilon ||
            fabs(old_readings.pm4 - new_readings.pm4) > epsilon ||
            fabs(old_readings.pm10 - new_readings.pm10) > epsilon ||
            fabs(old_readings.tvoc - new_readings.tvoc) > epsilon ||
            fabs(old_readings.temperature - new_readings.temperature) > epsilon ||
            fabs(old_readings.humidity - new_readings.humidity) > epsilon);
}

extern "C" void app_main() {
    M5.begin();
    M5.Display.setRotation(3);
    M5.Display.clear(TFT_BLACK);

    ESP_LOGI(TAG, "Initializing I2C");
    i2c_master_init();
    
    check_i2c_bus();
    
    ESP_LOGI(TAG, "Initializing SEN54");
    initialize_sensor();

    SensorReadings readings = {0};
    SensorReadings previous_readings = {0};
    bool first_reading = true;
    
    while (true) {
        if (read_sensor_values(readings)) {
            // Update display only on first reading or when values change
            if (first_reading || readings_changed(previous_readings, readings)) {
                update_display(readings);
                previous_readings = readings;
                first_reading = false;
                
                ESP_LOGI(TAG, "Readings: PM1=%.1f PM2.5=%.1f PM4=%.1f PM10=%.1f TVOC=%.1f Temp=%.1f°C Hum=%.1f%%",
                        readings.pm1, readings.pm25, readings.pm4, readings.pm10,
                        readings.tvoc, readings.temperature, readings.humidity);
            }
        } else {
            ESP_LOGW(TAG, "Failed to read sensor - retrying...");
            M5.Display.clear(TFT_RED);
            M5.Display.setCursor(10, 10);
            M5.Display.print("Sensor Error!");
            
            // Try to reinitialize sensor
            initialize_sensor();
            first_reading = true;  // Force display update after reinitialize
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}