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

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 無名名前空間：ヘルパ類のみ(接続処理本体はnamespace MMP内で定義)
namespace {

  static bool g_STARTED_WEBUI = false; // WebUIを既に起動したか

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
  // ログ出力：改行あり
  // ログ出力：改行なし
  //─────────────
  static void conln(Stream* log, const String& s) { if (log) log->println(s); }
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



String g_WIFI_MODE; // "STA" もしくは "AP"
String g_WIFI_SSID; // 接続中の SSID / AP の SSID
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
namespace MMP {

  // Wi-Fi候補1つを試行
  bool tryConnectOne(const WifiCand& c,Stream* log);

  // WEB-UI起動
  void startSTA(Stream* log);
  void startAP (Stream* log, const char* note = nullptr);

//─────────────
// 終了（クローズ/切断）
//─────────────
void クライアント切断(MmpClient& cli, bool wifiOff, Stream* log){
  cli.Close();
  conln(log, "<< 切断しました >>");
}

//─────────────
// 状況表示
//─────────────
void 状況表示(MmpClient& cli, Stream* log){
  // 接続情報を表示
  conln(log, "\n<< MMP library[状況表示] >>");
  conln(log, "・通信ポート  : " + cli.ConnectedPort()                 );
  conln(log, "・接続速度    : " + String(cli.ConnectedBaud()) + " bps");
  conln(log, "・MMP firmware: " + String(cli.info.version())          );
  conln(log, "・PCA9685  [0]: " + String(cli.pwm.info.connect(0))     );
  conln(log, "・DFPlayer [1]: " + String(cli.mp3.info.connect(1))     );
}

//━━━━━━━━━━━━━
// 0) 一括：アプリ設定
// 2) 一括：JSON設定
//━━━━━━━━━━━━━
bool 通信接続(
  const char* conn,   // 接続文字列
  MmpClient&  cli,    // MMPクライアント
  const char* ssid,   // wifiのSSID
  const char* pass,   // SSIDのパスワード
  Stream*     log     // ログ用シリアル
){
  conln(log, "\n<< MMP library[一括接続] >>");

  #if MMP_HAS_WIFI
    // → wifi未接続の場合：
    bool netReady = (WiFi.status() == WL_CONNECTED);
    if (!netReady) {
      // wifiに接続する
      if (ssid && *ssid) netReady = 通信接続_WiFi(ssid, pass, log);
      else               netReady = 通信接続_WiFi_外部(log);
    }
  #endif

  // 接続文字列が無ければ、強制的に自動接続
  if (!conn) conn = "auto";

  // → 接続文字列が、シリアル自動接続の場合：
  if (strncmp(conn, "auto", 4) == 0) {

  conln(log, "-----------------------");
    con  (log, "[UART(auto)] ... ");

    if (!cli.ConnectAutoBaud()) {
      conln(log, "faile");
      conln(log, "LastError: " + cli.LastError());
      クライアント切断(cli, false, log);
      return false;
    }
    conln(log, "success");

  // → 接続文字列が、TCPブリッジの場合：
  } else if (strncmp(conn, "tcp://", 6) == 0) {

    #if MMP_HAS_WIFI
      // TCPブリッジ
      if (netReady){
        if (!通信接続_Tcp(conn, cli, log)) {
          クライアント切断(cli, true, log);
          return false;
        }
      }
    #else
      conln(log, "[ERROR] wifi未搭載の機器:" + String(conn));
      クライアント切断(cli, false, log);
      return false;
    #endif

  // →その他：
  } else {
    conln(log, "[ERROR] 不明な接続方法:" + String(conn));
    クライアント切断(cli, false, log);
    return false;
  }
  return true;
}
// オーバーロード
bool 通信接続(const String& conn,
    MmpClient&    cli ,
    const char*   ssid,
    const char*   pass,
    Stream*       log ){
  return 通信接続(conn.c_str(), cli, ssid, pass, log);
}

//━━━━━━━━━━━━━
// 個別：wifi接続(アプリ設定)
// 1) 個別：アプリ設定
//━━━━━━━━━━━━━
bool 通信接続_WiFi(
  const char* ssid, //
  const char* pass, //
  Stream*     log   // // ログ用シリアル
){
  conln(log, "\n<< MMP library[個別接続/wifi] >>");

  // → wifi非搭載機の場合：
  #if !MMP_HAS_WIFI
    conln(log, "[ERROR] wifi未搭載の機器");
    return false;

  // → wifi搭載機の場合：
  #else
    // → SSIDが未指定の場合：
    if (!ssid || !*ssid) {
      // WebUI(APモード)起動
      startAP(log, "[ERROR] SSID が空");
      return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    unsigned long t0 = millis();
    con(log, String(ssid) + " ");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      con(log, ".");
      // → タイムオーバーの場合：
      if (millis() - t0 > 20000UL) {
        // WebUI(APモード)起動
        conln(log, " fail");
        startAP(log, "[ERROR] Wi-Fi接続タイムアウト");
        return false;
      }
    }
    conln(log, " success");

    // WebUI(STAモード)起動
    // ※引数のSSIDを採用
    g_WIFI_SSID = ssid;
    startSTA(log);
    return true;
  #endif
}
// オーバーロード
bool 通信接続_WiFi(const String& ssid, const String& pass, Stream* log){
  return 通信接続_WiFi(ssid.c_str(), pass.c_str(), log);
}

//━━━━━━━━━━━━━
// 個別：wifi接続(JSON設定)
// 3) 個別：JSON設定
//━━━━━━━━━━━━━
bool 通信接続_WiFi_外部(Stream* log){

  conln(log, "\n<< MMP library[個別接続/wifi(json)] >>");

  #if !MMP_HAS_WIFI
    conln(log, "[ERROR] wifi未搭載の機器");
    return false;
  #endif

  // 設定読み込み 
  if (!loadConfig()) {
    // WebUI(APモード)起動
    startAP(log, "[ERROR] 設定ファイルなし");
    return false;
  }

  // Wi-Fi: 候補を順に試行：
  bool ok = false;
  for (int i=0; i<WIFI.candN && !ok; i++){
    con(log, "(" + String(i+1) + "/" + String(WIFI.candN) + ") " + WIFI.cand[i].ssid + " .");
    ok = tryConnectOne(WIFI.cand[i], log);
    conln(log, String(ok ? " success" : " fail"));
  }

  // (すべて失敗)
  if (!ok){
    // WebUI(APモード)起動
    startAP(log, "[ERROR] 設定ファイルの接続に失敗");
    return false;
  }

  // WebUI(STAモード)起動
  // ※SSIDはtryConnectOneで設定済み
  startSTA(log);
  return true;
}

//━━━━━━━━━━━━━
// TCPブリッジ
// 1) 個別：アプリ設定
// 3) 個別：JSON設定
//━━━━━━━━━━━━━
bool 通信接続_Tcp(
  const char* conn,
  MmpClient&  cli,
  Stream*     log   // // ログ用シリアル
){
  conln(log, "\n<< MMP library[個別接続/TCP] >>");

  String host; uint16_t port; uint32_t ioMs; uint32_t verMs;
  if (!parse_tcp_conn(conn, host, port, ioMs, verMs)) {
    conln(log, "[ERROR] 接続指定が不正" + String(conn ? conn : ""));
    return false;
  }

  con(log, " " + host + ":" + String(port) + " ... ");
  if (!cli.ConnectTcp(host.c_str(), port, ioMs, verMs)) {
    conln(log, "fail");
    conln(log, "LastError: " + cli.LastError());
    return false;
  }
  conln(log, "success");
  return true;
}
// オーバーロード
bool 通信接続_Tcp(const String& conn, MmpClient& cli, Stream* log){
  return 通信接続_Tcp(conn.c_str(), cli, log);
}


//============================================================
// 接続系ヘルパー
//============================================================
//─────────────
// Wi-Fi候補1つを試行
//─────────────
bool tryConnectOne(
  const   WifiCand& c,
  Stream* log
){
  // SSIDが空なら失敗
  if (c.ssid.isEmpty()) return false;

  // Wi-Fi接続
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(WIFI.hostname.c_str());
  WiFi.begin(c.ssid.c_str(), c.pass.c_str());

  // 接続完了待ち（簡易待機）
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis()-t0) < c.timeout_ms) {
    delay(500);
    con(log, ".");
  }

  // (成功) -> 状態更新
  if (WiFi.status() == WL_CONNECTED) {
    g_WIFI_SSID = c.ssid;
    return true;
  }

  // (失敗) -> 切断して false
#if defined(ARDUINO_ARCH_ESP32)
    WiFi.disconnect(true, true);
#else
    WiFi.disconnect(true);     // RP2040(Earle core) は引数1つ
#endif
delay(200);
return false;
}

//─────────────
// WEB UI起動(STA)
//─────────────
void startSTA(
  Stream* log //
){
// → wifi搭載機の場合：
#if MMP_HAS_WIFI
  if (g_STARTED_WEBUI) {
    conln(log, "[INFO] WEB UI already running.");
    return;
  }

  g_STARTED_WEBUI = true; 
  g_WIFI_MODE     = "STA";

  webui_begin();
  conln(log, buildStartupStatusText());
#endif
}
//─────────────
// WEB UI起動(AP)
//─────────────
void startAP(
  Stream* log,      //
  const char* note  //
){
// → wifi搭載機の場合：
#if MMP_HAS_WIFI
  // → ノートがある場合：ログ出力
  if (note && *note) conln(log, note);

  // → APフォールバックが無効な場合：中断
  if (!WIFI.apfb.enabled) return;

  if (g_STARTED_WEBUI) {
    conln(log, "[INFO] WEB UI already running.");
    return;
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI.apfb.ssid.c_str(), WIFI.apfb.pass.length() ? WIFI.apfb.pass.c_str() : nullptr);

  g_STARTED_WEBUI = true; 
  g_WIFI_MODE     = "AP";
  g_WIFI_SSID     = WIFI.apfb.ssid;

  webui_begin();
  conln(log, buildStartupStatusText());
#endif
}

} // namespace MMP
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
