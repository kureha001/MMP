// filename : config.hpp
//============================================================
// 設定関連
// バージョン：0.5
//------------------------------------------------------------
// ボード依存の薄い定義（MmpClient.* が参照）
// できるだけ最小に保ち、各プロジェクトで必要なら上書き可
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
  WifiAP apfb { true, "wifi-setup", "", 0 };
};

//━━━━━━━━━━━━━━━
// グローバル（config.cppで定義
//━━━━━━━━━━━━━━━
extern WifiCfg  WIFI;

//━━━━━━━━━━━━━━━
// 設定読込み
//━━━━━━━━━━━━━━━
bool loadConfig();