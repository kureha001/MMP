// WebUI.cpp
#include "WebUI.h"
#include "config.h"
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>

extern String g_WIFI_MODE;      // "STA" もしくは "AP"
extern String g_WIFI_SSID;      // 接続中の SSID / AP の SSID
static WebServer g_HTTP(80);    // このHTTPサーバ

/**
 * @brief /status & シリアル用の起動情報テキストを生成
 * @return 表示テキスト（Mode/SSID/IP と各UART情報）
 */
String StatusText(){
  String s; s.reserve(256);
  s += "Mode="; s += g_WIFI_MODE; s += "  SSID="; s += g_WIFI_SSID; s += "\n";
  s += "IP="; s += (g_WIFI_MODE=="STA" ? WiFi.localIP().toString() : WiFi.softAPIP().toString()); s += "\n";
  for (int i=0;i<NUM_UARTS;i++){
    s += "UART"; s += (UARTS[i].port==&Serial1)? "1":"2";
    s += " -> TCP:"; s += String(UARTS[i].tcpPort);
    s += " (TX=G"; s += String(UARTS[i].tx);
    s += " RX=G"; s += String(UARTS[i].rx);
    s += ", "; s += String(UARTS[i].baud); s += "bps)\n";
  }
  return s;
}

/**
 * @brief ルート: 設定アップロード用の簡易HTMLを返す
 */
static void handleRoot(){
  g_HTTP.setContentLength(CONTENT_LENGTH_UNKNOWN);
  g_HTTP.send(200, "text/html; charset=utf-8",
    "<!doctype html><meta charset='utf-8'>"
    "<h3>ＴＣＰ ブリッジ サーバー 設定画面</h3>"
    "<form method='POST' action='/upload' enctype='multipart/form-data'>"
    "<input type='file' name='f'><button type='submit'>取り込み開始</button>"
    "</form><p><a href='/config'>現在の設定ファイルの中身</a> | <a href='/status'>現在の接続状況</a></p>");
}

/**
 * @brief /config: 現在の /config.json をそのまま返す
 * - ない場合は 404
 */
static void handleViewConfig(){
  if (!LittleFS.begin(true)) { g_HTTP.send(500,"text/plain","LittleFS error"); return; }
  if (!LittleFS.exists("/config.json")) { g_HTTP.send(404,"text/plain","/config.json not found"); return; }
  File f = LittleFS.open("/config.json","r");
  g_HTTP.streamFile(f, "application/json");
  f.close();
}

/**
 * @brief /upload(POST): 受信ファイルを /config.json として保存→再起動
 * - PC側のファイル名に関係なく /config.json で保存する
 */
static void handleUpload(){
  HTTPUpload& up = g_HTTP.upload();
  static File uf;
  if (up.status == UPLOAD_FILE_START){
    if (!LittleFS.begin(true)) { g_HTTP.send(500,"text/plain","LittleFS error"); return; }
    uf = LittleFS.open("/config.json","w"); // ★ 常に /config.json に保存
  } else if (up.status == UPLOAD_FILE_WRITE){
    if (uf) uf.write(up.buf, up.currentSize);
  } else if (up.status == UPLOAD_FILE_END){
    if (uf) uf.close();
    g_HTTP.send(200,"text/plain","Uploaded. Rebooting...");
    delay(500);
    ESP.restart();
  }
}

/**
 * @brief /status: 現在の起動情報を text/plain で返す
 */
static void handleStatus(){
  g_HTTP.send(200,"text/plain", StatusText());
}

/**
 * @brief Web サーバ開始＆ルーティング登録
 */
void webui_begin(){
  g_HTTP.on("/", HTTP_GET, handleRoot);
  g_HTTP.on("/config", HTTP_GET, handleViewConfig);
  g_HTTP.on("/upload", HTTP_POST, [](){}, handleUpload);
  g_HTTP.on("/status", HTTP_GET, handleStatus);
  g_HTTP.begin();
  Serial.println("HTTP ready: /  /config  /upload  /status");
}

/**
 * @brief ループ内でHTTP処理を回す
 */
void webui_handle(){
  g_HTTP.handleClient();
}
