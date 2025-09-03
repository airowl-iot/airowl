// sensor_manager.h - Sensor Manager for Airowl 3.0
#pragma once

#include <Arduino.h>
#include <vector>
#include <functional>
#include "../core/event_bus.h"
#include "../hal/hal_pms.h"
#include "../hal/hal_pm700.h"
#include "../hal/hal_aht.h"
#include "../hal/hal_ens160.h"

namespace APP {

class SensorManager {
public:
    /**
     * @brief Sensor reading interval configuration
     */
    struct SensorConfig {
        bool enabled;                  
        unsigned long readInterval;   
        unsigned long publishInterval;  
    };
    
    /**
     * @brief Initialize the sensor manager
     * @param pmsConfig PMS sensor configuration
     * @param pm700Config PM700 sensor configuration
     * @param ahtConfig AHT sensor configuration
     * @param ens160Config ENS160 sensor configuration
     * @return True if initialization was successful
     */
    static bool init(const SensorConfig& pmsConfig, const SensorConfig& pm700Config, const SensorConfig& ahtConfig, const SensorConfig& ens160Config);
    
    /**
     * @brief Start sensor readings
     * @return True if started successfully
     */
    static bool start();
    
    /**
     * @brief Stop sensor readings
     */
    static void stop();
    
    /**
     * @brief Check if sensor manager is running
     * @return True if running
     */
    static bool isRunning();
    
    /**
     * @brief Update sensor configuration
     * @param pmsConfig PMS sensor configuration
     * @param pm700Config PM700 sensor configuration
     * @param ahtConfig AHT sensor configuration
     * @param ens160Config ENS160 sensor configuration
     */
    static void updateConfig(const SensorConfig& pmsConfig, const SensorConfig& pm700Config, const SensorConfig& ahtConfig, const SensorConfig& ens160Config);
    
    /**
     * @brief Get current PMS sensor configuration
     * @return PMS sensor configuration
     */
    static const SensorConfig& getPMSConfig();
    
    /**
     * @brief Get current PM700 sensor configuration
     * @return PM700 sensor configuration
     */
    static const SensorConfig& getPM700Config();
    
    /**
     * @brief Get current AHT sensor configuration
     * @return AHT sensor configuration
     */
    static const SensorConfig& getAHTConfig();
    
    /**
     * @brief Get current ENS160 sensor configuration
     * @return ENS160 sensor configuration
     */
    static const SensorConfig& getENS160Config();
    
    /**
     * @brief Sensor manager task function
     * @param parameter Task parameters (unused)
     */
    static void task(void* parameter);
    
    /**
     * @brief Start the sensor manager task
     * @return True if task was started successfully
     */
    static bool startTask();
    
    /**
     * @brief Restart the sensor manager task
     * @return True if task was restarted successfully
     */
    static bool restartTask();

private:
    // Private implementation details
    
    /**
     * @brief Read PMS sensor data
     */
    static void readPMSSensor();
    
    /**
     * @brief Read PM700 sensor data
     */
    static void readPM700Sensor();
    
    /**
     * @brief Read AHT sensor data
     */
    static void readAHTSensor();
    
    /**
     * @brief Read ENS160 sensor data
     */
    static void readENS160Sensor();
    
    /**
     * @brief Publish PMS sensor data
     * @param data PMS sensor data
     */
    static void publishPMSData(const HAL::PMS::Data& data);
    
    /**
     * @brief Publish PM700 sensor data
     * @param data PM700 sensor data
     */
    static void publishPM700Data(const HAL::PM700::Data& data);
    
    /**
     * @brief Publish AHT sensor data
     * @param data AHT sensor data
     */
    static void publishAHTData(const HAL::AHT::Data& data);
    
    /**
     * @brief Publish ENS160 sensor data
     * @param data ENS160 sensor data
     */
    static void publishENS160Data(const HAL::ENS160::Data& data);
};

} // namespace APP