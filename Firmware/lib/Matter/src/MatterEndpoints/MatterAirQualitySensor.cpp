#include "MatterAirQualitySensor.h"
#include <sdkconfig.h>
#ifdef CONFIG_ESP_MATTER_ENABLE_DATA_MODEL

using namespace esp_matter;
using namespace esp_matter::endpoint;
using namespace esp_matter::cluster;
using namespace chip::app::Clusters;

static const char *TAG = "MatterAirQualitySensor";

MatterAirQualitySensor::MatterAirQualitySensor() {}

MatterAirQualitySensor::~MatterAirQualitySensor() { 
    end(); 
}

bool MatterAirQualitySensor::begin(uint8_t _enabledMeasurements) {
    if (getEndPointId() != 0) {
        log_e("Air Quality Sensor already created with Endpoint ID %d", getEndPointId());
        return false;
    }

    if (!node::get()) {
        ArduinoMatter::_init();
        if (!node::get()) {
            log_e("Failed to initialize Matter node.");
            return false;
        }
    }

    enabledMeasurements = _enabledMeasurements;

    air_quality_sensor::config_t air_quality_config;

    endpoint_t *endpoint = air_quality_sensor::create(node::get(), &air_quality_config, ENDPOINT_FLAG_NONE, this);
    if (!endpoint) {
        log_e("Failed to create Air Quality Sensor endpoint");
        return false;
    }

    setEndPointId(endpoint::get_id(endpoint));
    log_i("Air Quality Sensor created with Endpoint ID %d", getEndPointId());

    // if (enabledMeasurements & ENABLE_CO2) {
    //     addCarbonDioxideConcentrationMeasurementCluster();
    // }
    // if (enabledMeasurements & ENABLE_TVOC) {
    //     addTotalVolatileOrganicCompoundsConcentrationMeasurementCluster();
    // }
    if (enabledMeasurements & ENABLE_PM1) {
        addPm1ConcentrationMeasurementCluster();
    }
    if (enabledMeasurements & ENABLE_PM25) {
        addPm25ConcentrationMeasurementCluster();
    }
    if (enabledMeasurements & ENABLE_PM10) {
        addPm10ConcentrationMeasurementCluster();
    }

    started = true;
    updateAirQualityEnum();
    return true;
}

// void MatterAirQualitySensor::addCarbonDioxideConcentrationMeasurementCluster() {
//     carbon_dioxide_concentration_measurement::config_t cluster_config;
//     cluster_config.measurement_medium = static_cast<uint8_t>(CarbonDioxideConcentrationMeasurement::MeasurementMediumEnum::kAir);
    
//     cluster_t* cluster = carbon_dioxide_concentration_measurement::create(
//         endpoint::get(node::get(), getEndPointId()), 
//         &cluster_config, 
//         CLUSTER_FLAG_SERVER
//     );

//     if (cluster) {
//         carbon_dioxide_concentration_measurement::feature::numeric_measurement::config_t numeric_config;
//         numeric_config.measurement_unit = static_cast<uint8_t>(CarbonDioxideConcentrationMeasurement::MeasurementUnitEnum::kPpm);
//         carbon_dioxide_concentration_measurement::feature::numeric_measurement::add(cluster, &numeric_config);
        
//         log_i("Added CO2 Concentration Measurement cluster");
//     } else {
//         log_e("Failed to add CO2 Concentration Measurement cluster");
//     }
// }

// void MatterAirQualitySensor::addTotalVolatileOrganicCompoundsConcentrationMeasurementCluster() {
//     total_volatile_organic_compounds_concentration_measurement::config_t cluster_config;
//     cluster_config.measurement_medium = static_cast<uint8_t>(TotalVolatileOrganicCompoundsConcentrationMeasurement::MeasurementMediumEnum::kAir);
    
//     cluster_t* cluster = total_volatile_organic_compounds_concentration_measurement::create(
//         endpoint::get(node::get(), getEndPointId()), 
//         &cluster_config, 
//         CLUSTER_FLAG_SERVER
//     );

//     if (cluster) {
//         total_volatile_organic_compounds_concentration_measurement::feature::numeric_measurement::config_t numeric_config;
//         numeric_config.measurement_unit = static_cast<uint8_t>(TotalVolatileOrganicCompoundsConcentrationMeasurement::MeasurementUnitEnum::kPpb);
//         total_volatile_organic_compounds_concentration_measurement::feature::numeric_measurement::add(cluster, &numeric_config);
        
//         log_i("Added TVOC Concentration Measurement cluster");
//     } else {
//         log_e("Failed to add TVOC Concentration Measurement cluster");
//     }
// }

void MatterAirQualitySensor::addPm1ConcentrationMeasurementCluster() {
    pm1_concentration_measurement::config_t cluster_config;
    cluster_config.measurement_medium = static_cast<uint8_t>(Pm1ConcentrationMeasurement::MeasurementMediumEnum::kAir);
    
    cluster_t* cluster = pm1_concentration_measurement::create(
        endpoint::get(node::get(), getEndPointId()), 
        &cluster_config, 
        CLUSTER_FLAG_SERVER
    );

    if (cluster) {
        pm1_concentration_measurement::feature::numeric_measurement::config_t numeric_config;
        numeric_config.measurement_unit = static_cast<uint8_t>(Pm1ConcentrationMeasurement::MeasurementUnitEnum::kUgm3);
        pm1_concentration_measurement::feature::numeric_measurement::add(cluster, &numeric_config);
        
        log_i("Added PM1.0 Concentration Measurement cluster");
    } else {
        log_e("Failed to add PM1.0 Concentration Measurement cluster");
    }
}

void MatterAirQualitySensor::addPm25ConcentrationMeasurementCluster() {
    pm25_concentration_measurement::config_t cluster_config;
    cluster_config.measurement_medium = static_cast<uint8_t>(Pm25ConcentrationMeasurement::MeasurementMediumEnum::kAir);
    
    cluster_t* cluster = pm25_concentration_measurement::create(
        endpoint::get(node::get(), getEndPointId()), 
        &cluster_config, 
        CLUSTER_FLAG_SERVER
    );

    if (cluster) {
        pm25_concentration_measurement::feature::numeric_measurement::config_t numeric_config;
        numeric_config.measurement_unit = static_cast<uint8_t>(Pm25ConcentrationMeasurement::MeasurementUnitEnum::kUgm3);
        pm25_concentration_measurement::feature::numeric_measurement::add(cluster, &numeric_config);
        
        log_i("Added PM2.5 Concentration Measurement cluster");
    } else {
        log_e("Failed to add PM2.5 Concentration Measurement cluster");
    }
}

void MatterAirQualitySensor::addPm10ConcentrationMeasurementCluster() {
    pm10_concentration_measurement::config_t cluster_config;
    cluster_config.measurement_medium = static_cast<uint8_t>(Pm10ConcentrationMeasurement::MeasurementMediumEnum::kAir);
    
    cluster_t* cluster = pm10_concentration_measurement::create(
        endpoint::get(node::get(), getEndPointId()), 
        &cluster_config, 
        CLUSTER_FLAG_SERVER
    );

    if (cluster) {
        pm10_concentration_measurement::feature::numeric_measurement::config_t numeric_config;
        numeric_config.measurement_unit = static_cast<uint8_t>(Pm10ConcentrationMeasurement::MeasurementUnitEnum::kUgm3);
        pm10_concentration_measurement::feature::numeric_measurement::add(cluster, &numeric_config);
        
        log_i("Added PM10 Concentration Measurement cluster");
    } else {
        log_e("Failed to add PM10 Concentration Measurement cluster");
    }
}

// bool MatterAirQualitySensor::setCO2(float co2_ppm) {
//     if (!started || !(enabledMeasurements & ENABLE_CO2)) {
//         log_e("CO2 measurement not enabled or sensor not started");
//         return false;
//     }

//     co2Value = co2_ppm;
    
//     esp_matter_attr_val_t val = esp_matter_invalid(NULL);
//     val.type = ESP_MATTER_VAL_TYPE_FLOAT;
//     val.val.f = co2_ppm;

//     if (!updateAttributeVal(CarbonDioxideConcentrationMeasurement::Id, 
//                            CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id, &val)) {
//         log_e("Failed to update CO2 attribute");
//         return false;
//     }

//     updateAirQualityEnum();
//     log_v("CO2 set to %.1f ppm", co2_ppm);
//     return true;
// }



bool MatterAirQualitySensor::setPM1(float pm1_ugm3) {
    if (!started || !(enabledMeasurements & ENABLE_PM1)) {
        log_e("PM1.0 measurement not enabled or sensor not started");
        return false;
    }

    pm1Value = pm1_ugm3;
    
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    val.type = ESP_MATTER_VAL_TYPE_FLOAT;
    val.val.f = pm1_ugm3;

    if (!updateAttributeVal(Pm1ConcentrationMeasurement::Id, 
                           Pm1ConcentrationMeasurement::Attributes::MeasuredValue::Id, &val)) {
        log_e("Failed to update PM1.0 attribute");
        return false;
    }

    log_v("PM1.0 set to %.1f µg/m³", pm1_ugm3);
    return true;
}

bool MatterAirQualitySensor::setPM25(float pm25_ugm3) {
    if (!started || !(enabledMeasurements & ENABLE_PM25)) {
        log_e("PM2.5 measurement not enabled or sensor not started");
        return false;
    }

    pm25Value = pm25_ugm3;
    
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    val.type = ESP_MATTER_VAL_TYPE_FLOAT;
    val.val.f = pm25_ugm3;

    if (!updateAttributeVal(Pm25ConcentrationMeasurement::Id, 
                           Pm25ConcentrationMeasurement::Attributes::MeasuredValue::Id, &val)) {
        log_e("Failed to update PM2.5 attribute");
        return false;
    }

    updateAirQualityEnum();
    log_v("PM2.5 set to %.1f µg/m³", pm25_ugm3);
    return true;
}

bool MatterAirQualitySensor::setPM10(float pm10_ugm3) {
    if (!started || !(enabledMeasurements & ENABLE_PM10)) {
        log_e("PM10 measurement not enabled or sensor not started");
        return false;
    }

    pm10Value = pm10_ugm3;
    
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    val.type = ESP_MATTER_VAL_TYPE_FLOAT;
    val.val.f = pm10_ugm3;

    if (!updateAttributeVal(Pm10ConcentrationMeasurement::Id, 
                           Pm10ConcentrationMeasurement::Attributes::MeasuredValue::Id, &val)) {
        log_e("Failed to update PM10 attribute");
        return false;
    }

    updateAirQualityEnum();
    log_v("PM10 set to %.1f µg/m³", pm10_ugm3);
    return true;
}

void MatterAirQualitySensor::updateAirQualityEnum() {
    // uint8_t co2Quality = classifyAirQualityByCO2(co2Value);
    uint8_t pm1Quality = classifyAirQualityByPM1(pm1Value);
    uint8_t pm25Quality = classifyAirQualityByPM25(pm25Value);
    uint8_t pm10Quality = classifyAirQualityByPM10(pm10Value);

    // airQualityEnum = co2Quality;
    if (pm1Quality > airQualityEnum) airQualityEnum = pm1Quality;
    if (pm25Quality > airQualityEnum) airQualityEnum = pm25Quality;
    if (pm10Quality > airQualityEnum) airQualityEnum = pm10Quality;

    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    val.type = ESP_MATTER_VAL_TYPE_UINT8;
    val.val.u8 = airQualityEnum;

    updateAttributeVal(AirQuality::Id, AirQuality::Attributes::AirQuality::Id, &val);
    log_d("Air Quality updated to: %d", airQualityEnum);
}

// uint8_t MatterAirQualitySensor::classifyAirQualityByCO2(float co2) {
//     if (co2 < 400) return 0;  
//     if (co2 <= 600) return 1;  // Good
//     if (co2 <= 800) return 2;  // Fair
//     if (co2 <= 1000) return 3; // Moderate
//     if (co2 <= 1500) return 4; // Poor
//     if (co2 <= 2000) return 5; // Very Poor
//     return 6;                   // Extremely Poor
// }

uint8_t MatterAirQualitySensor::classifyAirQualityByPM1(float pm1) {

    if (pm1 <= 12.0) return 1;   // Good
    if (pm1 <= 35.4) return 2;   // Fair
    if (pm1 <= 55.4) return 3;   // Moderate
    if (pm1 <= 150.4) return 4;  // Poor
    if (pm1 <= 250.4) return 5;  // Very Poor
    return 6;                      // Extremely Poor
}

uint8_t MatterAirQualitySensor::classifyAirQualityByPM25(float pm25) {
    if (pm25 <= 12.0) return 1;   // Good
    if (pm25 <= 35.4) return 2;   // Fair
    if (pm25 <= 55.4) return 3;   // Moderate
    if (pm25 <= 150.4) return 4;  // Poor
    if (pm25 <= 250.4) return 5;  // Very Poor
    return 6;                      // Extremely Poor
}

uint8_t MatterAirQualitySensor::classifyAirQualityByPM10(float pm10) {
    if (pm10 <= 54) return 1;     // Good
    if (pm10 <= 154) return 2;    // Fair
    if (pm10 <= 254) return 3;    // Moderate
    if (pm10 <= 354) return 4;    // Poor
    if (pm10 <= 424) return 5;    // Very Poor
    return 6;                      // Extremely Poor
}

bool MatterAirQualitySensor::attributeChangeCB(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val) {
    log_d("Attribute changed: endpoint=%u, cluster=%u, attribute=%u", endpoint_id, cluster_id, attribute_id);
    return true;
}

void MatterAirQualitySensor::end() {
    started = false;
}

#endif /* CONFIG_ESP_MATTER_ENABLE_DATA_MODEL */
