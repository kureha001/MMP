// filename : MMP.h
//============================================================
// アプリ接続
// バージョン：0.5
//------------------------------------------------------------
// M5Stamp S3 : 2x UART
// ボード設定：Flash Size 4MB(32Mb)
//------------------------------------------------------------
// ラズパイPico: 2x UART
// ボード設定：Flash Size 4MB(FS:64KB)
//============================================================
#pragma once
#include <Arduino.h>
#include "MmpClient.h"

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
namespace MMP {

//─────────────────────
// API: ユーティリティ
//─────────────────────
void クライアント切断(Mmp::Core::MmpClient& cli, bool wifiOff = true, Stream* log = &Serial);
void 状況表示(Mmp::Core::MmpClient& cli, Stream* log = &Serial);

//─────────────────────
// API: 接続（Wi-Fi）
//─────────────────────
bool 通信接続_WiFi(const char*    ssid, const char*   pass, Stream* log = &Serial);
bool 通信接続_WiFi(const String&  ssid, const String& pass, Stream* log = &Serial);

//─────────────────────
// API: 接続（Wi-Fi 外部ファイル読込）
//─────────────────────
bool 通信接続_WiFi_外部(Stream* log = &Serial);

//─────────────────────
// API: 接続（TCPブリッジ）
//─────────────────────
bool 通信接続_Tcp(const char*   conn, Mmp::Core::MmpClient& cli, Stream* log = &Serial);
bool 通信接続_Tcp(const String& conn, Mmp::Core::MmpClient& cli, Stream* log = &Serial);

//─────────────────────
// API: 接続（統一入口）
//─────────────────────
bool 通信接続(
  const char*           conn            ,
  Mmp::Core::MmpClient& cli             ,
  const char*           ssid  = nullptr ,
  const char*           pass  = nullptr ,
  Stream*               log   = &Serial );
//─────────────────────
bool 通信接続(
  const String&         conn            ,
  Mmp::Core::MmpClient& cli             ,
  const char*           ssid  = nullptr ,
  const char*           pass  = nullptr ,
  Stream*               log   = &Serial );

} // namespace MMP
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
