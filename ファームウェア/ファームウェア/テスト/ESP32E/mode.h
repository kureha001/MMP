#pragma once
#include <WiFi.h>
#include <Arduino.h>
#include <queue>
#include <mutex>

extern const char* SRV_IP;

namespace modeUART{
  extern bool IS_CONNECT;
  bool   BEGIN();
  bool   END();
  String RUN(const char* cmdStr, unsigned long argTimeoutMs);
}

namespace modeTCP{
  extern bool IS_CONNECT;
  bool   BEGIN(uint16_t argPort );
  bool   END();
  String RUN(const char* cmdStr, unsigned long argTimeoutMs);
}

namespace modeWebAPI{
  extern bool IS_CONNECT;
  bool   BEGIN(uint16_t argPort );
  bool   END();
  String RUN(const char* cmdStr);
}

namespace modeWebSoc{
  extern bool IS_CONNECT;
  bool   BEGIN(uint16_t argPort );
  bool   END();
  String RUN(const char* cmdStr, unsigned long argTimeoutMs);
}

namespace modeBLE{
  extern bool IS_CONNECT;
  bool   BEGIN();
  bool   END();
  String RUN(const char* cmdStr, unsigned long argTimeoutMs);
}

namespace modeIIC{
  extern bool IS_CONNECT;
  bool   BEGIN();
  bool   END();
  String RUN(const char* cmdStr);
}
