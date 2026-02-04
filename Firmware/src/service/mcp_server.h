#pragma once

namespace SVC {

class MCPServer {
public:
    static bool init();
    static bool start();
    static bool startTask();
    static void loop();
};

}
