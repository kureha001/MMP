// filename : webui.cpp
//============================================================
// ＷＥＢサーバーＵＩ
// バージョン：0.5
//============================================================
#include "webui.hpp"
#include "config.hpp"
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>

extern String g_WIFI_MODE;  // "STA" もしくは "AP"
extern String g_WIFI_SSID;  // 接続中の SSID / AP の SSID
static WebServer g_HTTP(80);  // HTTPサーバ(ポート)

//━━━━━━━━━━━━━━━
// 起動情報テキストを生成
//━━━━━━━━━━━━━━━
String buildStartupStatusText(){
  String s;
  s.reserve(128);
  s += "[WEB UI]\n";
  s += " Mode: " + String(g_WIFI_MODE) + "\n";
  s += " SSID: " + String(g_WIFI_SSID) + "\n";
  s += " IP  : " + String(g_WIFI_MODE=="STA" ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
  return s;
}

//━━━━━━━━━━━━━━━
// ルート: 設定アップロード画面
//━━━━━━━━━━━━━━━
static void handleRoot(){
  g_HTTP.setContentLength(CONTENT_LENGTH_UNKNOWN);
  g_HTTP.send(200, "text/html; charset=utf-8",
    "<!doctype html><meta charset='utf-8'>"
    "<h3>クライアントWifi設定画面</h3>"
    "<form method='POST' action='/upload' enctype='multipart/form-data'>"
    "<input type='file' name='f'><button type='submit'>取り込み開始</button>"
    "</form><p><a href='/config'>現在の設定ファイルの中身</a> | <a href='/status'>現在の接続状況</a></p>");
}

//━━━━━━━━━━━━━━━
// /config.json をそのまま返す
// ない場合は 404
//━━━━━━━━━━━━━━━
static void handleViewConfig(){

  // ファイル操作を開始
  #if defined(ARDUINO_ARCH_RP2040)
    if (!LittleFS.begin())
  #else
    if (!LittleFS.begin(true))
  #endif
  { g_HTTP.send(500,"text/plain","LittleFS mount error"); return; }

  // ファイル存在を確認
  if (!LittleFS.exists("/config.json"))
  { g_HTTP.send(404,"text/plain","/config.json not found"); return; }

  // ファイルを開く
  File f = LittleFS.open("/config.json","r");
  if (!f) { g_HTTP.send(500,"text/plain","open failed"); return; }
  g_HTTP.streamFile(f, "application/json");   // ファイルを表示
  f.close();                                // ファイルを閉じる
}

//━━━━━━━━━━━━━━━
// 受信ファイルを /config.json
// として保存→再起動
//━━━━━━━━━━━━━━━
static void handleUpload(){

  HTTPUpload& up = g_HTTP.upload();   // アップロード状態
  static File uf;                   // アップロード中のファイル

  // → アップロード開始の場合：
  if (up.status == UPLOAD_FILE_START){

    // ファイル操作を開始
    #if defined(ARDUINO_ARCH_RP2040)
      if (!LittleFS.begin())
    #else
      if (!LittleFS.begin(true))
    #endif
    { g_HTTP.send(500,"text/plain","LittleFS mount error"); return; }

    // ファイルを開く
    uf = LittleFS.open("/config.json","w");

  // → アップロード中の場合：
  } else if (up.status == UPLOAD_FILE_WRITE){
    // ファイルを書き込む
    if (uf) uf.write(up.buf, up.currentSize);

  // → アップロード終了の場合：
  } else if (up.status == UPLOAD_FILE_END){
    // ファイルを閉じる
    if (uf) uf.close();

    // 再起動メッセージをしばらく表示
    g_HTTP.send(200,"text/plain","Uploaded. Rebooting...");
    delay(500);

    // 再起動する
    #if defined(ARDUINO_ARCH_RP2040)
      rp2040.reboot();
    #else
      ESP.restart();
    #endif
  }
}

//━━━━━━━━━━━━━━━
// 起動情報を text/plain で返す
//━━━━━━━━━━━━━━━
static void handleStatus(){g_HTTP.send(200,"text/plain", buildStartupStatusText());}

//━━━━━━━━━━━━━━━
// Webサーバ開始＆ルーティング登録
//━━━━━━━━━━━━━━━
void webui_begin(){
  g_HTTP.on("/", HTTP_GET, handleRoot);
  g_HTTP.on("/config", HTTP_GET, handleViewConfig);
  g_HTTP.on("/upload", HTTP_POST, [](){}, handleUpload);
  g_HTTP.on("/status", HTTP_GET, handleStatus);
  g_HTTP.begin();
}

//━━━━━━━━━━━━━━━
// ループ内でHTTP処理を回す
//━━━━━━━━━━━━━━━
void webui_handle(){g_HTTP.handleClient();}
