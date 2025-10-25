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

// 統一入口(fnPerser.h)
extern String MMP_REQUEST(const String& wire, int clientID);

// 内部で WebServer を保持
static WebServer* g_SERVER = nullptr;

//========================================================
// ルーティング内処理
//========================================================
namespace {

  // 経路番号
  constexpr int g_ID_HTTP = 3;

//━━━━━━━━━━━━━━━━━
// ヘルパー：HTTPドキュメント
//━━━━━━━━━━━━━━━━━
  //─────────────────
  inline void add_cors(WebServer& srv) {
    srv.sendHeader("Access-Control-Allow-Origin", "*");
    srv.sendHeader("Access-Control-Allow-Methods", "GET,OPTIONS");
    srv.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Requested-With, Authorization");
    srv.sendHeader("Access-Control-Max-Age", "600");
  } // add_cors()
  //─────────────────
  inline void send_json(WebServer& srv, const String& s, int code=200) {
    add_cors(srv);
    srv.send(code, "application/json; charset=utf-8", s);
  } // send_json()
  //─────────────────
  inline void send_204(WebServer& srv) { add_cors(srv); srv.send(204); }
  //─────────────────

//━━━━━━━━━━━━━━━━━
// ヘルパー：ＭＭＰコマンド編集
//━━━━━━━━━━━━━━━━━
  static const char* map_error(const String& resp){
    if (resp == "#CMD!") return "Error:コマンド名が不正"      ;
    if (resp == "#CHK!") return "Error:引数チェックで違反"    ;
    if (resp == "#INI!") return "Error:データが未初期化"      ;
    if (resp == "#DEV!") return "Error:使用不可のデバイス"    ;
    if (resp == "#FIL!") return "Error:ファイル操作が異常終了";
    if (resp == "#NOD!") return "Error:データ項目名が不正"    ;
    return "その他のエラー";
  } // map_error()
  //─────────────────
  static bool isDec4Signed(const String& body){
    if (body.length() != 4) return false;
    int start = (body[0]=='-') ? 1 : 0;
    for (int i=start; i<4; ++i){
      if (!isDigit((unsigned char)body[i])) return false;
    } // for
    return true;
  } // isDec4Signed
  //─────────────────
  static int parseDec4Signed(const String& body){
    bool neg = (body[0]=='-');
    int v = 0;
    for (int i = neg ? 1 : 0; i < 4; ++i) v = v*10 + (body[i]-'0');
    return neg ? -v : v;
  } // parseDec4Signed()
  //─────────────────

//━━━━━━━━━━━━━━━━━
// ルート別処理
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ＭＭＰコマンド
  //─────────────────
  void routeMMP(WebServer& srv) {

    bool    result = false;   // MMPの処理結果      {OK:true | NG:false}
    String  error  = "";      // エラーメッセージ   {正常の場合は空}
    int     value  = -1000;   // 戻値が数値の場合   {-999～9999、対象外は-1000 }
    String  text   = "";      // 戻値が文字列の場合 {４バイトの文字列、対象外は空}

    // 1) ＭＭＰコマンド文字列を作成
      // 受信URIを取得
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
      } // if

      // 先頭の '/' を除去 → wire 組み立て(末尾'!'付与)
      String path = uri;
      if (path.length() && path[0] == '/') path.remove(0, 1);
      if (!path.endsWith("!")) path += '!';

    // 2) コマンドパーサーへリクエスト
    String resp = MMP_REQUEST(path, g_ID_HTTP);

    // 3) レスポンスを編集
      //◇コマンドパーサーの戻値に応じてJSON項目を編集
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
      // → 本文(末尾 '!' を除く)
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
        } // else
      } // if

      // JSON形式に編集
      String js;
      js.reserve(160);
      js += F("{\"ok\":true"  );
      js += F(",\"result\":"  ); js += (result ? "true" : "false");
      js += F(",\"error\":\"" ); js += error; js += '"'           ;
      js += F(",\"value\":"   ); js += String(value)              ;
      js += F(",\"text\":\""  ); js += text                       ;
      js += "\"}"              ;

    // 4) 通信経路にレスポンスする
    send_json(srv, js); // JSON形式に出力

  } // routeMMP()

  //─────────────────
  // ルート(ホスト直下)
  //─────────────────
  void routeRoot(WebServer& srv){
    send_json(srv, F("{"
      "\"ok\":true,"
      "\"result\":true,"
      "\"error\":\"\","
      "\"value\":-1,"
      "\"text\":\"MMP WEB API\""
      "}"));
  } // routeRoot()

//━━━━━━━━━━━━━━━━━
// ルーティング登録
//━━━━━━━━━━━━━━━━━
  //─────────────────
  void allow_options(WebServer& srv, const char* path) {
    srv.on(path, HTTP_OPTIONS, [&srv](){ send_204(srv); });
  } // allow_options()
  //─────────────────
  void registRootes(WebServer& server){

    // ルート((ホスト直下))を登録
    server.on("/", HTTP_GET, [&server](){routeRoot(server);});
    allow_options(server, "/");

    // ルート(ＭＭＰコマンド)を登録
    server.onNotFound(
      [&server](){
        if (server.method() == HTTP_OPTIONS){ send_204(server); return;}
        routeMMP(server);
      } // [&server]()
    );  // server.onNotFound
  } // registRootes()
  //─────────────────

} // namespace (No Name)


//========================================================
// ハンドラ関連処理
//========================================================
namespace srvHttp {
  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  // - cliNet.hから実行
  //━━━━━━━━━━━━━━━━━
  bool start(uint16_t port) {

    // 1) 前処理
      // 二重起動チェック： → 起動済みの場合は中断
      if ( g_SERVER ) return true;

      // スロット情報を確保
      // (該当処理なし)

    // 2) サーバを起動
    g_SERVER = new WebServer(port); // サーバ登録
    registRootes(*g_SERVER)       ; // ルーティング登録
    g_SERVER->begin()             ; // サーバ起動

    // 3) 正常終了
    return true;
  } // start

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口
  // - スケッチのloop()から実行
  // - 実処理はrouteMMP()
  //━━━━━━━━━━━━━━━━━
  void handle() {

    // 1) 起動チェック
    // (該当処理なし)

    // 2) スロット情報を更新
    // (該当処理なし)

    // 3) 全スロットを処理
    if ( g_SERVER ) g_SERVER->handleClient();
  } // handle()

} // namespace srvHttp