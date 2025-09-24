// config.cpp
#include "config.hpp"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>

// ===================== 設定関連 実装 =====================

// ---- 既定値（最小動作用） ----
WifiCfg  WIFI; // hostname は JSON から設定
UartCfg  UARTS[2] = {
  { &Serial1, 3, 1, 115200, 5331 },   // UART1: RX=G3,  TX=G1
  { &Serial2, 10, 9, 115200, 5332 }   // UART2: RX=G10, TX=G9
};
int      NUM_UARTS = 2;
ServerCfg SRV      = { 4, false, 30000 };

/**
 * @brief /config.json を読み込む（無ければ最小構成で自動生成）
 * @details
 *  - 初回は hostname=m5-bridge・UART/Serverのみを書いたJSONを自動生成
 *  - wifi.candidates は空 → すべて失敗時は AP フォールバックで設定投入
 */
bool loadConfig() {
  if (!LittleFS.begin(true)) return false;

  // ---- 初回：/config.json が無い場合は自動生成 ----
  if (!LittleFS.exists("/config.json")) {
    StaticJsonDocument<512> doc;
    doc["wifi"]["hostname"] = "m5-bridge"; // ★ 初期ホスト名（必要ならここを変更）
    // candidates は空のまま（APフォールバックで設定投入）

    // UART 既定値
    JsonArray ua = doc.createNestedArray("uart");
    for (int i=0;i<NUM_UARTS;i++){
      JsonObject u = ua.createNestedObject();
      u["rx"]=UARTS[i].rx; u["tx"]=UARTS[i].tx; u["baud"]=UARTS[i].baud; u["tcp_port"]=UARTS[i].tcpPort;
    }
    // Server 既定値
    doc["server"]["max_clients"] = SRV.maxClients;
    doc["server"]["write_lock"]  = SRV.writeLock;
    doc["server"]["write_lock_ms"] = SRV.writeLockMs;

    File nf = LittleFS.open("/config.json","w");
    if (!nf) return false;
    serializeJsonPretty(doc,nf); nf.close();
  }

  // ---- 読み込み ----
  File f = LittleFS.open("/config.json","r"); if (!f) return false;
  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc,f);
  f.close();
  if (err) return false;

  // ホスト名
  WIFI.hostname = doc["wifi"]["hostname"] | "m5-bridge";

  // Wi-Fi候補（順序で優先度）
  WIFI.candN = 0;
  if (doc["wifi"]["candidates"].is<JsonArray>()){
    for (JsonObject c : doc["wifi"]["candidates"].as<JsonArray>()){
      if (WIFI.candN >= 6) break;                   // ★ 上限保護
      WIFI.cand[WIFI.candN].ssid = String((const char*)(c["ssid"] | ""));
      WIFI.cand[WIFI.candN].pass = String((const char*)(c["pass"] | ""));
      WIFI.cand[WIFI.candN].timeout_ms = (uint32_t)(c["timeout_ms"] | 8000);
      if (WIFI.cand[WIFI.candN].ssid.length()) WIFI.candN++;  // ★ 空は無視
    }
  } else {
    // 後方互換：旧形式（ssid/pass の単一指定）
    String ssid = doc["wifi"]["ssid"] | "";
    String pass = doc["wifi"]["pass"] | "";
    if (ssid.length()){
      WIFI.cand[0] = { ssid, pass, 8000 };
      WIFI.candN = 1;
    }
  }

  // APフォールバック
  WIFI.apfb.enabled = doc["wifi"]["ap_fallback"]["enabled"] | true;
  WIFI.apfb.ssid    = doc["wifi"]["ap_fallback"]["ssid"]    | "m5-bridge-setup";
  WIFI.apfb.pass    = doc["wifi"]["ap_fallback"]["pass"]    | "";
  WIFI.apfb.hold_seconds = (uint32_t)(doc["wifi"]["ap_fallback"]["hold_seconds"] | 0);

  // UART
  if (doc["uart"].is<JsonArray>()){
    int i=0;
    for (JsonObject u : doc["uart"].as<JsonArray>()){
      if (i>=NUM_UARTS) break; // ★ 範囲保護
      UARTS[i].rx = u["rx"] | UARTS[i].rx;
      UARTS[i].tx = u["tx"] | UARTS[i].tx;
      UARTS[i].baud = u["baud"] | UARTS[i].baud;
      UARTS[i].tcpPort = u["tcp_port"] | UARTS[i].tcpPort;
      i++;
    }
  }

  // Server
  SRV.maxClients  = doc["server"]["max_clients"]   | SRV.maxClients;
  SRV.writeLock   = doc["server"]["write_lock"]    | SRV.writeLock;
  SRV.writeLockMs = doc["server"]["write_lock_ms"] | SRV.writeLockMs;
  return true;
}

/**
 * @brief ホスト名と UART 設定を適用する
 * - WiFi.setHostname()
 * - 各 UARTポートの begin()
 */
void applyUartConfig(){
  if(WIFI.hostname.length()) {
    WiFi.setHostname(WIFI.hostname.c_str());
  }
  for (int i=0;i<NUM_UARTS;i++){
    UARTS[i].port->end();
    UARTS[i].port->begin(UARTS[i].baud, SERIAL_8N1, UARTS[i].rx, UARTS[i].tx);
  }
}
