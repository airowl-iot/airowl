#pragma once

#ifdef __cplusplus
#include <Arduino.h>

namespace SVC {
namespace Matter {

    bool init();
    bool startTask();
    bool restartTask();

    bool isCommissioned();
    String getManualPairingCode();
    String getQRCodeData();

    void task(void* parameter);

} // namespace Matter
} // namespace SVC
#endif  // __cplusplus


#ifdef __cplusplus
extern "C" {
#endif

bool is_matter_commissioned();
const char* get_matter_pairing_code();
const char* get_matter_qrcode_data();

#ifdef __cplusplus
}
#endif
