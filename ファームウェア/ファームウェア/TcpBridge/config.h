// filename : config.h
//============================================================
// 設定関連
// バージョン：0.5
//============================================================
#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>

//━━━━━━━━━━━━━━━
// Wi-Fi の接続候補
//━━━━━━━━━━━━━━━
struct WifiCand { String ssid, pass; uint32_t timeout_ms; };

//━━━━━━━━━━━━━━━
// AP フォールバックの設定
//━━━━━━━━━━━━━━━
struct WifiAP   { bool enabled; String ssid, pass; uint32_t hold_seconds; };

//━━━━━━━━━━━━━━━
// Wi-Fi 設定全体
//━━━━━━━━━━━━━━━
struct WifiCfg {
  String hostname;
  WifiCand cand[6];
  int candN = 0;
  WifiAP apfb { true, "m5-bridge-setup", "", 0 };
};

//━━━━━━━━━━━━━━━
// UART 設定全体
//━━━━━━━━━━━━━━━
struct UartCfg { HardwareSerial* port; uint8_t rx, tx; uint32_t baud; uint16_t tcpPort; };

//━━━━━━━━━━━━━━━
// ブリッジサーバ 設定全体
//━━━━━━━━━━━━━━━
struct ServerCfg { int maxClients; bool writeLock; uint32_t writeLockMs; };

//━━━━━━━━━━━━━━━
// グローバル（config.cppで定義
//━━━━━━━━━━━━━━━
extern WifiCfg  WIFI;
extern UartCfg  UARTS[2];
extern int      MMP_NUM_UARTS;
extern ServerCfg SRV;

//━━━━━━━━━━━━━━━
// 設定読込み
//━━━━━━━━━━━━━━━
bool loadConfig();

//━━━━━━━━━━━━━━━
// ホスト名と UART 設定を適用する
// WiFi.setHostname()
// Serial1/2 の begin()（GPIO, ボーレート）
//━━━━━━━━━━━━━━━
void applyUartConfig();