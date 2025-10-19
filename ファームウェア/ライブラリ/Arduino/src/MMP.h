// filename : MMP.h
//============================================================
// アプリ接続
// バージョン：0.5
//============================================================
#pragma once
#include <Arduino.h>
#include "MmpClient.h"

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
namespace MMP {

//─────────────────────
// API: 終了
//─────────────────────
void 終了(Mmp::Core::MmpClient& cli, bool wifiOff = true, Stream* log = &Serial);

//─────────────────────
// API: 接続（Wi-Fi）
//─────────────────────
bool 通信接続_WiFi(const char*    ssid, const char*   pass, Stream* log = &Serial);
bool 通信接続_WiFi(const String&  ssid, const String& pass, Stream* log = &Serial);

//─────────────────────
// API: 接続（Wi-Fi 外部ファイル読込）
//─────────────────────
bool 通信接続_WiFi_外部(const char* path = nullptr, Stream* log = &Serial);

//─────────────────────
// API: 接続（TCPブリッジ）
//─────────────────────
bool 通信接続_Tcp(const char*   conn, Mmp::Core::MmpClient& cli, Stream* log = &Serial);
bool 通信接続_Tcp(const String& conn, Mmp::Core::MmpClient& cli, Stream* log = &Serial);

//─────────────────────
// API: 接続（統一入口）
//─────────────────────
bool 通信接続(
  const char*           conn                ,
  Mmp::Core::MmpClient& cli                 ,
  const char*           wifiSsid  = nullptr ,
  const char*           wifiPass  = nullptr ,
  Stream*               log       = &Serial );
//─────────────────────
bool 通信接続(
  const String&         conn                ,
  Mmp::Core::MmpClient& cli                 ,
  const char*           wifiSsid  = nullptr ,
  const char*           wifiPass  = nullptr ,
  Stream*               log       = &Serial );

} // namespace MMP
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
