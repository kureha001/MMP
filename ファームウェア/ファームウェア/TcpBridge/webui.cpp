#include "webui.hpp"
#include "config.hpp"
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>

extern String g_wifi_mode;
extern String g_wifi_ssid;

static WebServer http(80);

/**
 * @brief /status & シリアル用の起動情報テキストを生成
 * @return 表示テキスト（Mode/SSID/IP と各UART情報）
 */
String buildStartupStatusText(){
  String s; s.reserve(256);
  s += "Mode="; s += g_wifi_mode; s += "  SSID="; s += g_wifi_ssid; s += "\n";
  s += "IP="; s += (g_wifi_mode=="STA" ? WiFi.localIP().toString() : WiFi.softAPIP().toString()); s += "\n";
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
  http.setContentLength(CONTENT_LENGTH_UNKNOWN);
  http.send(200,"text/html",
    "<h3>M5Stamp S3 Config Upload</h3>"
    "<form method='POST' action='/upload' enctype='multipart/form-data'>"
    "<input type='file' name='f'><button type='submit'>Upload</button>"
    "</form><p><a href='/config'>View current config.json</a> | <a href='/status'>Status</a></p>");
}

/**
 * @brief /config: 現在の /config.json をそのまま返す
 * - ない場合は 404
 */
static void handleViewConfig(){
  if (!LittleFS.begin(true)) { http.send(500,"text/plain","LittleFS error"); return; }
  if (!LittleFS.exists("/config.json")) { http.send(404,"text/plain","/config.json not found"); return; }
  File f = LittleFS.open("/config.json","r");
  http.streamFile(f, "application/json");
  f.close();
}

/**
 * @brief /upload(POST): 受信ファイルを /config.json として保存→再起動
 * - PC側のファイル名に関係なく /config.json で保存する
 */
static void handleUpload(){
  HTTPUpload& up = http.upload();
  static File uf;
  if (up.status == UPLOAD_FILE_START){
    if (!LittleFS.begin(true)) { http.send(500,"text/plain","LittleFS error"); return; }
    uf = LittleFS.open("/config.json","w"); // ★ 常に /config.json に保存
  } else if (up.status == UPLOAD_FILE_WRITE){
    if (uf) uf.write(up.buf, up.currentSize);
  } else if (up.status == UPLOAD_FILE_END){
    if (uf) uf.close();
    http.send(200,"text/plain","Uploaded. Rebooting...");
    delay(500);
    ESP.restart();
  }
}

/**
 * @brief /status: 現在の起動情報を text/plain で返す
 */
static void handleStatus(){
  http.send(200,"text/plain", buildStartupStatusText());
}

/**
 * @brief Web サーバ開始＆ルーティング登録
 */
void webui_begin(){
  http.on("/", HTTP_GET, handleRoot);
  http.on("/config", HTTP_GET, handleViewConfig);
  http.on("/upload", HTTP_POST, [](){}, handleUpload);
  http.on("/status", HTTP_GET, handleStatus);
  http.begin();
  Serial.println("HTTP ready: /  /config  /upload  /status");
}

/**
 * @brief ループ内でHTTP処理を回す
 */
void webui_handle(){
  http.handleClient();
}
