#pragma once
#include <WiFi.h>
#include <Arduino.h>
#include <queue>
#include <mutex>

extern const char* SRV_IP;

namespace modeUART  {bool BEGIN(); bool END(); String RUN(const char* cmdStr);}
namespace modeTCP   {bool BEGIN(); bool END(); String RUN(const char* cmdStr);}
namespace modeWebAPI{bool BEGIN(); bool END(); String RUN(const char* cmdStr);}
namespace modeWebSoc{bool BEGIN(); bool END(); String RUN(const char* cmdStr);}
namespace modeBLE   {bool BEGIN(); bool END(); String RUN(const char* cmdStr);}
namespace modeIIC   {bool BEGIN(); bool END(); String RUN(const char* cmdStr);}
