// TcpBridge.ino
#include <WiFi.h>
#include <WebServer.h>
#include "config.hpp"
#include "webui.hpp"

// Wi-Fi ランタイム状態（/status とシリアル表示で使用）
String g_wifi_mode;   // "STA" もしくは "AP"
String g_wifi_ssid;   // 接続中の SSID / AP の SSID

// Wi-Fi候補1つを試行
bool tryConnectOne(const WifiCand& c);

// APフォールバック起動
void startAPFallback();


//-----------------------
// セットアップ
//-----------------------
void setup(){

  Serial.begin(115200);

  // 設定読み込み 
  if (!loadConfig()) {
    Serial.println("Config load failed (LittleFS?). Continuing with defaults.");
  }

  // Wi-Fi: 候補を順に試行
  // (すべて失敗) -> AP フォールバック
  bool ok = false;
  for (int i=0; i<WIFI.candN && !ok; i++){
    Serial.printf("WiFi try %d/%d: SSID=%s timeout=%lums\n",
      i+1, WIFI.candN, WIFI.cand[i].ssid.c_str(), (unsigned long)WIFI.cand[i].timeout_ms);
    ok = tryConnectOne(WIFI.cand[i]);
  }

  // ★試行結果で処理分岐：
  // (すべて失敗) -> AP フォールバック
  if (!ok){
    Serial.println("WiFi all candidates failed.");

    // APフォールバック
    if (WIFI.apfb.enabled){
      startAPFallback();
      Serial.printf("AP mode: SSID=%s  IP=%s\n",
        g_wifi_ssid.c_str(), WiFi.softAPIP().toString().c_str());

    // STAのままIP無しで継続
    } else {
      g_wifi_mode = "STA"; g_wifi_ssid = "(disconnected)";
    }

  // (STA接続成功) -> 状態更新
  } else {
    Serial.printf(
      "WiFi connected: SSID=%s  IP=%s\n",
      g_wifi_ssid.c_str(),
      WiFi.localIP().toString().c_str()
    );
  }

  // WebUI起動・起動ステータス表示
  webui_begin();
  Serial.println(buildStartupStatusText());
}


//-----------------------
// ループ
//-----------------------
void loop(){
  webui_handle(); // ループ内で呼び出す HTTP 処理
}


//-----------------------
// Wi-Fi候補1つを試行
//-----------------------
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
    WiFi.disconnect(true, true);
    delay(200);
    return false;
  }


//-----------------------
// APフォールバック起動
//-----------------------
void startAPFallback(){
  if (!WIFI.apfb.enabled) return;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI.apfb.ssid.c_str(), WIFI.apfb.pass.length() ? WIFI.apfb.pass.c_str() : nullptr);
  g_wifi_mode = "AP";
  g_wifi_ssid = WIFI.apfb.ssid;
}
