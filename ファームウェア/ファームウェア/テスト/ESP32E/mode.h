#pragma once
#include <WiFi.h>
#include <Arduino.h>
#include <queue>
#include <mutex>


namespace modeTCP   {
  extern const char*    SRV_IP;
  extern const uint16_t SRV_PORT;
  extern bool           ENABLED;
  bool   INIT();
  String RUN(const char* cmdStr);
  void   DISCONNECT();
}

namespace modeUART  { extern bool ENABLED; bool INIT(); String RUN(const char* cmdStr); }
namespace modeWebAPI{ extern bool ENABLED; bool INIT(); String RUN(const char* cmdStr); }
namespace modeWebSoc{ extern bool ENABLED; bool INIT(); String RUN(const char* cmdStr); }
namespace modeBLE   { extern bool ENABLED; bool INIT(); String RUN(const char* cmdStr); }
namespace modeIIC   { extern bool ENABLED; bool INIT(); String RUN(const char* cmdStr); }