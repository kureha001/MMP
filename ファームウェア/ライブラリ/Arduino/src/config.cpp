// filename : config.cpp
//============================================================
// 設定関連
// バージョン：0.5
//------------------------------------------------------------
// ボード依存の薄い定義（MmpClient.* が参照）
// できるだけ最小に保ち、各プロジェクトで必要なら上書き可
//============================================================
#include "config.hpp"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>

WifiCfg  WIFI; // hostname は JSON から設定

//-----------------------
// 設定読込み
//-----------------------
bool loadConfig() {

  // ★RP2040/Earle core は begin() に引数を取らない
  #if defined(ARDUINO_ARCH_RP2040)
    if (!LittleFS.begin()) return false;
  #else
    if (!LittleFS.begin(true)) return false;
  #endif

  // (初回) -> /config.json が無い場合は自動生成
  if (!LittleFS.exists("/config.json")) {
//    StaticJsonDocument<512> doc;
    JsonDocument doc;
    doc["wifi"]["hostname"] = "wifi_sta"; // ★ 初期ホスト名

    // candidates は空のまま（APフォールバックで設定投入）
    File nf = LittleFS.open("/config.json","w");
    if (!nf) return false;                      // 開けなかった
    serializeJsonPretty(doc,nf); nf.close();    // 書き込み完了
  }

  // 読み込み
  File f = LittleFS.open("/config.json", "r"); if (!f) return false;
  size_t fs = f.size();
  size_t cap = fs + 1024;         // ファイルサイズ + 余裕
  DynamicJsonDocument doc(cap);   // ← 静的 2048 から変更
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    // 失敗時の一時ログ（必要なら）: Serial.println(String("JSON err: ") + err.c_str());
    return false;
  }

  // ホスト名
  WIFI.hostname = doc["wifi"]["hostname"] | "wifi-sta";

  // Wi-Fi候補（順序で優先度）
  WIFI.candN = 0;
  for (JsonObject c : doc["wifi"]["candidates"].as<JsonArray>()){
    if (WIFI.candN >= 6) break;                   // ★ 上限保護
    WIFI.cand[WIFI.candN].ssid = String((const char*)(c["ssid"] | ""));
    WIFI.cand[WIFI.candN].pass = String((const char*)(c["pass"] | ""));
    WIFI.cand[WIFI.candN].timeout_ms = (uint32_t)(c["timeout_ms"] | 8000);
    if (WIFI.cand[WIFI.candN].ssid.length()) WIFI.candN++;  // ★ 空は無視
  }

  // APフォールバック
  WIFI.apfb.enabled = doc["wifi"]["ap_fallback"]["enabled"] | true;
  WIFI.apfb.ssid    = doc["wifi"]["ap_fallback"]["ssid"]    | "wifi-ap";
  WIFI.apfb.pass    = doc["wifi"]["ap_fallback"]["pass"]    | "";
  WIFI.apfb.hold_seconds = (uint32_t)(doc["wifi"]["ap_fallback"]["hold_seconds"] | 0);

  // 読み込み成功
  return true;
}