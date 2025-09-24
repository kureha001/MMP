// TcpBridge.ino
// =============================================================
// M5Stamp S3 : 2x UART <-> TCP Bridge (RAW)
// Split project (config / webui / bridge modules)
// - UART1 -> TCP:5331  (RX=G3,  TX=G1)
// - UART2 -> TCP:5332  (RX=G10, TX=G9)
// - LittleFS /config.json (Wi-Fi candidates, UART, server)
// - Web UI: / (upload), /upload (POST), /config (view), /status (text status)
// =============================================================

#include <WiFi.h>
#include <WebServer.h>
#include "config.hpp"
#include "bridge.hpp"
#include "webui.hpp"
#include "WebApiCore.h"

// ---- Wi-Fi ランタイム状態（/status とシリアル表示で使用） ----
String g_wifi_mode;   // "STA" もしくは "AP"
String g_wifi_ssid;   // 接続中の SSID / AP の SSID

WebServer httpApi(8080);

//==============================================================
// Wi-Fi 補助関数（宣言）
//==============================================================

/**
 * @brief 単一候補（SSID/パスワード/タイムアウト）で STA 接続を試行する
 * @param c 接続候補（ssid, pass, timeout_ms）
 * @return true: 接続成功 / false: 失敗（タイムアウト・認証エラーなど）
 *
 * 処理の流れ:
 * - WIFI_STA に設定し、WiFi.begin() を呼ぶ
 * - 指定の timeout_ms の間 WL_CONNECTED を待つ
 * - 成功→グローバル g_wifi_mode/g_wifi_ssid を更新
 * - 失敗→WiFi.disconnect(true,true) で状態をクリア
 */
bool tryConnectOne(const WifiCand& c);

/**
 * @brief すべての候補に失敗した場合の AP フォールバック開始
 *
 * 処理の流れ:
 * - WIFI_AP に切替、softAP(ssid, pass) を起動
 * - g_wifi_mode/g_wifi_ssid を AP 情報で更新
 */
void startAPFallback();

//==============================================================
// Arduino 標準エントリ
//==============================================================

/**
 * @brief 初期化手順
 * - 設定ロード（/config.json：無ければ最小構成を自動生成）
 * - UART 設定の適用
 * - Wi-Fi 候補を順に試行→全滅なら AP フォールバック
 * - Web サーバ起動
 * - UART<->TCP ブリッジの起動
 * - 起動ステータスをシリアル出力
 */
void setup(){
  Serial.begin(115200);

  if (!loadConfig()) {
    Serial.println("Config load failed (LittleFS?). Continuing with defaults.");
  }
  applyUartConfig();

  // ---- Wi-Fi: 候補を順に試行 ----
  bool ok = false;
  for (int i=0; i<WIFI.candN && !ok; i++){
    Serial.printf("WiFi try %d/%d: SSID=%s timeout=%lums\n",
      i+1, WIFI.candN, WIFI.cand[i].ssid.c_str(), (unsigned long)WIFI.cand[i].timeout_ms);
    ok = tryConnectOne(WIFI.cand[i]);
  }

  if (!ok){
    Serial.println("WiFi all candidates failed.");
    if (WIFI.apfb.enabled){
      startAPFallback();
      Serial.printf("AP mode: SSID=%s  IP=%s\n",
        g_wifi_ssid.c_str(), WiFi.softAPIP().toString().c_str());
    } else {
      // ★ APフォールバック無効の場合：STAのままIP無しで継続
      g_wifi_mode = "STA"; g_wifi_ssid = "(disconnected)";
    }
  } else {
    Serial.printf("WiFi connected: SSID=%s  IP=%s\n",
      g_wifi_ssid.c_str(), WiFi.localIP().toString().c_str());
  }

  // Web UI 起動
  webui_begin();

  // WebAPI 起動
  WebApiCore::begin(httpApi);
  httpApi.begin();

  // UART<->TCP ブリッジ開始
  for (int i=0;i<NUM_UARTS;i++){
    ctxs[i] = new PortCtx(UARTS[i].tcpPort, SRV.maxClients);
    ctxs[i]->server.begin();
    ctxs[i]->server.setNoDelay(true);
    ctxs[i]->tcp2uart.init(4096);
    ctxs[i]->uart2tcp.init(8192);
    Serial.printf("UART%d -> TCP:%u (TX=G%d RX=G%d, %lubps)\n",
      (UARTS[i].port==&Serial1)?1:2, UARTS[i].tcpPort,
      UARTS[i].tx, UARTS[i].rx, (unsigned long)UARTS[i].baud);
  }

  // 起動ステータス（/status と同じテキスト）
  Serial.println(buildStartupStatusText());
}

/**
 * @brief メインループ
 * - Web サーバのリクエスト処理
 * - 各ポートでのクライアント管理／データ中継／ロック期限監視
 */
void loop(){
  webui_handle();           // WEB-UI
  httpApi.handleClient();   // WEB-API
  for (int i=0;i<NUM_UARTS;i++){
    acceptClients(ctxs[i]);                 // 新規接続受入／切断掃除／ロック期限解放
    pumpTCPtoRing(ctxs[i]);                 // TCP→リング
    pumpUARTtoRing(ctxs[i], UARTS[i].port); // UART→リング
    flushRingToUART(ctxs[i], UARTS[i].port);// リング→UART
    flushRingToTCP(ctxs[i]);                // リング→全TCPクライアント
  }
}

//==============================================================
// Wi-Fi 補助関数（実装）
//==============================================================

bool tryConnectOne(const WifiCand& c){
  if (c.ssid.isEmpty()) return false;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(WIFI.hostname.c_str());
  WiFi.begin(c.ssid.c_str(), c.pass.c_str());

  // 接続完了待ち（簡易待機）
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis()-t0) < c.timeout_ms) {
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED) {
    // ★ 成功：状態更新
    g_wifi_mode = "STA";
    g_wifi_ssid = c.ssid;
    return true;
  }

  // ★ 失敗：クリーンに切断して次候補
  WiFi.disconnect(true, true);
  delay(200);
  return false;
}

void startAPFallback(){
  if (!WIFI.apfb.enabled) return;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI.apfb.ssid.c_str(), WIFI.apfb.pass.length() ? WIFI.apfb.pass.c_str() : nullptr);
  g_wifi_mode = "AP";
  g_wifi_ssid = WIFI.apfb.ssid;
}
