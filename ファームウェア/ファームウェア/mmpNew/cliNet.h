// filename : cliNet.h
//========================================================
// クライアント：ネット(TCP/HTTP WEB API)
//--------------------------------------------------------
// Ver0.6.00 (2025/xx/xx)
//========================================================
#pragma once
#include <WiFi.h>
#include "modWIFI.h"    // 設定ファイル(新規作成/読込)
#include "cliNetTcp.h"  // サーバー：TCPブリッジ
#include "cliNetHttp.h" // サーバー：WEB API

//━━━━━━━━━━━━━━━━━
// ヘルパ
//━━━━━━━━━━━━━━━━━
  // ─────────────────
  // host 配列からtype一致のものを取得
  // （"sta" / "ap"）
  // ─────────────────
  static const typHost* findHostByType(const char* want) {
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
  static bool tryConnectOne(const typeWifi& argWifi)
  {
    if (argWifi.ssid.isEmpty()) return false;

    // STA ホスト情報を JSON から取得（無ければデフォルト）
    const typHost* sta = findHostByType("sta");
    String hostName = sta ? sta->name : String("mmp-sta-mode");
    String staIpStr = sta ? sta->ip   : String("");  // 末尾オクテット or 空

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

  // ─────────────────
  // OLED表示
  // ─────────────────
  void printOLED(const String& s){
  }

//━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━
bool InitNet(){

  Serial.println(String("---------------------------"));
  Serial.println(String("Wifiを初期化中..."));

  // 設定読み込み
  if (!InitWifi()) {
    Serial.println(String("設定内容が読み込めず、規定値で接続します。"));
  }

  // Wi-Fi: 候補を順に試行（仮実装：isDefault優先は後で統合時に実装）
  bool ok = false;
  for (int i=0; i<g_WIFI.candNum && !ok; i++){
    Serial.println(String("Connect SSID=") + g_WIFI.candList[i].ssid.c_str());
    ok = tryConnectOne(g_WIFI.candList[i]);
  }

  // 
  if (ok){
  // → 接続成功
    // メッセージ表示
    Serial.println(String("Wifi = STA mode"));
    Serial.println(String("SSID = ") + WiFi.SSID().c_str());
    Serial.println(String("IP   = ") + WiFi.localIP().toString().c_str());

  // → 接続失敗
  } else {
    // APモードで起動
    const typHost*  ap      = findHostByType("ap");
    String          apSsid  = ap ? ap->name : String("mmp-ap-mode");
    IPAddress       apIp    = ap ? buildApIp(ap->ip) : IPAddress(192,168,254,254);
    if (!apIp) apIp = IPAddress(192,168,254,254);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIp, apIp, IPAddress(255,255,255,0));
    if (WiFi.softAP(apSsid.c_str())) {
    // → 接続成功
      // メッセージ表示
      Serial.println(String("Wifi = AP mode"));
      Serial.println(String("SSID = ") + apSsid.c_str());
      Serial.println(String("IP   = ") + WiFi.softAPIP().toString().c_str());
    } else {
    // → 接続失敗
      // メッセージ表示
      Serial.println(String("Wifi = failed !"));
      return false;
    }
  }

  // クライアントのハンドルを作成
  int srcPort;

    // HTTP(WEB API)
    srcPort = 8080; if (!srvHttp::start(srcPort)) return false;
    Serial.println(String("HTTP = ") + String(srcPort));

    // TCP
    srcPort = 8081; if (!srvTcp::begin (srcPort)) return false;
    Serial.println(String("TCP  = ") + String(srcPort));

  Serial.println(String("Wifiを初期化済み\n"));
 return true;
}