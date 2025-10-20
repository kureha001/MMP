// TcpBridge.ino
// =============================================================
// ＭＭＰ ＴＣＰブリッジ＆ＷＥＢ－ＡＰＩ サーバ
// バージョン：0.5
//------------------------------------------------------------
// M5Stamp S3 : 2x UART
// ボード設定：Flash Size 4MB(32Mb)
// - UART1 -> TCP:5331  (RX=G3,  TX=G1)
// - UART2 -> TCP:5332  (RX=G10, TX=G9)
//------------------------------------------------------------
// ラズパイPico: 2x UART
// ボード設定：Flash Size 4MB(FS:64KB)
// - UART1 -> TCP:5331  (RX=0, TX=1)
// - UART2 -> TCP:5332  (RX=4, TX=5)
// =============================================================
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"
#include "bridge.h"
#include "WebUI.h"
#include "WebApiCore.h"

// ---- Wi-Fi ランタイム状態（/status とシリアル表示で使用） ----
bool    g_STARTED_WEBUI = false; // WebUIを既に起動したか
String  g_WIFI_MODE;   // "STA" もしくは "AP"
String  g_WIFI_SSID;   // 接続中の SSID / AP の SSID

WebServer g_WEB_API(8080);

//==============================================================
// Wi-Fi 補助関数（宣言）
//==============================================================
//─────────────
// Wi-Fi候補1つを試行
//─────────────
bool tryConnectOne(const WifiCand& c);

//─────────────
// WEB UI起動(STA)
//─────────────
void startSTA(){
  g_STARTED_WEBUI = true; 
  g_WIFI_MODE     = "STA";
  webui_begin();
  Serial.println(buildStartupStatusText());
}

//─────────────
// WEB UI起動(AP)
//─────────────
void startAP(){

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI.apfb.ssid.c_str(), WIFI.apfb.pass.length() ? WIFI.apfb.pass.c_str() : nullptr);

  g_STARTED_WEBUI = true; 
  g_WIFI_MODE     = "AP";
  g_WIFI_SSID     = WIFI.apfb.ssid;

  webui_begin();
  Serial.println(buildStartupStatusText());
  Serial.println("\n");
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
  applyUartConfig();

  // Wi-Fi: 候補を順に試行：
  bool ok = false;
  for (int i=0; i<WIFI.candN && !ok; i++){
    Serial.printf("WiFi try %d/%d: SSID=%s timeout=%lums\n",
      i+1, WIFI.candN, WIFI.cand[i].ssid.c_str(), (unsigned long)WIFI.cand[i].timeout_ms);
    ok = tryConnectOne(WIFI.cand[i]);
  }

  Serial.println("=======================");

  // (すべて失敗) -> AP フォールバック
  if (!ok){
    // WebUI(APモード)起動
    Serial.println("[ERROR] 設定内容の接続に失敗");
    startAP();
  } else {
    // WebUI(STAモード)起動
    startSTA();
  }

  // WebAPIサーバーを起動
  Serial.println("-----------------------");
  WebApiCore::begin(g_WEB_API);
  g_WEB_API.begin();

  // TCPブリッジ サーバーを起動
  Serial.println("-----------------------");
  Serial.println("[TCP<->UART Bridge]");
  for (int i=0;i<MMP_NUM_UARTS;i++){
    ctxs[i] = new PortCtx(UARTS[i].tcpPort, SRV.maxClients);
    ctxs[i]->server.begin();
    ctxs[i]->server.setNoDelay(true);
    ctxs[i]->tcp2uart.init(4096);
    ctxs[i]->uart2tcp.init(8192);
    Serial.println(" - - - - - - - - - - -");
    Serial.printf (" TCP PORT  : %u\n"    , UARTS[i].tcpPort                );
    Serial.printf ("  UART     : %d\n"    , UARTS[i].port == &Serial1 ?1 : 2);
    Serial.printf ("  GPIO(Tx) : %d\n"    , UARTS[i].tx                     );
    Serial.printf ("      (Rx) : %d\n"    , UARTS[i].rx                     );
    Serial.printf ("  Baud Rate: %lubps\n", (unsigned long)UARTS[i].baud    );
  }
  Serial.println("=======================");
}

//━━━━━━━━━━━━━━━
// ルーティング
//━━━━━━━━━━━━━━━
void loop(){
  webui_handle();           // WEB-UI
  g_WEB_API.handleClient();   // WEB-API
  for (int i=0;i<MMP_NUM_UARTS;i++){
    acceptClients(ctxs[i]);                 // 新規接続受入／切断掃除／ロック期限解放
    pumpTCPtoRing(ctxs[i]);                 // TCP→リング
    pumpUARTtoRing(ctxs[i], UARTS[i].port); // UART→リング
    flushRingToUART(ctxs[i], UARTS[i].port);// リング→UART
    flushRingToTCP(ctxs[i]);                // リング→全TCPクライアント
  }
}

//━━━━━━━━━━━━━━━
// 接続系ヘルパー
//━━━━━━━━━━━━━━━
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

  // (成功) -> 状態更新
  if (WiFi.status() == WL_CONNECTED) {
    g_WIFI_SSID = c.ssid;
    return true;
  }

  // (失敗) -> 切断して false
  #if defined(ARDUINO_ARCH_RP2040)
    WiFi.disconnect(true);
  #else
    WiFi.disconnect(true, true);
  #endif
 
  delay(200);
  return false;
}