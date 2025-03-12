#include <Matter.h>

extern "C" {
    bool is_matter_commissioned() {
        return ArduinoMatter::isDeviceCommissioned();
    }
}