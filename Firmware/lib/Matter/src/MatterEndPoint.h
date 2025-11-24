#pragma once
#include <sdkconfig.h>
#ifdef CONFIG_ESP_MATTER_ENABLE_DATA_MODEL

#include <Matter.h>
#include <functional>

using namespace esp_matter;

class MatterEndPoint {
public:
  enum attrOperation_t {
    ATTR_SET = false,
    ATTR_UPDATE = true
  };

  uint16_t getEndPointId() {
    return endpoint_id;
  }

  void setEndPointId(uint16_t ep) {
    endpoint_id = ep;
  }

  esp_matter::attribute_t *getAttribute(uint32_t cluster_id, uint32_t attribute_id) {
    if (endpoint_id == 0) {
      log_e("Endpoint ID is not set");
      return NULL;
    }
    endpoint_t *endpoint = endpoint::get(node::get(), endpoint_id);
    if (endpoint == NULL) {
      log_e("Endpoint [%d] not found", endpoint_id);
      return NULL;
    }
    cluster_t *cluster = cluster::get(endpoint, cluster_id);
    if (cluster == NULL) {
      log_e("Cluster [%d] not found", cluster_id);
      return NULL;
    }
    esp_matter::attribute_t *attribute = attribute::get(cluster, attribute_id);
    if (attribute == NULL) {
      log_e("Attribute [%d] not found", attribute_id);
      return NULL;
    }
    return attribute;
  }

  bool getAttributeVal(uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *attrVal) {
    esp_matter::attribute_t *attribute = getAttribute(cluster_id, attribute_id);
    if (attribute == NULL) {
      return false;
    }
    if (attribute::get_val(attribute, attrVal) == ESP_OK) {
      log_v("GET_VAL Success for cluster %d, attribute %d with value %d", cluster_id, attribute_id, attrVal->val.u32);
      return true;
    }
    log_e("GET_VAL FAILED! for cluster %d, attribute %d with value %d", cluster_id, attribute_id, attrVal->val.u32);
    return false;
  }

  bool setAttributeVal(uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *attrVal) {
    esp_matter::attribute_t *attribute = getAttribute(cluster_id, attribute_id);
    if (attribute == NULL) {
      return false;
    }
    if (attribute::set_val(attribute, attrVal) == ESP_OK) {
      log_v("SET_VAL Success for cluster %d, attribute %d with value %d", cluster_id, attribute_id, attrVal->val.u32);
      return true;
    }
    log_e("SET_VAL FAILED! for cluster %d, attribute %d with value %d", cluster_id, attribute_id, attrVal->val.u32);
    return false;
  }

  bool updateAttributeVal(uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *attrVal) {
    if (attribute::update(endpoint_id, cluster_id, attribute_id, attrVal) == ESP_OK) {
      log_v("Update Success for cluster %d, attribute %d with value %d", cluster_id, attribute_id, attrVal->val.u32);
      return true;
    }
    log_e("Update FAILED! for cluster %d, attribute %d with value %d", cluster_id, attribute_id, attrVal->val.u32);
    return false;
  }

  virtual bool attributeChangeCB(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val) = 0;


  bool endpointIdentifyCB(uint16_t endpoint_id, bool identifyIsEnabled) {
    if (_onEndPointIdentifyCB) {
      return _onEndPointIdentifyCB(identifyIsEnabled);
    }
    return true;
  }
  using EndPointIdentifyCB = std::function<bool(bool)>;
  void onIdentify(EndPointIdentifyCB onEndPointIdentifyCB) {
    _onEndPointIdentifyCB = onEndPointIdentifyCB;
  }

protected:
  uint16_t endpoint_id = 0;
  EndPointIdentifyCB _onEndPointIdentifyCB = NULL;
};
#endif /* CONFIG_ESP_MATTER_ENABLE_DATA_MODEL */
