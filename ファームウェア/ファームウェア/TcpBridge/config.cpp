// filename : config.cpp
//============================================================
// 設定関連
// バージョン：0.5
//============================================================
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>

WifiCfg  WIFI; // hostname は JSON から設定

#if defined(ARDUINO_ARCH_RP2040)
  UartCfg  UARTS[2] = {
    { &Serial1, 1, 0, 921600, 5331 },  // UART1: RX=G1, TX=G0
    { &Serial2, 5, 4, 921600, 5332 }   // UART2: RX=G5, TX=G4
  };
  int MMP_NUM_UARTS = 2;
#else
  UartCfg  UARTS[2] = {
    { &Serial1, 3,  1, 921600, 5331 },  // UART1: RX=G3,  TX=G1
    { &Serial2, 10, 9, 921600, 5332 }   // UART2: RX=G10, TX=G9
  };
  int MMP_NUM_UARTS = 2;
#endif

ServerCfg SRV     = { 4, false, 30000 };

//━━━━━━━━━━━━━
// 設定読込み
//━━━━━━━━━━━━━
bool loadConfig() {

  // ★RP2040/Earle core は begin() に引数を取らない
  #if defined(ARDUINO_ARCH_RP2040)
    if (!LittleFS.begin())     return false;
  #else
    if (!LittleFS.begin(true)) return false;
  #endif

  // (初回) -> /config.json が無い場合は自動生成
  if (!LittleFS.exists("/config.json")) {

    JsonDocument doc;

    // HOST名
    doc["wifi"]["hostname"] = "mmp-bridge";

    // UART既定
    JsonArray ua = doc["uart"].to<JsonArray>();
    for (int i=0;i<MMP_NUM_UARTS;i++){
      JsonObject u = ua.add<JsonObject>();
      u["rx"      ] = UARTS[i].rx;      // GPIO Rx
      u["tx"      ] = UARTS[i].tx;      // GPIO Tx
      u["baud"    ] = UARTS[i].baud;    // ボーレート
      u["tcp_port"] = UARTS[i].tcpPort; // ポート番号
    }
    // Server既定
    doc["server"]["max_clients"  ] = SRV.maxClients;  // 同時接続数
    doc["server"]["write_lock"   ] = SRV.writeLock;   // 書込ロック有無
    doc["server"]["write_lock_ms"] = SRV.writeLockMs; // 書込ロック時間

    // jsonファイルを保存
    File nf = LittleFS.open("/config.json","w");
    if (!nf) return false;
    serializeJsonPretty(doc,nf);
    nf.close();
  }

  // jsonファイルを読み込み
  File f = LittleFS.open("/config.json", "r");
  if (!f) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {return false;}

  // ホスト名
  WIFI.hostname = doc["wifi"]["hostname"] | "mmp-bridge";

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
  WIFI.apfb.ssid    = doc["wifi"]["ap_fallback"]["ssid"]    | "mmp-bridge-ap";
  WIFI.apfb.pass    = doc["wifi"]["ap_fallback"]["pass"]    | "";
  WIFI.apfb.hold_seconds = (uint32_t)(doc["wifi"]["ap_fallback"]["hold_seconds"] | 0);

  // UART
  if (doc["uart"].is<JsonArray>()){
    int i=0;
    for (JsonObject u : doc["uart"].as<JsonArray>()){
      if (i>=MMP_NUM_UARTS) break; // ★ 範囲保護
      UARTS[i].rx       = u["rx"      ] | UARTS[i].rx;
      UARTS[i].tx       = u["tx"      ] | UARTS[i].tx;
      UARTS[i].baud     = u["baud"    ] | UARTS[i].baud;
      UARTS[i].tcpPort  = u["tcp_port"] | UARTS[i].tcpPort;
      i++;
    }
  }

  // Server
  SRV.maxClients  = doc["server"]["max_clients"   ] | SRV.maxClients;
  SRV.writeLock   = doc["server"]["write_lock"    ] | SRV.writeLock;
  SRV.writeLockMs = doc["server"]["write_lock_ms" ] | SRV.writeLockMs;

  // 読み込み成功
  return true;
}

/**
 * @brief ホスト名と UART 設定を適用する
 * - WiFi.setHostname()
 * - 各 UARTポートの begin()
 */
void applyUartConfig(){

  if(WIFI.hostname.length()){WiFi.setHostname(WIFI.hostname.c_str());}

  for (int i=0;i<MMP_NUM_UARTS;i++){

    // USB CDC (Serial) を対象にしない
    if (UARTS[i].port == &Serial) continue;

    UARTS[i].port->end();

    #if defined(ARDUINO_ARCH_RP2040)
      if (i==0){
        Serial1.setRX(UARTS[i].rx);
        Serial1.setTX(UARTS[i].tx);
      }
      if (i==1){
        Serial2.setRX(UARTS[i].rx);
        Serial2.setTX(UARTS[i].tx);
      }
      UARTS[i].port->begin(UARTS[i].baud);
    #else
      UARTS[i].port->begin(UARTS[i].baud, SERIAL_8N1, UARTS[i].rx, UARTS[i].tx);
    #endif
  }
}