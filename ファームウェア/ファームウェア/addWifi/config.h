// filename : config.h
// 設定ファイル(新規作成/読込)

#pragma once
#include <Arduino.h>

// 上限数（必要なら後で調整）
constexpr int kMaxHosts = 4;
constexpr int kMaxWifi  = 6;

//━━━━━━━━━━━━━━━
// 接続情報
//━━━━━━━━━━━━━━━
// 接続情報：ホスト（JSON: host[]）
struct typHost {
  String type;  // "sta" | "ap"
  String name;  // hostname
  String ip;    // STA:末尾オクテット or "", AP:フルIP
};

// 接続情報：Wi-Fi候補（JSON: wifi[]）
struct typeWifi {
  String label;
  String ssid;
  String pass;
  bool   isDefault = false;  // JSONの "default"
};

// 接続情報ひとまとめ
struct typeConnect {
  typHost  hostList[kMaxHosts];
  typeWifi candList[kMaxWifi];
  int      hostNum = 0;
  int      candNum = 0;
};

//━━━━━━━━━━━━━━━
// サーバ情報（JSON: server）
//━━━━━━━━━━━━━━━
struct typeServer {
  int      maxClients   = 0;
  bool     writeLock    = false;
  uint32_t writeLockMs  = 0;
};

//━━━━━━━━━━━━━━━
/* グローバル（config.cppで定義） */
//━━━━━━━━━━━━━━━
extern typeConnect g_WIFI;
extern typeServer  g_SRV;

//━━━━━━━━━━━━━━━
// 設定読込み
//━━━━━━━━━━━━━━━
bool loadConfig();  // 実装は config.cpp


//━━━━━━━━━━━━━━━
// JSON操作API（LittleFS:/config.json を直接更新）
//  成功:true / 失敗:false
//━━━━━━━━━━━━━━━

// ファイル操作
bool cfgDeleteJson();                 // JSONファイルを削除
bool cfgCreateEmpty();                // 空のJSON（最小骨格）を新規作成

// server
bool cfgSetServer(int max_clients, bool write_lock, uint32_t write_lock_ms);
bool cfgClearServer();                // "server" キーを削除

// host
bool cfgAddOrUpdateHost(const String& type, const String& name, const String& ip);
// ※削除キーは type or name のどちらかで運用できるよう2種用意
bool cfgRemoveHostByType(const String& type);
bool cfgRemoveHostByName(const String& name);

// wifi
bool cfgAddOrUpdateWifi(const String& label, const String& ssid, const String& pass, bool isDefault=false);
bool cfgRemoveWifi(const String& label);
bool cfgSetWifiDefault(const String& label, bool isDefault);