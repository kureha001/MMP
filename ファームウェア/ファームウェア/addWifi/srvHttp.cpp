// srvHttp.cpp
// サーバー：WEB API

#include "srvHttp.h"
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "config.h"

// 本体プログラム側で実装：wire("CMD[:a:b:c]!") を投げて "!!!!!"/"dddd!"/"#xxx!" を返す
extern String MMP_REQUEST(const String& wire);

// ===== 内部で WebServer を保持（新API用） =====
static WebServer* s_http = nullptr;

namespace {

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
  if (resp == "#CMD!") return "Error:コマンド名不正";
  if (resp == "#CHK!") return "Error:引数チェック違反";
  if (resp == "#INI!") return "Error:未初期化データ";
  if (resp == "#DEV!") return "Error:使用不可デバイス";
  return "unknown error";
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
  String  error  = "";
  bool    result = false;
  int     value  = -1;
  String  text   = "";

  // 受信URI
  String uri = srv.uri();

  // ルート "/" は固定レスポンス
  if (uri.isEmpty() || uri == "/") {
    send_json(srv, F("{\"ok\":true,\"error\":\"\",\"result\":true,\"value\":-1,\"text\":\"\"}"));
    return;
  }

  // 先頭の '/' を除去 → wire 組み立て（末尾'!'付与）
  String path = uri;
  if (path.length() && path[0] == '/') path.remove(0, 1);
  if (!path.endsWith("!")) path += '!';

  // ★ＭＭＰファームウェアにリクエスト
  String resp = MMP_REQUEST(path);

  // ★返答の「形」で判定
  if (resp.length() < 1 || resp[resp.length()-1] != '!') {
    error  = "timeout or bad reply";
    result = false;
  } else if (resp == "!!!!!") {
    // OK
    error  = "";
    result = true;
  } else if (resp.length() == 5 && resp[0] == '#') {
    // #xxx! エラー
    error  = map_error(resp);
    result = false;
  } else {
    // 本文（末尾 '!' を除く）
    String body = resp.substring(0, resp.length()-1);
    if (isDec4Signed(body)) {
      // 4文字の符号付き十進 → 数値
      value  = parseDec4Signed(body);
      error  = "";
      result = true;
    } else {
      // それ以外は文字列
      text   = body;
      error  = "";
      result = true;
    }
  }

  // JSON 組み立て
  String js;
  js.reserve(160);
  js += F("{\"ok\":true");
  js += F(",\"error\":\""); js += error; js += '"';
  js += F(",\"result\":");   js += (result ? "true" : "false");
  js += F(",\"value\":");    js += String(value);
  js += F(",\"text\":\"");   js += text;  js += "\"}";
  send_json(srv, js);
}

// ルート（簡易疎通）
void h_root_impl(WebServer& srv){
  send_json(srv, F("{\"ok\":true,\"service\":\"MMP-WebAPI\",\"note\":\"GET-only\"}"));
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


namespace srvHttp {

//================ 新API：内部保持 ==================
bool start(uint16_t port) {
  if (s_http) return true;             // 二重起動防止
  s_http = new WebServer(port);
  install_routes(*s_http);
  s_http->begin();
  return true;
}
void handle() {
  if (s_http) s_http->handleClient();
}

//================ 既存API（互換） ==================
void begin(WebServer& server) {
  install_routes(server);
}
void handle(WebServer& server) {
  server.handleClient();
}

} // namespace srvHttp
