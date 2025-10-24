// filename : cliNet.h
//========================================================
// クライアント：ネット(TCP/HTTP WEB API)
//--------------------------------------------------------
// Ver 0.6.0 (2025/xx/xx)
//========================================================
#pragma once
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "modNET.h"    // 設定ファイル(新規作成/読込)
#include "cliNetTcp.h"  // サーバー：TCPブリッジ
#include "cliNetHttp.h" // サーバー：WEB API

//━━━━━━━━━━━━━━━━━
// ヘルパ
//━━━━━━━━━━━━━━━━━
  // ─────────────────
  // host 配列からtype一致のものを取得
  // （"sta" / "ap"）
  // ─────────────────
  static const typeHost* findHostByType(const char* want) {
    for (int i=0; i<g_WIFI.hostNum; ++i)
      if (g_WIFI.hostList[i].type == want) return &g_WIFI.hostList[i];
    return nullptr;
  }

  // ─────────────────
  // "254" → 末尾オクテット
  // 空や不正は false
  // ─────────────────
  static bool parseLastOctet(const String& s, uint8_t& out) {
    if (s.length()==0) return false;
    for (char c: s) if (c<'0'||c>'9') return false;
    long v = strtol(s.c_str(), nullptr, 10);
    if (v<0 || v>255) return false;
    out = (uint8_t)v;
    return true;
  }

  // ─────────────────
  // DHCPで得たIPの第1〜第3オクテットを
  // 使い、末尾を差し替え
  // 不正なら 0.0.0.0
  // ─────────────────
  static IPAddress buildStaStaticIp(const IPAddress& argDhcpIP, const String& argLastOctet) {
    uint8_t last=0;
    if (!parseLastOctet(argLastOctet, last)) return IPAddress(); // 0.0.0.0
    return IPAddress(argDhcpIP[0], argDhcpIP[1], argDhcpIP[2], last);
  }

  // ─────────────────
  // AP用：フル "x.x.x.x" を優先、
  // 末尾だけなら 192.168.254.X
  // どちらでもなければ 0.0.0.0
  // ─────────────────
  static IPAddress buildApIp(const String& s) {
    IPAddress ip;
    if (ip.fromString(s)) return ip;             // フル表記
    uint8_t last=0;
    if (parseLastOctet(s, last)) return IPAddress(192,168,254,last);
    return IPAddress(); // 0.0.0.0
  }

  // ─────────────────
  // Wi-Fi候補1つを試行
  // （DHCP → 必要なら静的IPへ再接続）
  // 要件：sta.ipが空ならDHCPのまま採用
  // ─────────────────
  static bool tryConnectOne(const typeRouter& argWifi)
  {
    if (argWifi.ssid.isEmpty()) return false;

    // STA ホスト情報を JSON から取得（無ければデフォルト）
    const typeHost* sta = findHostByType("sta");
    String hostName     = sta ? sta->name : String("mmp-sta-mode");
    String staIpStr     = sta ? sta->ip   : String("");  // 末尾オクテット or 空

    // ① DHCPで接続（第三オクテット把握のため）
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(hostName.c_str());
    WiFi.begin(argWifi.ssid.c_str(), argWifi.pass.c_str());

    // 8秒間接続トライ
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis()-t0) < 8000) {
      delay(200);
    }
    // → 接続失敗
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect(true, true);
      delay(200);
      return false;
    }

    // DHCP情報を退避
    IPAddress dhcpIP     = WiFi.localIP();
    IPAddress gatewayIP  = WiFi.gatewayIP();
    IPAddress subnetMask = WiFi.subnetMask();
    IPAddress dnsIP1     = WiFi.dnsIP(0);
    IPAddress dnsIP2     = WiFi.dnsIP(1);

    // ② sta.ip が空なら DHCP のまま採用（要件）
    if (staIpStr.length() == 0) return true;

    // 末尾オクテットで静的IPを合成
    IPAddress staStatic = buildStaStaticIp(dhcpIP, staIpStr);
    // →（失敗なら DHCP のまま）
    if (!staStatic) return true;

    // サブネットは固定 /24
    // GW は DHCP 優先・無ければ x.y.z.1、DNS 未取得なら GW
    subnetMask = IPAddress(255,255,255,0);
    if (!gatewayIP) gatewayIP = IPAddress(staStatic[0], staStatic[1], staStatic[2], 1);
    if (!dnsIP1   ) dnsIP1 = gatewayIP;

    // ③ 静的IPで再接続
    WiFi.disconnect(false, false);
    delay(100);
    WiFi.config(staStatic, gatewayIP, subnetMask, dnsIP1, dnsIP2);
    WiFi.begin(argWifi.ssid.c_str(), argWifi.pass.c_str());

    // 8秒間接続トライ
    t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis()-t0) < 8000) {
      delay(200);
    }
    // → 接続失敗
    if (WiFi.status() == WL_CONNECTED) return true;

    // 失敗したら切断（必要ならここで DHCP に戻す実装も可）
    WiFi.disconnect(true, true);
    delay(200);
    return false;
  }

//━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━
  //─────────────────
  // 初期化 ① 設定ファイル読込２
  //─────────────────
  bool InitNet_Json2() {
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
  //─────────────────
  // 初期化 ① 設定ファイル読込１
  //─────────────────
  bool InitNet_Json1(){

    // ファイルシステムが機能しない場合は失敗
    if (!LittleFS.begin(true)) {
      Serial.println("　[Error] LittleFS初期化に失敗)");
      return false;
    }

    // → 設定ファイルが無い場合：
    if (!LittleFS.exists("/config.json")) {
      if (!jsonCreate()) { 
        Serial.println("　[Error] 設定ファイルの初期作成に失敗");
        return false;
      }
      Serial.println("　[Notice] 設定ファイルの初期作成が完了");
    }

    // JSONファイルを読み込み
    if (!InitNet_Json2()) {
      Serial.println("　[Error] 設定ファイルの読込に失敗");
      return false;
    }
    Serial.println("　-> 設定ファイルの読込が完了");

    // 正常でリターン
    return true;
  }
  //─────────────────
  // 初期化 ③ Wifiの設定
  //─────────────────
  bool InitNet_Wifi(){

    // Wi-Fi: 候補を順に試行（仮実装：isDefault優先は後で統合時に実装）
    bool ok = false;
    for (int i=0; i<g_WIFI.candNum && !ok; i++){
      Serial.println(String("　Try SSID=") + g_WIFI.candList[i].ssid.c_str());
      // ＳＴＡモードで起動
      ok = tryConnectOne(g_WIFI.candList[i]);
    }

    // 起動状態で処理を分岐：
    if (ok){
    // → ＳＴＡモードで起動できた場合：
      // メッセージ表示
      Serial.println("　STAモード");
      Serial.println(String("　 SSID: ") + WiFi.SSID().c_str());
      Serial.println(String("　 IP  : ") + WiFi.localIP().toString().c_str());

    // → ＳＴＡモードで起動できない場合：
    } else {

      // APモードで起動
      const typeHost*  ap      = findHostByType("ap");
      String          apSSID  = ap ? ap->name : String("mmp-ap-mode");
      IPAddress       apIP    = ap ? buildApIp(ap->ip) : IPAddress(192,168,254,254);

      if (!apIP) apIP = IPAddress(192,168,254,254);
      WiFi.mode(WIFI_AP);
      WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));

      // ＡＰモードで起動
      if (WiFi.softAP(apSSID.c_str())) {
      // → 接続成功
        // メッセージ表示
        Serial.println("　APモード");
        Serial.println(String("　 SSID: ") + apSSID.c_str());
        Serial.println(String("　 IP  : ") + WiFi.softAPIP().toString().c_str());

      // → 接続失敗
      } else {
        // メッセージ表示
        Serial.println("　[Error] Wifiを起動できません");
        return false;
      }
    }

    // 正常でリターン
    return true;
  }
  //─────────────────
  // 初期化 ④ サービス作成
  //─────────────────
  bool InitNet_Service(){

    int srcPort;

    // HTTP(WEB API)
    srcPort = 8080; if (!srvHttp::start(srcPort)) return false;
    Serial.println(String("　HTTP port : ") + String(srcPort));

    // TCP
    srcPort = 8081; if (!srvTcp::begin (srcPort)) return false;
    Serial.println(String("　TCP  port : ") + String(srcPort));

    // 正常でリターン
    return true;
  }
  //─────────────────
  // 初期化処理のメイン
  //─────────────────
  bool InitNet(){

  Serial.println("---------------------------");
  Serial.println("[Network initialize]");

    Serial.println("１．設定ファイルの読込");
    if (!InitNet_Json1())  return false;

    Serial.println("２．Wifiの設定");
    if (!InitNet_Wifi())  return false;

    Serial.println("３．サービスの起動");
    if (!InitNet_Service())  return false;

    return true;
  }
