// filename : MMP.cpp
//============================================================
// アプリ接続
// バージョン：0.5
//============================================================
#include "MMP.h"
#include "MmpClient.h"
#include "config.hpp"   // TCPブリッジサーバより流用
#include "webui.hpp"    // TCPブリッジサーバより流用

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
using Mmp::Core::MmpClient;
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

//─────────────
// Wi-Fi可否を判定
//─────────────
#if __has_include(<WiFi.h>)
  #include <WiFi.h>
  #define MMP_HAS_WIFI 1  // Wifi機能：あり
#else
  #define MMP_HAS_WIFI 0  // Wifi機能：なし
#endif

//─────────────
// ファイルシステム可否を判定
//─────────────
#if __has_include(<LittleFS.h>)
  #include <LittleFS.h>
  #define MMP_HAS_LFS 1
#else
  #define MMP_HAS_LFS 0
#endif

//─────────────
// SDカード可否を判定
//─────────────
#if __has_include(<SD.h>)
  #include <SD.h>
  #define MMP_HAS_SD 1
#else
  #define MMP_HAS_SD 0
#endif


//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 無名名前空間：ヘルパ類のみ(接続処理本体はnamespace MMP内で定義)
namespace {

  //─────────────
  // "a=1&b=2"からkeyの値を取得
  // （無ければ defVal）
  //─────────────
  static String get_qparam(const String& qs, const char* key, const char* defVal) {
    int p = 0, n = qs.length();
    while (p < n) {
      int amp = qs.indexOf('&', p); if (amp < 0) amp = n;
      int eq  = qs.indexOf('=', p);
      if (eq > p && eq < amp) {
        String k = qs.substring(p, eq);
        String v = qs.substring(eq + 1, amp);
        if (k == key) return v;
      }
      p = amp + 1;
    }
    return String(defVal ? defVal : "");
  }
  //─────────────
  // 文字列→uint32（ms）
  // 妥当でなければ defMs
  //─────────────
  static uint32_t to_u32_ms(const String& s, uint32_t defMs) {
    if (s.length() == 0) return defMs;
    long v = s.toInt();
    if (v < 0) return defMs;
    return (uint32_t)v;
  }
  //─────────────
  // tcp://host:port?...
  //─────────────
  static bool parse_tcp_conn(
    const char* connCstr,
    String&     host,
    uint16_t&   port,
    uint32_t&   ioTimeoutMs,
    uint32_t&   verifyTimeoutMs
  ){
    host = ""; port = 0; ioTimeoutMs = 1000; verifyTimeoutMs = 2000;

    String s(connCstr ? connCstr : "");
    if (!s.startsWith("tcp://")) return false;

    String rest = s.substring(6);
    int qpos = rest.indexOf('?');
    String hp  = (qpos >= 0) ? rest.substring(0, qpos) : rest;
    String qstr = (qpos >= 0) ? rest.substring(qpos + 1) : "";

    int colon = hp.indexOf(':');
    if (colon <= 0) return false;
    host = hp.substring(0, colon);
    long portL = hp.substring(colon + 1).toInt();
    if (portL <= 0 || portL > 65535) return false;
    port = (uint16_t)portL;

    ioTimeoutMs     = to_u32_ms(get_qparam(qstr, "io"    , "1000"), 1000);
    verifyTimeoutMs = to_u32_ms(get_qparam(qstr, "verify", "2000"), 2000);
    return true;
  }

  //─────────────
  //
  //─────────────
  static void conln(Stream* log, const String& s) { if (log) log->println(s); }

  //─────────────
  //
  //─────────────
  static void con  (Stream* log, const String& s) { if (log) log->print  (s); }

  // ---------- 外部ファイル読込（共通） ----------
  //─────────────
  // 内容をStringで返す
  // (成功:true)
  //─────────────
  static bool read_file_to_string(const char* path, String& out, Stream* log) {

    out = "";

    #if MMP_HAS_LFS
        if (LittleFS.begin()) {
          if (LittleFS.exists(path)) {
            File f = LittleFS.open(path, "r");
            if (f) {
              while (f.available()) out += (char)f.read();
              f.close();
              return true;
            }
          }
        }
    #endif

    #if MMP_HAS_SD
        if (SD.begin()) {
          if (SD.exists(path)) {
            File f = SD.open(path, FILE_READ);
            if (f) {
              while (f.available()) out += (char)f.read();
              f.close();
              return true;
            }
          }
        }
    #endif

    (void)log;
    return false;
  }

  //─────────────
  // 前後の空白/改行を除去
  //─────────────
  static String trim_ws(const String& s) {
    int i = 0, j = s.length();
    while (i < j && isspace((unsigned char)s[i])) ++i;
    while (j > i && isspace((unsigned char)s[j-1])) --j;
    return s.substring(i, j);
  }

  //─────────────
  // "key":"value"を抽出
  //─────────────
  static bool extract_key(const String& src, const char* key, String& val) {

    // 1) JSON風："key":"value"
    String pat = String("\"") + key + "\"";
    int p = src.indexOf(pat);
    if (p >= 0) {
      int colon = src.indexOf(':', p + pat.length());
      if (colon >= 0) {
        int q1 = src.indexOf('"', colon + 1);
        int q2 = (q1 >= 0) ? src.indexOf('"', q1 + 1) : -1;
        if (q1 >= 0 && q2 > q1) {
          val = src.substring(q1 + 1, q2);
          return true;
        }
      }
    }

    // 2) INI風： key = value / key: value
    int start = 0;
    while (start < (int)src.length()) {
      int nl = src.indexOf('\n', start);
      if (nl < 0) nl = src.length();
      String line = trim_ws(src.substring(start, nl));
      start = nl + 1;
      if (line.length() == 0 || line[0] == '#') continue;
      String ks = String(key);
      if (line.startsWith(ks)) {
        int sep = line.indexOf('=');
        if (sep < 0) sep = line.indexOf(':');
        if (sep > 0) {
          String v = trim_ws(line.substring(sep + 1));
          if (v.length() >= 2 && v[0] == '"' && v[v.length()-1] == '"') {
            v = v.substring(1, v.length()-1);
          }
          val = v;
          return true;
        }
      }
    }
    return false;
  }

} // namespace（無名）
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━


String g_wifi_mode; // "STA" もしくは "AP"
String g_wifi_ssid; // 接続中の SSID / AP の SSID
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
namespace MMP {

  bool tryConnectOne(const WifiCand& c);    // Wi-Fi候補1つを試行
  void startAPFallback();                   // APフォールバック起動

//─────────────
// 終了（クローズ/切断）
//─────────────
void 終了(MmpClient& cli, bool wifiOff, Stream* log){
  cli.Close();

  #if MMP_HAS_WIFI
    if (wifiOff) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    }
  #else
    (void)wifiOff;
  #endif

  conln(log, F("<< 接続を終了しました >>"));
}

//─────────────
//─────────────
void シリアル待ち(Stream* log){
  if (log == &Serial) {
    unsigned long deadline = millis() + 15000;
    while (!Serial && millis() < deadline) delay(10);
    delay(50);
  }
}

//─────────────
// Wi-Fi 接続のみ
//─────────────
bool 通信接続_WiFi(const char* ssid, const char* pass, Stream* log){
  シリアル待ち(log);
  #if !MMP_HAS_WIFI
    conln(log, F("[ERROR] このボード/ビルドには Wi-Fi がありません。"));
    return false;
  #else
    if (!ssid || !*ssid) { conln(log, F("[ERROR] SSID が空です。")); return false; }

    con  (log, F("Wi-Fi 接続中"));
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(200);
      con(log, ".");
      if (millis() - t0 > 20000UL) {
        conln(log, F("\n[ERROR] Wi-Fi 接続タイムアウト。"));
        return false;
      }
    }
    conln(log, "");
    conln(log, String(F("IP: ")) + WiFi.localIP().toString());
    return true;
  #endif
}

//─────────────
// Wi-Fi 接続のみ
//─────────────
bool 通信接続_WiFi(const String& ssid, const String& pass, Stream* log){
  return 通信接続_WiFi(ssid.c_str(), pass.c_str(), log);
}

//─────────────
// TCP 接続のみ
//─────────────
bool 通信接続_Tcp(const char* conn, MmpClient& cli, Stream* log){
  シリアル待ち(log);
  String host; uint16_t port; uint32_t ioMs; uint32_t verMs;
  if (!parse_tcp_conn(conn, host, port, ioMs, verMs)) {
    conln(log, String(F("[ERROR] 接続指定が不正です: ")) + (conn ? conn : ""));
    return false;
  }

  con(log, String(F("接続中[")) + host + ":" + String(port) + F("]..."));
  if (!cli.ConnectTcp(host.c_str(), port, ioMs, verMs)) {
    conln(log, F("失敗"));
    conln(log, String(F("LastError: ")) + cli.LastError());
    return false;
  }
  conln(log, F("成功"));
  return true;
}

//─────────────
// TCP 接続のみ
//─────────────
bool 通信接続_Tcp(const String& conn, MmpClient& cli, Stream* log){
  return 通信接続_Tcp(conn.c_str(), cli, log);
}

//─────────────
// Wi-Fi設定を読込んで接続
//─────────────
bool 通信接続_WiFi_外部(const char* path, Stream* log){
  シリアル待ち(log);
  #if !MMP_HAS_WIFI
    conln(log, F("[ERROR] このボード/ビルドには Wi-Fi がありません。"));
    return false;
  #endif

  // 設定読み込み 
  if (!loadConfig()) {
    conln(log, F("設定内容が読み込めず、規定値で接続します。"));
  }

  // Wi-Fi: 候補を順に試行：
  bool ok = false;
  for (int i=0; i<WIFI.candN && !ok; i++){
    conln(log, String("WiFi try ") + String(i+1) + "/" + String(WIFI.candN) +
                 ": SSID=" + WIFI.cand[i].ssid +
                 " timeout=" + String((unsigned long)WIFI.cand[i].timeout_ms) + "ms");
    ok = tryConnectOne(WIFI.cand[i]);
  }

  // ★試行結果で処理分岐：
  // (すべて失敗) -> AP フォールバック
  if (!ok){
    conln(log, F("設定内容での接続に失敗しました。"));

    // APフォールバック
    if (WIFI.apfb.enabled){
      startAPFallback();
      conln(log, String("AP mode: SSID=") + g_wifi_ssid +
                   "  IP=" + WiFi.softAPIP().toString());

    // STAのままIP無しで継続
    } else {
      g_wifi_mode = "STA"; g_wifi_ssid = "(disconnected)";
    }

  // (STA接続成功) -> 状態更新
  } else {
    conln(log, String("WiFi connected: SSID=") + g_wifi_ssid +
                 "  IP=" + WiFi.localIP().toString());
  }

  // WebUI起動・起動ステータス表示
  webui_begin();
  conln(log, buildStartupStatusText());

  return true;
}

//─────────────
// Wi-Fi候補1つを試行
//─────────────
bool tryConnectOne(const WifiCand& c){

  // SSIDが空なら失敗
  if (c.ssid.isEmpty()) return false;

  // Wi-Fi接続
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(WIFI.hostname.c_str());
  WiFi.begin(c.ssid.c_str(), c.pass.c_str());

  // 接続完了待ち（簡易待機）
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis()-t0) < c.timeout_ms) {
    delay(200);
  }

  // ★接続結果で処理分岐：
  // (成功) -> 状態更新
  if (WiFi.status() == WL_CONNECTED) {
    g_wifi_mode = "STA";
    g_wifi_ssid = c.ssid;
    return true;

  // (失敗) -> 切断して false
  } else {
    #if defined(ARDUINO_ARCH_ESP32)
      WiFi.disconnect(true, true);
    #else
      WiFi.disconnect(true);     // RP2040(Earle core) は引数1つ
    #endif
    delay(200);
    return false;
  }
}

//─────────────
// APフォールバック起動
//─────────────
void startAPFallback(){
  if (!WIFI.apfb.enabled) return;
  #if MMP_HAS_WIFI
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI.apfb.ssid.c_str(), WIFI.apfb.pass.length() ? WIFI.apfb.pass.c_str() : nullptr);
  #endif
  g_wifi_mode = "AP";
  g_wifi_ssid = WIFI.apfb.ssid;
}

//─────────────
// 複合：Wi-Fi（必要なら）
// + TCP / UART auto
//─────────────
bool 通信接続(
    const char* conn,
    MmpClient&  cli,
    const char* wifiSsid,
    const char* wifiPass,
    Stream*     log
){
  シリアル待ち(log);
  conln(log, F("<< ＭＭＰライブラリ for Arduino >>"));

  if (!conn) conn = "auto";

  if (strncmp(conn, "auto", 4) == 0) {
    con  (log, F("接続中(UART auto)..."));
    if (!cli.ConnectAutoBaud()) {
      conln(log, F("失敗"));
      conln(log, String(F("LastError: ")) + cli.LastError());
      終了(cli, false, log);
      return false;
    }
    conln(log, F("成功"));

  } else if (strncmp(conn, "tcp://", 6) == 0) {

    #if MMP_HAS_WIFI
        bool netReady = (WiFi.status() == WL_CONNECTED);
        if (!netReady) {
          if (wifiSsid && *wifiSsid) {
            if (!通信接続_WiFi(wifiSsid, wifiPass, log)) {
              終了(cli, true, log);
              return false;
            }
            netReady = true;
          } else {
            if (!通信接続_WiFi_外部(nullptr, log)) {
              終了(cli, true, log);
              return false;
            }
            netReady = true;
          }
        }
        (void)netReady;
    #endif

    if (!通信接続_Tcp(conn, cli, log)) {
      終了(cli, true, log);
      return false;
    }

  } else {
    conln(log, String(F("[ERROR] 未対応の接続指定: ")) + conn);
    終了(cli, false, log);
    return false;
  }

  #if MMP_HAS_WIFI
    if (WiFi.status() == WL_CONNECTED) {
      conln(log, String(F("IP address(WiFi): ")) + WiFi.localIP().toString());
    }
  #endif

  conln(log, String(F("通信ポート  : ")) + cli.ConnectedPort()                );
  conln(log, String(F("接続速度    : ")) + String(cli.ConnectedBaud()) + "bps");
  conln(log, String(F("MMP firmware: ")) + String(cli.INFO.VERSION())         );
  conln(log, String(F("PCA9685  [0]: ")) + String(cli.PWM.INFO.CONNECT(0))    );
  conln(log, String(F("DFPlayer [1]: ")) + String(cli.MP3.INFO.CONNECT(1))    );

  return true;
}

//─────────────
//─────────────
bool 通信接続(const String& conn,
    MmpClient&    cli,
    const char*   wifiSsid,
    const char*   wifiPass,
    Stream*       log){
  return 通信接続(conn.c_str(), cli, wifiSsid, wifiPass, log);
}

} // namespace MMP
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
