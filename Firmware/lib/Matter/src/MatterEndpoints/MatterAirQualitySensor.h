#pragma once
#include <sdkconfig.h>
#ifdef CONFIG_ESP_MATTER_ENABLE_DATA_MODEL

#include <Matter.h>
#include <MatterEndPoint.h>

class MatterAirQualitySensor : public MatterEndPoint {
public:
    MatterAirQualitySensor();
    ~MatterAirQualitySensor();

    // Bits: 0=CO2, 1=TVOC, 2=PM1.0, 3=PM2.5, 4=PM10
    bool begin(uint8_t enabledMeasurements = 0xFF);
    void end();

    // bool setCO2(float co2_ppm);           // CO2 in ppm
    // bool setTVOC(float tvoc_ppb);         // TVOC in ppb
    bool setPM1(float pm1_ugm3);          // PM1.0 in µg/m³
    bool setPM25(float pm25_ugm3);        // PM2.5 in µg/m³
    bool setPM10(float pm10_ugm3);        // PM10 in µg/m³

    // float getCO2() const { return co2Value; }
    // float getTVOC() const { return tvocValue; }
    float getPM1() const { return pm1Value; }
    float getPM25() const { return pm25Value; }
    float getPM10() const { return pm10Value; }

    uint8_t getAirQuality() const { return airQualityEnum; }

    bool attributeChangeCB(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val) override;

    // static constexpr uint8_t ENABLE_CO2   = 0x01;
    // static constexpr uint8_t ENABLE_TVOC  = 0x02;
    static constexpr uint8_t ENABLE_PM1   = 0x04;
    static constexpr uint8_t ENABLE_PM25  = 0x08;
    static constexpr uint8_t ENABLE_PM10  = 0x10;

protected:
    bool started = false;
    uint8_t enabledMeasurements = 0;
    
    // float co2Value = 400.0f;      
    // float tvocValue = 0.0f;
    float pm1Value = 0.0f;
    float pm25Value = 0.0f;
    float pm10Value = 0.0f;
    uint8_t airQualityEnum = 0; 

private:

    // void addCarbonDioxideConcentrationMeasurementCluster();
    // void addTotalVolatileOrganicCompoundsConcentrationMeasurementCluster();
    void addPm1ConcentrationMeasurementCluster();
    void addPm25ConcentrationMeasurementCluster();
    void addPm10ConcentrationMeasurementCluster();
    
    void updateAirQualityEnum();
    // uint8_t classifyAirQualityByCO2(float co2);
    uint8_t classifyAirQualityByPM1(float pm1);
    uint8_t classifyAirQualityByPM25(float pm25);
    uint8_t classifyAirQualityByPM10(float pm10);
};

#endif /* CONFIG_ESP_MATTER_ENABLE_DATA_MODEL */
