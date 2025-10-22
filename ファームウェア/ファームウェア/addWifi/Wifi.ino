// Wifi.ino
// =============================================================
// Wifiサーバ：MMPファームウェア統合前
// =============================================================
#include <WiFi.h>
#include "config.h"     // 設定ファイル(新規作成/読込)
#include "srvTcp.h"     // サーバー：TCPブリッジ
#include "srvHttp.h"    // サーバー：WEB API

//==================== ダミー本体連携フック ====================
// WebAPI/TCPブリッジから呼ばれる本体連携関数（ダミー実装）
// wire 例: "DIGITAL/INPUT:13!" / "INFO/VERSION!"
String MMP_REQUEST(const String& wire) {
  // もしデバッグしたければ下の1行のコメントを外してください
  // Serial.printf("[MMP] wire='%s' -> '!!!!!'\n", wire.c_str());
  return String("!!!!!");
}

//==============================================================
// ヘルパ
//==============================================================

// host 配列から type一致のものを取得（"sta" / "ap"）
static const typHost* findHostByType(const char* want) {
  for (int i=0; i<g_WIFI.hostNum; ++i)
    if (g_WIFI.hostList[i].type == want) return &g_WIFI.hostList[i];
  return nullptr;
}

// "254" → 末尾オクテット, 空や不正は false
static bool parseLastOctet(const String& s, uint8_t& out) {
  if (s.length()==0) return false;
  for (char c: s) if (c<'0'||c>'9') return false;
  long v = strtol(s.c_str(), nullptr, 10);
  if (v<0 || v>255) return false;
  out = (uint8_t)v;
  return true;
}

// DHCPで得たIPの第1〜第3オクテットを使い、末尾を差し替え（不正なら 0.0.0.0）
static IPAddress buildStaStaticIp(const IPAddress& dhcpIp, const String& lastOctetStr) {
  uint8_t last=0;
  if (!parseLastOctet(lastOctetStr, last)) return IPAddress(); // 0.0.0.0
  return IPAddress(dhcpIp[0], dhcpIp[1], dhcpIp[2], last);
}

// AP用：フル "x.x.x.x" を優先、末尾だけなら 192.168.254.X、どちらでもなければ 0.0.0.0
static IPAddress buildApIp(const String& s) {
  IPAddress ip;
  if (ip.fromString(s)) return ip;             // フル表記
  uint8_t last=0;
  if (parseLastOctet(s, last)) return IPAddress(192,168,254,last);
  return IPAddress(); // 0.0.0.0
}

//==============================================================
// Wi-Fi候補1つを試行（DHCP → 必要なら静的IPへ再接続）
// 要件：sta.ip が空なら DHCP のまま採用
//==============================================================
static bool tryConnectOne(const typeWifi& argWifi)
{
  if (argWifi.ssid.isEmpty()) return false;

  // STA ホスト情報を JSON から取得（無ければデフォルト）
  const typHost* sta = findHostByType("sta");
  String hostName = sta ? sta->name : String("mmp-sta-mode");
  String staIpStr = sta ? sta->ip   : String("");  // 末尾オクテット or 空

  // ① まず DHCP で接続（第三オクテット把握のため）
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostName.c_str());
  WiFi.begin(argWifi.ssid.c_str(), argWifi.pass.c_str());

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis()-t0) < 8000) {
    delay(200);
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true, true);
    delay(200);
    return false;
  }

  // DHCP 情報
  IPAddress dhcpIp = WiFi.localIP();
  IPAddress gw     = WiFi.gatewayIP();
  IPAddress subnet = WiFi.subnetMask();
  IPAddress dns1   = WiFi.dnsIP(0);
  IPAddress dns2   = WiFi.dnsIP(1);

  // ② sta.ip が空なら DHCP のまま採用（要件）
  if (staIpStr.length() == 0) return true;

  // 末尾オクテットで静的IPを合成（不正なら DHCP のまま）
  IPAddress staStatic = buildStaStaticIp(dhcpIp, staIpStr);
  if (!staStatic) return true;

  // サブネットは固定 /24、GW は DHCP 優先・無ければ x.y.z.1、DNS 未取得なら GW
  subnet = IPAddress(255,255,255,0);
  if (!gw)   gw   = IPAddress(staStatic[0], staStatic[1], staStatic[2], 1);
  if (!dns1) dns1 = gw;

  // ③ 静的IPで再接続
  WiFi.disconnect(false, false);
  delay(100);
  WiFi.config(staStatic, gw, subnet, dns1, dns2);
  WiFi.begin(argWifi.ssid.c_str(), argWifi.pass.c_str());

  t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis()-t0) < 8000) {
    delay(200);
  }
  if (WiFi.status() == WL_CONNECTED) return true;

  // 失敗したら切断（必要ならここで DHCP に戻す実装も可）
  WiFi.disconnect(true, true);
  delay(200);
  return false;
}

//━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━
void setup(){

  Serial.begin(115200);

  // USB CDC の列挙待ち（最大2秒）
  unsigned long dl = millis() + 2000;
  while (!Serial && millis() < dl) delay(10);

  // 設定読み込み
  if (!loadConfig()) {
    Serial.println("設定内容が読み込めず、規定値で接続します。");
  }

  // Wi-Fi: 候補を順に試行（仮実装：isDefault優先は後で統合時に実装）
  bool ok = false;
  for (int i=0; i<g_WIFI.candNum && !ok; i++){
    Serial.printf("WiFi try %d/%d: SSID=%s\n",
      i+1, g_WIFI.candNum, g_WIFI.candList[i].ssid.c_str());
    ok = tryConnectOne(g_WIFI.candList[i]);
  }

  // (すべて失敗) -> AP フォールバック
  if (!ok){
    Serial.println("[ERROR] STA接続に失敗。APフォールバックへ");

    const typHost* ap = findHostByType("ap");
    String apSsid = ap ? ap->name : String("mmp-ap-mode");
    IPAddress apIp = ap ? buildApIp(ap->ip) : IPAddress(192,168,254,254);
    if (!apIp) apIp = IPAddress(192,168,254,254);

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIp, apIp, IPAddress(255,255,255,0));   // GW=自分, /24
    // パスワードを付けたい場合は第2引数に8文字以上
    if (WiFi.softAP(apSsid.c_str())) {
      Serial.printf("[AP ] SSID=%s  IP=%s\n",
        apSsid.c_str(), WiFi.softAPIP().toString().c_str());
    } else {
      Serial.println("[AP ] 起動失敗");
    }
  } else {
    Serial.printf("[STA] connected  SSID=%s  IP=%s\n",
      WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  }

  // ---- WebAPI(8080) 起動 ----
  if (srvHttp::start(8080)) {
    //IPAddress ip = (WiFi.getMode()==WIFI_AP) ? WiFi.softAPIP() : WiFi.localIP();
    //Serial.printf("[HTTP] WebAPI listening on %s:%d\n", ip.toString().c_str(), 8080);
    Serial.printf("[HTTP] listening on :%d\n", 8080);
  }

  // ---- TCPブリッジ(8081) 起動 ----
  if (srvTcp::start(8081, g_SRV.maxClients)) {
    Serial.printf("[TCP] listen %d (max=%d)\n", 8081, g_SRV.maxClients);
  }
}

//━━━━━━━━━━━━━━━
// ループ（統合後にメイン側で回す想定）
//━━━━━━━━━━━━━━━
void loop(){
  // ---- 統合先で回すときは下記を有効化してください ----
  srvHttp::handle();  // WebAPIのハンドラ
  srvTcp::handle();   // TCPブリッジのハンドラ

  // 今回は基礎層のため何もしない
  delay(1);
}
