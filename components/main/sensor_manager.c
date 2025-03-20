#include "sensor_manager.h"
#include "SensirionI2CSen5x.h"
#include "esp_log.h"
#include "string.h"

static const char *TAG = "sensor_manager";

// Global variables
volatile int AQI = 0;
volatile float temperature = 0.0;
volatile float humidity = 0.0;

// Sensor data structure
typedef struct {
    float pm1;
    float pm25;
    float pm10;
    float pm4;
    float tvoc;
    int pm1_max;
    int pm25_max;
    int pm10_max;
    int pm4_max;
    int tvoc_max;
    int count;
} sensor_data_t;

// AQI breakpoint structure
typedef struct {
    float Cp_Lo;  // Low concentration breakpoint
    float Cp_Hi;  // High concentration breakpoint
    int Ip_Lo;    // Low index breakpoint
    int Ip_Hi;    // High index breakpoint
} aqi_breakpoint_t;

// AQI breakpoints
static const aqi_breakpoint_t pm1_bps[] = {
    {0.0, 8.0, 0, 50},      {8.1, 25.4, 51, 100},
    {25.5, 35.4, 101, 150}, {35.5, 50.4, 151, 200},
    {50.5, 75.4, 201, 300}, {75.5, 500.4, 301, 500}
};

static const aqi_breakpoint_t pm4_bps[] = {
    {0.0, 35.0, 0, 50},       {35.1, 75.4, 51, 100},
    {75.5, 125.4, 101, 150},  {125.5, 175.4, 151, 200},
    {175.5, 250.4, 201, 300}, {250.5, 500.4, 301, 500}
};

static const aqi_breakpoint_t pm25_bps[] = {
    {0.0, 12.0, 0, 50},       {12.1, 35.4, 51, 100},
    {35.5, 55.4, 101, 150},   {55.5, 150.4, 151, 200},
    {150.5, 250.4, 201, 300}, {250.5, 500.4, 301, 500}
};

static const aqi_breakpoint_t pm10_bps[] = {
    {0, 54, 0, 50},       {55, 154, 51, 100},
    {155, 254, 101, 150}, {255, 354, 151, 200},
    {355, 424, 201, 300}, {425, 604, 301, 500}
};

static const aqi_breakpoint_t tvoc_bps[] = {
    {0.0, 300, 0, 50},      {300, 500, 51, 100},
    {500, 1000, 101, 150},  {1000, 3000, 151, 200},
    {4000, 5000, 201, 300}, {5000, 10000, 301, 500}
};

// MQTT configuration
static const char *MQTT_BROKER_URL = "mqtt://mqtt.oizom.com:1883";
static const char *MQTT_USERNAME = "xxxx";
static const char *MQTT_PASSWORD = "xxxx";
static const char *MQTT_TOPIC = "airowl";

// Global variables
static SensirionI2CSen5x sen5x;
static sensor_data_t sensor_data = {0};
static esp_mqtt_client_handle_t mqtt_client = NULL;
static char device_name[32] = {0};

// Function declarations
static int calculate_sub_index(float Cp, const aqi_breakpoint_t *bp);
static const aqi_breakpoint_t* get_breakpoint(float Cp, const aqi_breakpoint_t *bps, size_t num_bps);
static const char* get_aqi_category(int aqi);
static uint32_t get_aqi_color(int aqi);
static void setup_charts(void);
static esp_err_t mqtt_init(void);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

esp_err_t sensor_manager_init(void)
{
    // Initialize I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(I2C_NUM_0, &conf);
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));

    // Initialize SEN5x sensor
    uint16_t error;
    char errorMessage[256];
    
    error = sen5x.begin(I2C_NUM_0);
    if (error) {
        errorToString(error, errorMessage, 256);
        ESP_LOGE(TAG, "Error initializing SEN5x: %s", errorMessage);
        return ESP_FAIL;
    }

    // Reset sensor
    error = sen5x.deviceReset();
    if (error) {
        errorToString(error, errorMessage, 256);
        ESP_LOGE(TAG, "Error resetting SEN5x: %s", errorMessage);
        return ESP_FAIL;
    }

    // Set temperature offset
    float tempOffset = 0.0;
    error = sen5x.setTemperatureOffsetSimple(tempOffset);
    if (error) {
        errorToString(error, errorMessage, 256);
        ESP_LOGE(TAG, "Error setting temperature offset: %s", errorMessage);
        return ESP_FAIL;
    }

    // Start measurement
    error = sen5x.startMeasurement();
    if (error) {
        errorToString(error, errorMessage, 256);
        ESP_LOGE(TAG, "Error starting measurement: %s", errorMessage);
        return ESP_FAIL;
    }

    // Initialize MQTT
    ESP_ERROR_CHECK(mqtt_init());

    // Setup charts
    setup_charts();

    // Get device name from MAC address
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(device_name, sizeof(device_name), "AIROWL_%02X%02X%02X", mac[3], mac[4], mac[5]);

    return ESP_OK;
}

void sensor_manager_task(void *pvParameters)
{
    while (1) {
        uint16_t error;
        char errorMessage[256];

        // Read measurement values
        float t_pm1, t_pm25, t_pm4, t_pm10, t_hum, t_temp, vocIndex, noxIndex;
        error = sen5x.readMeasuredValues(t_pm1, t_pm25, t_pm4, t_pm10, t_hum, t_temp, vocIndex, noxIndex);

        if (error || isnan(t_pm1) || isnan(t_pm25) || isnan(t_pm4) || isnan(t_pm10) || isnan(vocIndex)) {
            errorToString(error, errorMessage, 256);
            ESP_LOGE(TAG, "Error reading measurements: %s", errorMessage);
        } else {
            // Update sensor data
            sensor_data.pm1 += t_pm1;
            sensor_data.pm25 += t_pm25;
            sensor_data.pm10 += t_pm10;
            sensor_data.pm4 += t_pm4;
            sensor_data.count++;

            // Update UI labels
            char buffer[8];
            
            snprintf(buffer, sizeof(buffer), "%.1f", t_pm1);
            lv_label_set_text(ui_pm1label, buffer);

            snprintf(buffer, sizeof(buffer), "%.1f", t_pm25);
            lv_label_set_text(ui_pm25label, buffer);

            snprintf(buffer, sizeof(buffer), "%.1f", t_pm4);
            lv_label_set_text(ui_pm4label, buffer);

            snprintf(buffer, sizeof(buffer), "%.1f", t_pm10);
            lv_label_set_text(ui_pm10label, buffer);

            if (!isnan(vocIndex)) {
                sensor_data.tvoc += vocIndex;
                snprintf(buffer, sizeof(buffer), "%.1f", vocIndex);
                lv_label_set_text(ui_tvoclabel, buffer);
            }

            if (!isnan(t_hum)) {
                snprintf(buffer, sizeof(buffer), "%.1f", t_hum);
                lv_label_set_text(ui_RHlabel, buffer);
                humidity = t_hum;
            }

            if (!isnan(t_temp)) {
                snprintf(buffer, sizeof(buffer), "%.1f", t_temp);
                lv_label_set_text(ui_templabel, buffer);
                temperature = t_temp;
            }

            // Process data when count reaches DATA_FREQ
            if (sensor_data.count == DATA_FREQ) {
                float avgPM1 = sensor_data.pm1 / sensor_data.count;
                float avgPM25 = sensor_data.pm25 / sensor_data.count;
                float avgPM10 = sensor_data.pm10 / sensor_data.count;
                float avgPM4 = sensor_data.pm4 / sensor_data.count;
                float avgTVOC = sensor_data.tvoc / sensor_data.count;

                // Calculate AQI indices
                const aqi_breakpoint_t *pm25_bp = get_breakpoint(avgPM25, pm25_bps, sizeof(pm25_bps)/sizeof(pm25_bps[0]));
                const aqi_breakpoint_t *pm10_bp = get_breakpoint(avgPM10, pm10_bps, sizeof(pm10_bps)/sizeof(pm10_bps[0]));
                const aqi_breakpoint_t *tvoc_bp = get_breakpoint(avgTVOC, tvoc_bps, sizeof(tvoc_bps)/sizeof(tvoc_bps[0]));
                const aqi_breakpoint_t *pm1_bp = get_breakpoint(avgPM1, pm1_bps, sizeof(pm1_bps)/sizeof(pm1_bps[0]));
                const aqi_breakpoint_t *pm4_bp = get_breakpoint(avgPM4, pm4_bps, sizeof(pm4_bps)/sizeof(pm4_bps[0]));

                int pm25_index = calculate_sub_index(avgPM25, pm25_bp);
                int pm10_index = calculate_sub_index(avgPM10, pm10_bp);
                int tvoc_index = calculate_sub_index(avgTVOC, tvoc_bp);
                int pm1_index = calculate_sub_index(avgPM1, pm1_bp);
                int pm4_index = calculate_sub_index(avgPM4, pm4_bp);

                // Update UI colors based on AQI
                uint32_t pm25_color = get_aqi_color(pm25_index);
                lv_obj_set_style_text_color(ui_pm25label, lv_color_hex(pm25_color), LV_PART_MAIN | LV_STATE_DEFAULT);

                uint32_t pm10_color = get_aqi_color(pm10_index);
                lv_obj_set_style_text_color(ui_pm10label, lv_color_hex(pm10_color), LV_PART_MAIN | LV_STATE_DEFAULT);

                uint32_t tvoc_color = get_aqi_color(tvoc_index);
                lv_obj_set_style_text_color(ui_tvoclabel, lv_color_hex(tvoc_color), LV_PART_MAIN | LV_STATE_DEFAULT);

                uint32_t pm1_color = get_aqi_color(pm1_index);
                lv_obj_set_style_text_color(ui_pm1label, lv_color_hex(pm1_color), LV_PART_MAIN | LV_STATE_DEFAULT);

                uint32_t pm4_color = get_aqi_color(pm4_index);
                lv_obj_set_style_text_color(ui_pm4label, lv_color_hex(pm4_color), LV_PART_MAIN | LV_STATE_DEFAULT);

                // Calculate overall AQI (using maximum sub-index)
                AQI = MAX(MAX(pm25_index, pm10_index), tvoc_index);

                // Update eye colors based on AQI
                uint32_t eye_color = get_aqi_color(AQI);
                lv_obj_set_style_bg_color(ui_lefteye, lv_color_hex(eye_color), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(ui_righteye, lv_color_hex(eye_color), LV_PART_MAIN | LV_STATE_DEFAULT);

                // Update charts
                update_charts(avgPM1, avgPM25, avgPM4, avgPM10, avgTVOC);

                // Publish data to MQTT if connected
                if (esp_mqtt_client_is_connected(mqtt_client)) {
                    lv_img_set_src(ui_nose, &ui_img_airowl_2_png);
                    publish_mqtt_data(avgPM1, avgPM25, avgPM10, avgPM4, avgTVOC);
                } else {
                    lv_img_set_src(ui_nose, &ui_img_airowl_1_png);
                }

                // Reset sensor data
                memset(&sensor_data, 0, sizeof(sensor_data));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// Helper functions
static int calculate_sub_index(float Cp, const aqi_breakpoint_t *bp)
{
    float Ip = ((bp->Ip_Hi - bp->Ip_Lo) / (bp->Cp_Hi - bp->Cp_Lo)) * (Cp - bp->Cp_Lo) + bp->Ip_Lo;
    return (int)round(Ip);
}

static const aqi_breakpoint_t* get_breakpoint(float Cp, const aqi_breakpoint_t *bps, size_t num_bps)
{
    for (size_t i = 0; i < num_bps; i++) {
        if (Cp >= bps[i].Cp_Lo && Cp <= bps[i].Cp_Hi) {
            return &bps[i];
        }
    }
    return &bps[num_bps - 1];
}

static const char* get_aqi_category(int aqi)
{
    if (aqi >= 0 && aqi <= 50) return "Good";
    if (aqi <= 100) return "Satisfactory";
    if (aqi <= 150) return "Moderate";
    if (aqi <= 200) return "Unhealthy";
    if (aqi <= 300) return "Very Unhealthy";
    return "Hazardous";
}

static uint32_t get_aqi_color(int aqi)
{
    if (aqi >= 0 && aqi <= 50) return 0x00E400;  // Good
    if (aqi <= 100) return 0x9CFF9C;  // Satisfactory
    if (aqi <= 150) return 0xFFFF00;  // Moderate
    if (aqi <= 200) return 0xFF7E00;  // Unhealthy
    if (aqi <= 300) return 0xFF0000;  // Very Unhealthy
    return 0x8F3F97;  // Hazardous
}

static esp_err_t mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT Error");
            break;
        default:
            break;
    }
}

static void publish_mqtt_data(float pm1, float pm25, float pm10, float pm4, float tvoc)
{
    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"deviceId\":\"%s\",\"p3\":%.2f,\"p1\":%.2f,\"p2\":%.2f,\"p5\":%.2f,\"v2\":%.2f}",
             device_name, pm1, pm25, pm10, pm4, tvoc);

    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, payload, 0, 1, 0);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish MQTT message");
    }
}