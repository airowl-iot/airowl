#include "MatterAirQualitySensor.h"
using namespace esp_matter;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

MatterAirQualitySensor::MatterAirQualitySensor() {}
MatterAirQualitySensor::~MatterAirQualitySensor() { end(); }

bool MatterAirQualitySensor::begin(uint16_t aqi) {
    if (getEndPointId() != 0) {
        log_e("Sensor already created with Endpoint ID %d", getEndPointId());
        return false;
    }

    // Ensure the Matter node is initialized
    if (!node::get()) {
        ArduinoMatter::_init(); // Call protected _init() to create the node
        if (!node::get()) {
            log_e("Failed to initialize Matter node.");
            return false;
        }
    }

    air_quality_sensor::config_t config = {};
    config.air_quality.cluster_revision = 1;

    // Create the endpoint with the Air Quality cluster
    endpoint_t *endpoint = air_quality_sensor::create(node::get(), &config, ENDPOINT_FLAG_NONE, this);
    if (!endpoint) {
        log_e("Failed to create Air Quality Sensor endpoint");
        return false;
    }

    aqiValue = aqi;
    setEndPointId(endpoint::get_id(endpoint));
    started = true;
    log_i("Air Quality Sensor created with Endpoint ID %d", getEndPointId());

    // Set initial AQI and category
    if (!setRawAQI(aqi)) {
        log_e("Failed to set initial AQI");
        end();
        return false;
    }

    return true;
}

bool MatterAirQualitySensor::setRawAQI(uint16_t aqi) {
    if (!started) {
        log_e("Sensor not started");
        return false;
    }

    // Update MeasuredValue (raw AQI)
    esp_matter_attr_val_t aqiVal = esp_matter_invalid(NULL);
    aqiVal.type = ESP_MATTER_VAL_TYPE_UINT16;
    aqiVal.val.u16 = aqi;

    // Update AirQuality enum based on AQI
    uint8_t qualityEnum = aqiToEnum(aqi);
    esp_matter_attr_val_t qualityVal = esp_matter_invalid(NULL);
    qualityVal.type = ESP_MATTER_VAL_TYPE_UINT8;
    qualityVal.val.u8 = qualityEnum;

    if (!updateAttributeVal(air_quality::Id, air_quality::AttributeAirQuality, &qualityVal)) {
        log_e("Failed to update AirQuality");
        return false;
    }
    log_d("Updated AirQuality: %d", qualityEnum);

    return true;
}

uint8_t MatterAirQualitySensor::aqiToEnum(uint16_t aqi) {
    if (aqi <= 50) {
        return static_cast<uint8_t>(AirQuality::AirQualityEnum::kGood);          // Good (0-50)
    } else if (aqi <= 100) {
        return static_cast<uint8_t>(AirQuality::AirQualityEnum::kFair);          // Fair (51-100)
    } else if (aqi <= 150) {
        return static_cast<uint8_t>(AirQuality::AirQualityEnum::kModerate);      // Moderate (101-150)
    } else if (aqi <= 200) {
        return static_cast<uint8_t>(AirQuality::AirQualityEnum::kPoor);          // Poor (151-200)
    } else if (aqi <= 300) {
        return static_cast<uint8_t>(AirQuality::AirQualityEnum::kVeryPoor);      // Very Poor (201-300)
    } else {
        return static_cast<uint8_t>(AirQuality::AirQualityEnum::kExtremelyPoor); // Extremely Poor (301+)
    }
}

bool MatterAirQualitySensor::setAQI(uint16_t aqi) {
    return setRawAQI(aqi);
}

uint16_t MatterAirQualitySensor::getAQI() const {
    return aqiValue;
}

bool MatterAirQualitySensor::attributeChangeCB(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val) {
    log_d("Attribute changed: endpoint=%u, cluster=%u, attribute=%u", endpoint_id, cluster_id, attribute_id);
    return true;
}

void MatterAirQualitySensor::end() {
    started = false;
}
