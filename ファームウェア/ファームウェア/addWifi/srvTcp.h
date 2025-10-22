// srvTcp.h
// サーバー：TCPブリッジ（UART廃止・本体直結）

#pragma once
#include <Arduino.h>
#include <WiFi.h>

// srvTcp.h
namespace srvTcp {
  bool        begin(uint16_t port, int maxClients);
  void        handle();
  inline bool start(uint16_t port, int maxClients) { return begin(port, maxClients); }
  int         clientCount();
  void        closeAll();
}
