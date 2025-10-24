// filename : cliNetHttp.h
//========================================================
// クライアント：ネット(HTTP WEB API)
//--------------------------------------------------------
// Ver 0.6.0 (2025/xx/xx)
//========================================================
#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "modNET.h"

// 本体プログラム側で実装：wire("CMD[:a:b:c]!") を投げて "!!!!!"/"dddd!"/"#xxx!" を返す
// 統一入口（経路番号方式）
extern String MMP_REQUEST(const String& wire, int clientID);

// ===== 内部で WebServer を保持（新API用） =====
static WebServer* s_http = nullptr;

namespace {

// 経路番号: HTTP(Web API)
constexpr int kClientIdHttp = 3;

// ---- CORS / 送信ヘルパ ----
inline void add_cors(WebServer& srv) {
  srv.sendHeader("Access-Control-Allow-Origin", "*");
  srv.sendHeader("Access-Control-Allow-Methods", "GET,OPTIONS");
  srv.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Requested-With, Authorization");
  srv.sendHeader("Access-Control-Max-Age", "600");
}
inline void send_json(WebServer& srv, const String& s, int code=200) {
  add_cors(srv);
  srv.send(code, "application/json; charset=utf-8", s);
}
inline void send_204(WebServer& srv) { add_cors(srv); srv.send(204); }

// ---- エラーマップ（#xxx! 用）----
static const char* map_error(const String& resp){
  if (resp == "#CMD!") return "Error:コマンド名が不正"      ;
  if (resp == "#CHK!") return "Error:引数チェックで違反"    ;
  if (resp == "#INI!") return "Error:データが未初期化"      ;
  if (resp == "#DEV!") return "Error:使用不可のデバイス"    ;
  if (resp == "#FIL!") return "Error:ファイル操作が異常終了";
  if (resp == "#NOD!") return "Error:データ項目名が不正"    ;
  return "その他のエラー";
}

// ---- 符号付き4桁の十進判定／変換 ----
static bool isDec4Signed(const String& body){
  if (body.length() != 4) return false;
  int start = (body[0]=='-') ? 1 : 0;
  for (int i=start; i<4; ++i){
    if (!isDigit((unsigned char)body[i])) return false;
  }
  return true;
}
static int parseDec4Signed(const String& body){
  bool neg = (body[0]=='-');
  int v = 0;
  for (int i = neg ? 1 : 0; i < 4; ++i) v = v*10 + (body[i]-'0');
  return neg ? -v : v;
}

// -----------------------------
// ハンドラ本体（サーバ参照を引数で受ける汎用形）
// -----------------------------
void h_any_path_impl(WebServer& srv) {
  //bool  ok     = true     // MMPにまで届いたか  {OK:true | NG:false}
  bool    result = false;   // MMPの処理結果      {OK:true | NG:false}
  String  error  = "";      // エラーメッセージ   {正常の場合は空}
  int     value  = -1000;   // 戻値が数値の場合   {-999～9999、対象外は-1000 }
  String  text   = "";      // 戻値が文字列の場合 {４バイトの文字列、対象外は空}

  // 受信URI
  String uri = srv.uri();

  // ルート "/" は固定レスポンス
  if (uri.isEmpty() || uri == "/") {
    send_json(srv, F("{"
        "\"ok\":true,"
        "\"result\":true,"  
        "\"error\":\"\","
        "\"value\":-1,"     
        "\"text\":\"\""
        "}"));
    return;
  }

  // 先頭の '/' を除去 → wire 組み立て（末尾'!'付与）
  String path = uri;
  if (path.length() && path[0] == '/') path.remove(0, 1);
  if (!path.endsWith("!")) path += '!';

  // コマンドパーサーへリクエスト（HTTP=3）
  String resp = MMP_REQUEST(path, kClientIdHttp);

  //◇コマンドパーサーの戻り値に応じてレスポンス
  if (resp.length() < 1 || resp[resp.length()-1] != '!') {
  // → 文字欠け
    result = false;
    error  = "MMPからの戻値が異常";

  } else if (resp == "!!!!!") {
  // → 実行結果OK
    result = true;
    error  = "";

  } else if (resp.length() == 5 && resp[0] == '#') {
  // → #xxx! エラー
    result = false;
    error  = map_error(resp);

  } else {
  // → 本文（末尾 '!' を除く）
    String body = resp.substring(0, resp.length()-1);
    if (isDec4Signed(body)) {
    // → 4文字の符号付き十進 → 数値
      result = true;
      error  = "";
      value  = parseDec4Signed(body);

    // → それ以外：文字列
    } else {
      result = true;
      error  = "";
      text   = body;
    }
  }

  // JSON 組み立て
  String js;
  js.reserve(160);
  js += F("{\"ok\":true"  );
  js += F(",\"result\":"  ); js += (result ? "true" : "false");
  js += F(",\"error\":\"" ); js += error; js += '"'           ;
  js += F(",\"value\":"   ); js += String(value)              ;
  js += F(",\"text\":\""  ); js += text                       ;
  js += "\"}"              ;
  send_json(srv, js);
}

// ルート（簡易疎通）
void h_root_impl(WebServer& srv){
  send_json(srv, F("{"
    "\"ok\":true,"
    "\"result\":true,"
    "\"error\":\"\","
    "\"value\":-1,"
    "\"text\":\"MMP WEB API\""
    "}"));
}

// OPTIONS helper
void allow_options(WebServer& srv, const char* path) {
  srv.on(path, HTTP_OPTIONS, [&srv](){ send_204(srv); });
}

// ルーティングをインストール（共通）
void install_routes(WebServer& server){
  server.on("/", HTTP_GET, [&server](){ h_root_impl(server); });
  allow_options(server, "/");

  // 任意コマンド（全てここに集約）
  server.onNotFound([&server](){
    if (server.method() == HTTP_OPTIONS) { send_204(server); return; }
    h_any_path_impl(server);
  });
}

} // namespace
//========================================================


//========================================================
namespace srvHttp {
bool start(uint16_t port) {
  if (s_http) return true;             // 二重起動防止
  s_http = new WebServer(port);
  install_routes(*s_http);
  s_http->begin();
  return true;
}
void handle() {if (s_http) s_http->handleClient();}

//================ 既存API（互換） ==================
void begin (WebServer& server) {install_routes(server);}
void handle(WebServer& server) {server.handleClient();}

} // namespace srvHttp
//========================================================
