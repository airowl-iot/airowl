#pragma once
#include <sdkconfig.h>
#ifdef CONFIG_ESP_MATTER_ENABLE_DATA_MODEL

#include <Matter.h>
#include <MatterEndPoint.h>

// Define Air Quality Cluster ID and Attribute IDs
namespace air_quality {
    constexpr uint32_t Id = 0x0000005B; // Cluster ID from cluster.h
    // constexpr uint32_t AttributeMeasuredValue = 0x00000001; // Attribute ID from attributes.h
    constexpr uint32_t AttributeAirQuality = 0x00000000; // Correct attribute ID for Air Quality enum
}

class MatterAirQualitySensor : public MatterEndPoint {
public:
    MatterAirQualitySensor();
    ~MatterAirQualitySensor();
    
    bool begin(uint16_t aqi = 0);
    bool setAQI(uint16_t aqi);
    uint16_t getAQI() const;
    void end();

    // Implement the required virtual function
    bool attributeChangeCB(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val) override;

protected:
    bool started = false;
    uint16_t aqiValue = 0;

private:
    bool setRawAQI(uint16_t aqi);
    uint8_t aqiToEnum(uint16_t aqi); // Helper to convert AQI to Matter enum
};
#endif
