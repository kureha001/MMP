// filename : config.cpp
// 設定ファイル(新規作成/読込)

#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"

typeConnect g_WIFI;                       // 規定値はここでは持たない
typeServer  g_SRV = { 4, false, 30000 };  // 既定値

//================ 内部ユーティリティ（共通I/O） ================
static const char* kPath = "/config.json";

// load (存在しなければ空骨格を作るオプション付き)
static bool loadDoc(JsonDocument& doc, bool createIfMissing=true) {
  if (!LittleFS.begin(true)) return false;
  if (!LittleFS.exists(kPath)) {
    if (!createIfMissing) return false;
    // 空骨格
    doc.clear();
    doc["server"];                 // object（空）
    doc["host"].to<JsonArray>();   // array
    doc["wifi"].to<JsonArray>();   // array
    File nf = LittleFS.open(kPath, "w");
    if (!nf) return false;
    serializeJsonPretty(doc, nf);
    nf.close();
    return true;
  }
  File f = LittleFS.open(kPath, "r");
  if (!f) return false;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  return !err;
}

static bool saveDoc(const JsonDocument& doc){
  File f = LittleFS.open(kPath, "w");
  if (!f) return false;
  serializeJsonPretty(doc, f);
  f.close();
  return true;
}

//================ 設定ファイル読込み =================
// -----------------------------
// jsonファイルを新規作成
// -----------------------------
bool loadConfig_create() {

  // 既存ファイルがあれば正常でリターン
  if (LittleFS.exists("/config.json")) return true;

  // 空の骨格を作成
  if (!cfgCreateEmpty()) return false;

  // server（既定値は g_SRV を採用）
  if (!cfgSetServer(g_SRV.maxClients, g_SRV.writeLock, g_SRV.writeLockMs)) return false;

  // host（sta/ap）
  if (!cfgAddOrUpdateHost("sta", "mmp-sta-mode", "254"))                 return false; // 末尾オクテット or ""=DHCP
  if (!cfgAddOrUpdateHost("ap",  "mmp-ap-mode",  "192.168.254.254"))     return false; // フルIP

  // wifi 候補
  if (!cfgAddOrUpdateWifi("home", "Buffalo-G-C0F0", "6shkfa53dk8ks", true))  return false;
  if (!cfgAddOrUpdateWifi("mbph", "kureha",        "09017390454",    false)) return false;
  if (!cfgAddOrUpdateWifi("mbrt", "Kureha-wimax",  "090173904541012",false)) return false;

  // 正常でリターン
  return true;
}
// -----------------------------
// jsonファイルを読み込む
// -----------------------------
bool loadConfig_read() {
  // jsonファイル読取を開始
  File f = LittleFS.open("/config.json", "r");
  if (!f) return false;

  // ファイルサイズに応じた余裕ある容量で
  size_t sz  = f.size();
  size_t cap = sz + 1024;
  if (cap < 4096) cap = 4096;
  if (cap > 16384) cap = 16384;

  DynamicJsonDocument doc(cap);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) { return false; }

  // 情報読取：サーバー（キーが無くても既定が生きる）
  g_SRV.maxClients  = doc["server"]["max_clients"   ] | g_SRV.maxClients;
  g_SRV.writeLock   = doc["server"]["write_lock"    ] | g_SRV.writeLock;
  g_SRV.writeLockMs = doc["server"]["write_lock_ms" ] | g_SRV.writeLockMs;

  // 補正
  if (g_SRV.maxClients  < 1  ) g_SRV.maxClients = 1;
  if (g_SRV.maxClients  > 16 ) g_SRV.maxClients = 16;
  if (g_SRV.writeLockMs < 100) g_SRV.writeLockMs = 100;  // 0.1秒未満は禁止

  // 情報読取：ホスト
  g_WIFI.hostNum = 0;
  if (doc["host"].is<JsonArray>()) {
    for (JsonObject c : doc["host"].as<JsonArray>()) {
      // 上限チェック：配列長から算出
      if (g_WIFI.hostNum >= (int)(sizeof(g_WIFI.hostList)/sizeof(g_WIFI.hostList[0]))) break;

      // バリデーション
      String type = String((const char*)(c["type"] | ""));
      String name = String((const char*)(c["name"] | ""));
      String ip   = String((const char*)(c["ip"  ] | ""));

      // 追加：前後の空白を除去
      type.trim();
      name.trim();
      ip.trim();

      // ここでバリデーション
      if (!(type.equalsIgnoreCase("sta") || type.equalsIgnoreCase("ap"))) continue;
      if (!name.length()) continue;
      if (type.equalsIgnoreCase("sta")) {
          if (ip.length() > 0) {
            bool digits = true;
            for (size_t i = 0; i < ip.length(); ++i) {
              if (!isDigit((unsigned char)ip[i])) {digits = false; break;}
            }
            if (digits) {
              long v = strtol(ip.c_str(), nullptr, 10);
              if (v < 0 || v > 255) ip = "";
            } else ip = "";
          }
        }

      // 代入
      auto& h = g_WIFI.hostList[g_WIFI.hostNum];
      h.type = type;
      h.name = name;
      h.ip   = ip;
      g_WIFI.hostNum++;
    }
  }

  // 情報読取：Wi-Fi候補
  g_WIFI.candNum = 0;
  if (doc["wifi"].is<JsonArray>()) {
    for (JsonObject c : doc["wifi"].as<JsonArray>()) {
      if (g_WIFI.candNum >= (int)(sizeof(g_WIFI.candList)/sizeof(g_WIFI.candList[0]))) break;
      auto& w  = g_WIFI.candList[g_WIFI.candNum];
      w.label     = String((const char*)(c["label" ] | ""));
      w.ssid      = String((const char*)(c["ssid"  ] | ""));
      w.pass      = String((const char*)(c["pass"  ] | ""));
      w.isDefault = (bool)(c["default"] | false);  // "default" → isDefault
      if (w.ssid.length()) g_WIFI.candNum++;
    }
  }

  // 正常でリターン
  return true;
}
//━━━━━━━━━━━━━
// 設定ファイル読込み
//━━━━━━━━━━━━━
bool loadConfig() {

  if (!LittleFS.begin(true)) return false;

  // config.json が無い場合は自動生成（APIで組み立て）
  if (!loadConfig_create()) return false;

  // JSONファイルを読み込み
  return loadConfig_read();
}

//================ JSON操作API 実装 ==================

// ファイル削除
bool cfgDeleteJson(){
  if (!LittleFS.begin(true)) return false;
  if (LittleFS.exists(kPath)) return LittleFS.remove(kPath);
  return true; // もともと無い
}

// 空のJSON（最小骨格）を作成
bool cfgCreateEmpty(){
  JsonDocument doc;
  doc["server"];                 // 空object
  doc["host"].to<JsonArray>();   // 空配列
  doc["wifi"].to<JsonArray>();   // 空配列
  return saveDoc(doc);
}

// server を登録・更新
bool cfgSetServer(int max_clients, bool write_lock, uint32_t write_lock_ms){
  JsonDocument doc;
  if (!loadDoc(doc, /*createIfMissing*/true)) return false;
  JsonObject s = doc["server"].to<JsonObject>();
  s["max_clients"]   = max_clients;
  s["write_lock"]    = write_lock;
  s["write_lock_ms"] = write_lock_ms;
  return saveDoc(doc);
}

// server を削除
bool cfgClearServer(){
  JsonDocument doc;
  if (!loadDoc(doc, true)) return false;
  doc.remove("server");
  return saveDoc(doc);
}

// host 追加/更新（type をキーとして上書き。見つからなければ追加）
bool cfgAddOrUpdateHost(
  const String& type,   // "sta" | "ap"
  const String& name,   // hostname
  const String& ip      // STA:末尾オクテット or "", AP:フルIP
){

  JsonDocument doc;
  if (!loadDoc(doc, true)) return false;
  JsonArray ha = doc["host"].to<JsonArray>();
  bool updated=false;
  for (JsonObject h : ha){
    String t = String((const char*)(h["type"] | ""));
    if (t.equalsIgnoreCase(type)){
      h["name"] = name;
      h["ip"]   = ip;
      updated = true;
      break;
    }
  }
  if (!updated){
    JsonObject h = ha.add<JsonObject>();
    h["type"] = type;
    h["name"] = name;
    h["ip"]   = ip;
  }
  return saveDoc(doc);
}

// host 削除（type で削除）
bool cfgRemoveHostByType(const String& type){
  JsonDocument doc;
  if (!loadDoc(doc, true)) return false;
  JsonArray ha = doc["host"].to<JsonArray>();
  for (size_t i=0;i<ha.size();++i){
    JsonObject h = ha[i];
    String t = String((const char*)(h["type"] | ""));
    if (t.equalsIgnoreCase(type)){
      ha.remove(i);
      return saveDoc(doc);
    }
  }
  return true; // 見つからなくても true
}

// host 削除（name で削除）
bool cfgRemoveHostByName(const String& name){
  JsonDocument doc;
  if (!loadDoc(doc, true)) return false;
  JsonArray ha = doc["host"].to<JsonArray>();
  for (size_t i=0;i<ha.size();++i){
    JsonObject h = ha[i];
    String n = String((const char*)(h["name"] | ""));
    if (n.equalsIgnoreCase(name)){
      ha.remove(i);
      return saveDoc(doc);
    }
  }
  return true;
}

// wifi 追加/更新（label をキーに上書き or 追加）
bool cfgAddOrUpdateWifi(const String& label, const String& ssid, const String& pass, bool isDefault){
  JsonDocument doc;
  if (!loadDoc(doc, true)) return false;
  JsonArray wa = doc["wifi"].to<JsonArray>();
  bool updated=false;
  for (JsonObject w : wa){
    String l = String((const char*)(w["label"] | ""));
    if (l.equalsIgnoreCase(label)){
      w["ssid"]    = ssid;
      w["pass"]    = pass;
      w["default"] = isDefault;
      updated = true;
      break;
    }
  }
  if (!updated){
    JsonObject w = wa.add<JsonObject>();
    w["label"]   = label;
    w["ssid"]    = ssid;
    w["pass"]    = pass;
    w["default"] = isDefault;
  }
  return saveDoc(doc);
}

// wifi 削除（label）
bool cfgRemoveWifi(const String& label){
  JsonDocument doc;
  if (!loadDoc(doc, true)) return false;
  JsonArray wa = doc["wifi"].to<JsonArray>();
  for (size_t i=0;i<wa.size();++i){
    JsonObject w = wa[i];
    String l = String((const char*)(w["label"] | ""));
    if (l.equalsIgnoreCase(label)){
      wa.remove(i);
      return saveDoc(doc);
    }
  }
  return true;
}

// wifi default の変更（label 指定）
bool cfgSetWifiDefault(const String& label, bool isDefault){
  JsonDocument doc;
  if (!loadDoc(doc, true)) return false;
  JsonArray wa = doc["wifi"].to<JsonArray>();
  bool found=false;
  for (JsonObject w : wa){
    String l = String((const char*)(w["label"] | ""));
    if (l.equalsIgnoreCase(label)){
      w["default"] = isDefault;
      found=true;
      break;
    }
  }
  if (!found) return false; // 見つからなければ false
  return saveDoc(doc);
}
