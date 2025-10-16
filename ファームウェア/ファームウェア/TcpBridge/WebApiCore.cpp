// WebApiCore.cpp
#include "WebApiCore.h"
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

// ---- UART 側（bridge で初期化済み） ----
#include "config.hpp"  // UARTS[0] を使う

namespace {

// -----------------------------
// レスポンス種別
// -----------------------------
enum RespKind {
  RK_RESULT,  // "!!!!!" を OK とみなす
  RK_VALUE,   // "dddd!"（10進、-100～4095 を想定）を数値化
  RK_STRING   // "XXXX!" をテキストとして返却
};

// コマンドと種別の対応（前方一致ではなく完全一致）
struct CmdSpec {
  const char* cmd;
  RespKind    kind;
};

// ★頻出コマンドを先頭に（総当たり O(N)）
static const CmdSpec kSpecs[] = {
  // PWM（頻出想定）
  { "PWM/OUTPUT",        RK_RESULT },
  { "PWM/INFO/CONNECT",  RK_VALUE  },
  { "PWM/ANGLE/SETUP",   RK_RESULT },
  { "PWM/ANGLE/DELETE",  RK_RESULT },
  { "PWM/ANGLE/OUTPUT",  RK_VALUE  },
  { "PWM/ANGLE/CENTER",  RK_VALUE  },
  { "PWM/ROTATE/SETUP",  RK_RESULT },
  { "PWM/ROTATE/DELETE", RK_RESULT },
  { "PWM/ROTATE/OUTPUT", RK_VALUE  },
  { "PWM/ROTATE/STOP",   RK_RESULT },

  // INFO
  { "INFO/VERSION",      RK_STRING },

  // ANALOG
  { "ANALOG/SETUP",      RK_RESULT },
  { "ANALOG/INPUT",      RK_RESULT },
  { "ANALOG/READ",       RK_VALUE  },

  // DIGITAL
  { "DIGITAL/INPUT",     RK_VALUE  },
  { "DIGITAL/OUTPUT",    RK_RESULT },

  // I2C
  { "I2C/WRITE",         RK_RESULT },
  { "I2C/READ",          RK_VALUE  },

  // MP3
  { "MP3/SET/VOLUME",    RK_RESULT },
  { "MP3/SET/EQ",        RK_RESULT },
  { "MP3/TRACK/PLAY",    RK_VALUE  },
  { "MP3/TRACK/LOOP",    RK_VALUE  },
  { "MP3/TRACK/STOP",    RK_VALUE  },
  { "MP3/TRACK/PAUSE",   RK_VALUE  },
  { "MP3/TRACK/START",   RK_VALUE  },
  { "MP3/INFO/CONNECT",  RK_VALUE  },
  { "MP3/INFO/TRACK",    RK_VALUE  },
  { "MP3/INFO/VOLUME",   RK_VALUE  },
  { "MP3/INFO/EQ",       RK_VALUE  },
  { "MP3/INFO/FILEID",   RK_VALUE  },
  { "MP3/INFO/FILES",    RK_VALUE  },
};

static WebServer* g_srv = nullptr;

// ---- CORS ----
inline void add_cors() {
  if (!g_srv) return;
  g_srv->sendHeader("Access-Control-Allow-Origin", "*");
  g_srv->sendHeader("Access-Control-Allow-Methods", "GET,OPTIONS");
  g_srv->sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Requested-With, Authorization");
  g_srv->sendHeader("Access-Control-Max-Age", "600");
}
inline void send_json(const String& s, int code=200) {
  add_cors();
  g_srv->send(code, "application/json", s);
}
inline void send_err(const String& msg, int code=400) {
  add_cors();
  g_srv->send(code, "application/json", String("{\"ok\":false,\"error\":\"")+msg+"\"}");
}
inline void send_204() { add_cors(); g_srv->send(204); }

// -----------------------------
// ユーティリティ
// -----------------------------
// URI 先頭の "/" を取り除いたコマンド名を得る
String get_command_from_uri(const String& uri){
  if (uri.length()==0) return String();
  if (uri[0]=='/') return uri.substring(1);
  return uri;
}

// コマンド種別の引当て（未登録は nullptr）
const CmdSpec* find_spec(const String& cmd) {
  for (size_t i=0;i<sizeof(kSpecs)/sizeof(kSpecs[0]);++i){
    if (cmd.equalsIgnoreCase(kSpecs[i].cmd)) return &kSpecs[i];
  }
  return nullptr;
}

// 5バイト応答を読み切る（末尾 '!' まで保持、先頭5文字だけ確保）
// 戻り値: 取得した 5 文字（例 "4095!" / "-100!" / "!!!!!" / "#CMD!"）
String read_reply_5(HardwareSerial* U, uint32_t timeout_ms){
  String buf;
  unsigned long deadline = millis() + timeout_ms;
  while ((long)(deadline - millis()) > 0) {
    while (U->available() > 0) {
      char c = (char)U->read();
      if (c=='\r' || c=='\n') continue;
      buf += c;
      if (buf.length() > 5) buf.remove(0, buf.length()-5);
      if (buf.length()==5 && buf[4]=='!') return buf;
    }
    delay(1);
  }
  return String(); // タイムアウト
}

// 10進数化（4桁＋先頭のみ '-' 許可）。成功なら true
bool parse_dec4(const String& s4, int& out) {
  if (s4.length()!=4) return false;
  // 先頭が '-' なら残り 3 桁は 0-9、そうでなければ全 4 桁が 0-9
  int start = 0;
  bool neg = false;
  if (s4[0]=='-') { neg=true; start=1; }
  for (int i=start;i<4;i++){
    if (s4[i]<'0' || s4[i]>'9') return false;
  }
  long v = 0;
  for (int i=start;i<4;i++){
    v = v*10 + (s4[i]-'0');
  }
  if (neg) v = -v;
  // 想定範囲（-100～4095）チェックは厳密には不要だが、参考で一応絞る
  if (v < -100 || v > 4095) {
    // 仕様上この範囲外は来ない前提だが、来たら parse 失敗扱い
    return false;
  }
  out = (int)v;
  return true;
}

// エラーコード (#xxx!) をメッセージに
const char* map_error(const String& five){
  if (five == "#CMD!") return "command not found";
  if (five == "#CHK!") return "argument check failed";
  if (five == "#INI!") return "not initialized";
  if (five == "#DEV!") return "bad device id";
  return "unknown error";
}

// -----------------------------
// ハンドラ本体
// -----------------------------
void h_any_path() {
  // 共通フィールドの初期値
  String  error  = "";
  bool    result = false;
  int     value  = -1;
  String  text   = "";

  // 受信URIを取得
  String uri = g_srv->uri();

  // ルート "/" は固定レスポンス（サービス到達確認用）
  if (uri == "/" || uri.length() == 0) {
    send_json(F("{\"ok\":true,\"command\":\"/\",\"error\":\"\",\"result\":true,\"value\":-1,\"text\":\"\"}"));
    return;
  }

  // 先頭の '/' を取り除いて path を得る（例: "MP3/TRACK/PLAY:1:2:3"）
  String path = uri;
  if (path.length() && path[0] == '/') path.remove(0, 1);

  // 最初の ':' でコマンド名と引数連結部を分離
  int colon = path.indexOf(':');
  String cmd, args;
  if (colon < 0) {
    cmd  = path;        // 引数なし
    args = "";
  } else {
    cmd  = path.substring(0, colon);     // 純粋なコマンド名
    args = path.substring(colon + 1);    // "a:b:c" 形式の引数連結
  }

  // コマンド定義の検索（未登録は即時エラー）
  const CmdSpec* spec = find_spec(cmd);
  if (!spec) {
    String js = String("{\"ok\":true,\"command\":\"") + path +
      "\",\"error\":\"command not found\",\"result\":false,\"value\":-1,\"text\":\"\"}";
    send_json(js);
    return;
  }

  // UART 送信用ワイヤ文字列を組み立て： <CMD>[:<args>]!
  String wire = cmd;
  if (args.length() > 0) {
    wire += ':';
    wire += args;
  }
  if (wire.length() == 0 || wire[wire.length() - 1] != '!') wire += '!'; // 末尾'!'の重複付与はしない

  // 送信前に残渣クリア → 送信
  HardwareSerial* U = UARTS[0].port;
  while (U->available() > 0) (void)U->read();
  U->print(wire);

  // 応答待ち（5文字固定、末尾 '!' 前提）
  String five = read_reply_5(U, /*timeout_ms*/ 1000);

  // 応答解釈（仕様に従う）
  if (five.length() != 5 || five[4] != '!') {
    // タイムアウト／不正
    error  = "timeout or bad reply";
    result = false;
    value  = -1;
    text   = "";
  } else if (five[0] == '#') {
    // 異常（#CMD!/#CHK!/#INI!/#DEV! など）
    error  = map_error(five);
    result = false;
    value  = -1;
    text   = "";
  } else if (five == "!!!!!") {
    // RESULT 系 OK
    error  = "";
    result = (spec->kind == RK_RESULT);
    value  = -1;
    text   = "";
  } else {
    // "XXXX!"（4文字＋'!'）
    String body = five.substring(0, 4);

    if (spec->kind == RK_VALUE) {
      int v = -1;
      if (parse_dec4(body, v)) {          // 10進4桁（負値も "-100" のように4文字で来る想定）
        error  = "";
        result = true;
        value  = v;
        text   = "";
      } else {
        error  = "bad numeric response";
        result = false;
        value  = -1;
        text   = "";
      }
    } else if (spec->kind == RK_STRING) {
      error  = "";
      result = true;
      value  = -1;
      text   = body;
    } else {
      // kind=RESULT なのに "XXXX!" が来た場合は成功扱い（仕様上は非到来想定）
      error  = "";
      result = true;
      value  = -1;
      text   = "";
    }
  }

  // JSON（共通キーは常に固定で返す）
  String js;
  js.reserve(160);
  js += F("{\"ok\":true");
  js += F(",\"command\":\"");
  js += path;
  js += F("\",\"error\":\"");
  js += error;
  js += F("\",\"result\":");
  js += (result ? "true" : "false");
  js += F(",\"value\":");
  js += String(value);
  js += F(",\"text\":\"");
  js += text;
  js += F("\"}");
  send_json(js);
}

// ルート（簡易疎通）
void h_root(){ send_json(F("{\"ok\":true,\"service\":\"MMP-WebAPI\",\"note\":\"GET-only\"}"));}

// OPTIONS helper
void allow_options(const char* path) { g_srv->on(path, HTTP_OPTIONS, [](){ send_204(); }); }

} // namespace

namespace WebApiCore {

void begin(WebServer& server) {
  g_srv = &server;

  // ルート
  server.on("/", HTTP_GET, h_root);
  allow_options("/");

  // 任意コマンド（全てここに集約）
  server.onNotFound([](){
    if (!g_srv) return;
    if (g_srv->method() == HTTP_OPTIONS) { send_204(); return; }
    h_any_path();
  });

  // 起動ログ
  IPAddress ip = (WiFi.getMode()==WIFI_AP) ? WiFi.softAPIP() : WiFi.localIP();
  Serial.printf("[WebApi] ready on %s (unified handler, decimal VALUE)\n", ip.toString().c_str());
}

void handle(WebServer& server) {
  server.handleClient();
}

} // namespace WebApiCore
