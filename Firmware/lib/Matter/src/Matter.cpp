
#include <sdkconfig.h>
#ifdef CONFIG_ESP_MATTER_ENABLE_DATA_MODEL

#include <Matter.h>
#include <app/server/Server.h>
#include <esp_mac.h>

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace esp_matter::identification;
using namespace chip::app::Clusters;

constexpr auto k_timeout_seconds = 300;

static bool _matter_has_started = false;
static node::config_t node_config;
static node_t *deviceNode = NULL;


static esp_err_t app_attribute_update_cb(
  attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data
) {
  log_d("Attribute update callback: type: %u, endpoint: %u, cluster: %u, attribute: %u, val: %u", type, endpoint_id, cluster_id, attribute_id, val->val.u32);
  esp_err_t err = ESP_OK;
  MatterEndPoint *ep = (MatterEndPoint *)priv_data;  
  switch (type) {
    case PRE_UPDATE:
      log_v("Attribute update callback: PRE_UPDATE");
      if (ep != NULL) {
        err = ep->attributeChangeCB(endpoint_id, cluster_id, attribute_id, val) ? ESP_OK : ESP_FAIL;
      }
      break;
    case POST_UPDATE:  
      log_v("Attribute update callback: POST_UPDATE");
      break;
    case READ:  
      log_v("Attribute update callback: READ");
      break;
    case WRITE:
      log_v("Attribute update callback: WRITE");
      break;
    default: log_v("Attribute update callback: Unknown type %d", type);
  }
  return err;
}

static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id, uint8_t effect_variant, void *priv_data) {
  log_d("Identification callback to endpoint %d: type: %u, effect: %u, variant: %u", endpoint_id, type, effect_id, effect_variant);
  esp_err_t err = ESP_OK;
  MatterEndPoint *ep = (MatterEndPoint *)priv_data;  
  bool identifyIsActive = false;

  if (type == identification::callback_type_t::START) {
    log_v("Identification callback: START");
    identifyIsActive = true;
  } else if (type == identification::callback_type_t::EFFECT) {
    log_v("Identification callback: EFFECT");
  } else if (type == identification::callback_type_t::STOP) {
    identifyIsActive = false;
    log_v("Identification callback: STOP");
  }
  if (ep != NULL) {
    err = ep->endpointIdentifyCB(endpoint_id, identifyIsActive) ? ESP_OK : ESP_FAIL;
  }

  return err;
}

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg) {
  switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
      log_i(
        "Interface %s Address changed", event->InterfaceIpAddressChanged.Type == chip::DeviceLayer::InterfaceIpChangeType::kIpV4_Assigned ? "IPv4" : "IPV6"
      );
      break;
    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:       log_i("Commissioning complete"); break;
    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:        log_i("Commissioning failed, fail safe timer expired"); break;
    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted: log_i("Commissioning session started"); break;
    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped: log_i("Commissioning session stopped"); break;
    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:   log_i("Commissioning window opened"); break;
    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:   log_i("Commissioning window closed"); break;
    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
    {
      log_i("Fabric removed successfully");
      if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0) {
        log_i("No fabric left, opening commissioning window");
        chip::CommissioningWindowManager &commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
        constexpr auto kTimeoutSeconds = chip::System::Clock::Seconds16(k_timeout_seconds);
        if (!commissionMgr.IsCommissioningWindowOpen()) {
          CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(kTimeoutSeconds, chip::CommissioningWindowAdvertisement::kDnssdOnly);
          if (err != CHIP_NO_ERROR) {
            log_e("Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
          }
        }
      }
      break;
    }
    case chip::DeviceLayer::DeviceEventType::kFabricWillBeRemoved: log_i("Fabric will be removed"); break;
    case chip::DeviceLayer::DeviceEventType::kFabricUpdated:       log_i("Fabric is updated"); break;
    case chip::DeviceLayer::DeviceEventType::kFabricCommitted:     log_i("Fabric is committed"); break;
    case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:    log_i("BLE deinitialized and memory reclaimed"); break;
    default:                                                       break;
  }
}

void ArduinoMatter::_init() {
  if (_matter_has_started) {
    return;
  }

  deviceNode = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
  if (deviceNode == nullptr) {
    log_e("Failed to create Matter node");
    return;
  }

  _matter_has_started = true;
}

void ArduinoMatter::begin() {
  if (!_matter_has_started) {
    log_e("No Matter endpoint has been created. Please create an endpoint first.");
    return;
  }

  esp_err_t err = esp_matter::start(app_event_cb);
  if (err != ESP_OK) {
    log_e("Failed to start Matter, err:%d", err);
    _matter_has_started = false;
  }
}

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
bool ArduinoMatter::isThreadConnected() {
  return false;  // Thread Network TBD
}
#endif

bool ArduinoMatter::isDeviceCommissioned() {
  return chip::Server::GetInstance().GetFabricTable().FabricCount() > 0;
}

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI_STATION
bool ArduinoMatter::isWiFiConnected() {
  return chip::DeviceLayer::ConnectivityMgr().IsWiFiStationConnected();
}
#endif

bool ArduinoMatter::isDeviceConnected() {
  bool retCode = false;
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
  retCode |= ArduinoMatter::isThreadConnected();
#endif
#if CHIP_DEVICE_CONFIG_ENABLE_WIFI_STATION
  retCode |= ArduinoMatter::isWiFiConnected();
#endif
  return retCode;
}

void ArduinoMatter::decommission() {
  esp_matter::factory_reset();
}

// Global Matter Object
ArduinoMatter Matter;

#endif /* CONFIG_ESP_MATTER_ENABLE_DATA_MODEL */