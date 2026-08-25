// filename : adpWAPI.cpp
//========================================================
// 通信アダプタ：ＷｅｂＡＰＩ
//（リクエスト／レスポンス型の通信）
//--------------------------------------------------------
//【目的】
// リクエストに従い、ＭＭＰコマンドを実行して、
// 結果をレスポンスする。
//--------------------------------------------------------
//【処理機能】
//・WEBサーバ(リスナ)が保持する「１リクエスト」を処理する
//・リクエストを取得する
//  - リスナ(ルーティング)を事前に登録する
//  - リスナがライズすると1リクエストを保持する
//  - リスナがライズしていない場合は何も行われない
//・リクエストを基に共通情報(コンテクスト)を纏める
//・必要に応じてユーザ認証を実施する
//・MMPコマンドを実行する
//・MMPコマンドの実行結果をレスポンする
//・処理中にエラーなどがあれば適宜レスポンスする
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/23) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <WebServer.h> // ユーザ受付資源
  //│
  //■ＭＭＰシステム
  #include "adp.h"  // 通信アダプタ共通へ公開
  //┴
//┴

//########################################################
//# 専用名の前空間
//########################################################
namespace adpWAPI {
//========================================================
// Ａ．基本情報
//========================================================
  //─────────────────
  // 公開情報
  //─────────────────
  const int  ROUTE_ID = ROUTE_ID_WAPI  ; // ＷＥＢ ＡＰＩ
  const int  SS_SLOTS = 1              ; // 固定スロット(1個を使いまわし)
        bool ENABLED  = false          ; // 有効性：{有効：true|無効：false}

  //─────────────────
  // 使用するサービス
  //─────────────────
  static WebServer* ADP_SRV  = nullptr; // WEBサーバ
  static int        SRV_PORT = 8080   ; // ポート番号

  //─────────────────
  // 接続スロット
  //─────────────────
  struct T_SS_SLOT{
    SS_SLOT_TYPE Base          ; // 基本メンバ
    WebServer*   conn = nullptr; // アクセス資源(参照)
  };
  static T_SS_SLOT* ssTBL = nullptr; // 事前予約

//========================================================
// Ｂ．接続管理
//=======================================================
  //─────────────────
  // 初期化
  //----------------------------------
  // 引数：(参照)接続管理スロット
  //─────────────────
  void SS_INI_SLOT(T_SS_SLOT& argSlot){}
  // ➡【該当処理なし】※固定スロット

  //─────────────────
  // 空きSID取得
  //----------------------------------
  // 戻り値：スロットID
  // ・0,1,2...：空きスロットのID
  // ・-1：空きスロットが無い
  //─────────────────
  int SS_GET_FREE_ID(){return -1;}
  // ➡【該当処理なし】※固定スロット

  //─────────────────
  // 登録（自動1スロット）
  //----------------------------------
  // 戻り値：スロットID（数値）
  // ・-1   ：失敗
  // ・0以上：成功
  //─────────────────
  int SS_ATTACH_EACH(){return -1;}
  // ➡【該当処理なし】

  //─────────────────
  // 登録（自動一括スロット）
  //----------------------------------
  // 戻り値 ：処理結果（論理値）
  // ・false：正常
  // ・true ：異常
  //─────────────────
  bool SS_ATTACH_FOREACH(){return true;}
  // ➡【該当処理なし】

  //─────────────────
  // 登録（固定スロット）
  //─────────────────
  void SS_ATTACH_STATIC(){
    //┬
    //○スロットに新規接続を登録
    ssTBL[0].Base.used = true   ; // 使用中
    ssTBL[0].conn      = ADP_SRV; // 参照先を登録
    //┴
  } /* SS_ATTACH_STATIC() */

//━━━━━━━━━━━━━━━━━
// ヘルパー
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // CORS許可用HTTPヘッダ追加
  //----------------------------------
  // ブラウザ上のJavaScriptからWeb APIを呼び出すための許可設定
  // → Webブラウザのセキュリティ制約(CORS)を通過させる
  //─────────────────
  inline void ADD_CROSS(WebServer& argSrv) {
    //┬
    //○アクセス元Webページの制限
    //  → 制限なし
    argSrv.sendHeader("Access-Control-Allow-Origin", "*");
    //│
    //○有効なHTTPメソッドを指定
    //  → データ取得・事前確認
    argSrv.sendHeader("Access-Control-Allow-Methods", "GET,OPTIONS");
    //│
    //○許可するHTTPリクエストヘッダーを指定
    //  → データ形式・JavaScript(Ajax)向け識別・認証情報
    argSrv.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Requested-With, Authorization");
    //│
    //○CORS確認結果をブラウザが記憶する時間を指定
    //  ← 600秒=10分
    argSrv.sendHeader("Access-Control-Max-Age", "600");
    //┴
  } /* ADD_CROSS() */

//========================================================
// Ｃ．レスポンス
//========================================================
  //─────────────────
  // JSON形式でレスポンス
  //─────────────────
  inline void SEND_JSON(
    WebServer&    argSrv , // 送信先
    const String& argJSON
  ) {
    //┬
    //●ログ出力
    if (ctx.sysLog >= 0) F_SHOW_LOG(argJSON);
    //│
    //○JSONをレスポンス
    ADD_CROSS(argSrv);
    argSrv.send(200, "application/json; charset=utf-8", argJSON);
    //┴
  } /* SEND_JSON() */

  //─────────────────
  // コマンド管理の戻り値が数値型であるか判定
  //─────────────────
  static bool SEND_IS_VALUE(const String& argBody){
    if (argBody.length() != 4) return false;
    int start = (argBody[0]=='-') ? 1 : 0;
    for (int i=start; i<4; ++i){
      if (!isDigit((unsigned char)argBody[i])) return false;
    } // for
    return true;
  } /* SEND_IS_VALUE() */

  //─────────────────
  // コマンド管理の戻り値が文字列型であるか判定
  //─────────────────
  static bool SEND_IS_STRING(const String& argBody){
    if (argBody.startsWith("#")) return false;
    if (argBody.startsWith("!")) return false;
    return true;
  } /* SEND_IS_STRING() */

  //─────────────────
  // コマンド管理の戻り値を数値に変換
  //─────────────────
  static int SEND_CONV_VALUE(const String& argBody){
    bool neg = (argBody[0]=='-');
    int v = 0;
    for (int i = neg ? 1 : 0; i < 4; ++i) v = v*10 + (argBody[i]-'0');
    return neg ? -v : v;
  } /* SEND_CONV_VALUE() */

  //─────────────────
  // メッセージIDに該当するメッセージを取得
  //─────────────────
  static const char* SEND_MSG(const String& argID){

    // 共通のコード
    if (argID == "!!!!!") return "OK:戻り値無し"            ;
    if (argID == "#CMD!") return "NG:コマンド名が不正"      ;
    if (argID == "#CHK!") return "NG:引数チェックで違反"    ;
    if (argID == "#INI!") return "NG:データが未初期化"      ;
    if (argID == "#DEV!") return "NG:使用不可のデバイス"    ;
    if (argID == "#FIL!") return "NG:ファイル操作が異常終了";
    if (argID == "#NOD!") return "NG:データ項目名が不正"    ;
    if (argID == "#VAL!") return "NG:数値が基底範囲外"      ;
    if (argID == "#NOM!") return "NG:機能モジュールが無い"  ;

    // アダプタ独自のコード
    if (argID == "!VAL!") return "OK:数値"                  ;
    if (argID == "!STR!") return "OK:文字列"                ;
    if (argID == "#DFL!") return "NG:フレーム長オーバー"    ;
    if (argID == "#SSZ!") return "NG:接続スロット不足"      ;
    if (argID == "!SS0!") return "OK:ユーザ認証に成功"      ;
    if (argID == "#SS1!") return "NG:認証管理の開始に失敗"  ;
    if (argID == "#SS2!") return "NG:認証に失敗"            ;
 
    return "NG:その他のエラー";
  } /* SEND_MSG() */

  //─────────────────
  // スロットの受付資源に送信
  //─────────────────
  struct JSON_DATA{
    bool    Res = false; // MMPの処理結果      {OK:true | NG:false}
    String  Msg = ""   ; // エラーMSG          {正常の場合は空}
    int     Val = -1000; // 戻値が数値の場合   {-999～9999、対象外は-1000 }
    String  Str = ""   ; // 戻値が文字列の場合 {４バイトの文字列、対象外は空}
  }; /* JSON_DATA */
  //─────────────────
  void SEND_CONN(
    T_SS_SLOT&    argSS, // 送信先
    const String& argMSG // 送信メッセージ
  ){
    //┬
    //○前処理
    JSON_DATA jsDat     ;
    String    js;
    //│
    //◇┐JSON内容編集
    String msgID = argMSG;
    if (ctx.authCD != ""){
      //├┐（認証コード発行の場合）
        //○MSGIDを独自IDに書き換え
        //○取得値を文字列型にセット
        //○処理結果をセット
        msgID     = "!SS0!"              ; // 認証開始
        jsDat.Str = argMSG               ; // 取得値(文字列)
        jsDat.Res = true                 ; // 正常
        //┴

    } else if (argMSG == "!!!!!") {
      //├┐（正常系：戻り値なし の場合）
        //○処理結果を正常にセット
        jsDat.Res = true ; // 正常
        //┴

    } else {
      //└┐（その他）
        //◇┐データ型に応じて編集
        String body = argMSG.substring(0, argMSG.length()-1);
        if (SEND_IS_VALUE(body)) {
          //├┐（戻り値が数値型の場合）
            //○MSGIDを独自IDに書き換え
            //○処理結果をセット
            //●取得値を数値型にセット
            msgID = "!VAL!"                  ; // 数値型
            jsDat.Val = SEND_CONV_VALUE(body); // 取得値(数値)
            jsDat.Res = true                 ; // 正常
            //┴

        } else if (SEND_IS_STRING(argMSG)) {
          //├┐（戻り値が文字列型の場合）
            //○MSGIDを独自IDに書き換え
            //○処理結果をセット
            //●取得値を数値型にセット
            msgID = "!STR!"                  ; // 文字列型
            jsDat.Str = argMSG               ; // 取得値(文字列)
            jsDat.Res = true                 ; // 正常
            //┴

        } else {
          //└┐（その他）
            //○処理結果をセット
            jsDat.Res = false                 ; // 異常
            //┴
        } /* END-if */
        //┴
    } /* END-if */
    //│
    //○メッセージを取得
    jsDat.Msg = SEND_MSG(msgID);
    //│
    //○JSON形式に編集
    js.reserve(160) ; // 予備確保
    js += F("{\"ok\":true"   )                                      ; // 処理結果：HTTP通信の成功
    js += F(",\"source\":\"" ); js += msgID.c_str(); js += '"'      ; // MMPの戻り値
    js += F(",\"result\":"   ); js += (jsDat.Res ? "true" : "false"); // 処理結果：MMPコマンドの成功
    js += F(",\"message\":\""); js += jsDat.Msg; js += '"'          ; // メッセージ
    js += F(",\"value\":"    ); js += String(jsDat.Val)             ; // 戻値（数値）
    js += F(",\"string\":\"" ); js += jsDat.Str                     ; // 戻値（文字列）
    js += "\"}"               ;
    //│
    //○通信経路にJSON形式でレスポンス
    SEND_JSON(*argSS.conn, js);
    //┴
  } /* SEND_CONN() */
  //─────────────────

//========================================================
// Ｄ．プロセス部品
//========================================================
  //─────────────────
  // １．接続状態を確認
  //----------------------------------
  // 戻り値：接続状態（論理値）
  // ・false：良好
  // ・true ：不良
  //─────────────────
  bool P1_CONNECT(T_SS_SLOT& argSS){return false;}
  // ➡【該当処理なし】

  //─────────────────
  // ２．フレームを取得
  //----------------------------------
  // 引数：(参照)接続管理スロット
  //----------------------------------
  // 戻り値：フレーム作成状況（論理値）
  // ・true ：未完成
  // ・false：完成
  //----------------------------------
  //【データ受信方式】
  // ・取得単位  ：パケット
  // ・データ受信：サーバ(参照) argSS.conn->uri()
  //─────────────────
  bool P2_MAKE_FRAME(T_SS_SLOT& argSS){
    //┬
    //○受信データからフレームを作成
    ctx.strFrame = argSS.conn->uri();
    //│
    //●フレームをURI形式に変換
    F2_FORMAT_URI(ctx.strFrame);
    //│
    //▼返却：フレームの作成状況
    return (ctx.strFrame == "" ? true : false);
    //┴
  } /* P2_MAKE_FRAME() */

  //─────────────────
  // ３．基本情報を取得
  //─────────────────
  void P3_MAKE_INFO(){
    //┬
    //〇フレームの内容をもとに認証CD・コマンドパスにセット
    F3_SET_ACD_CPATH();
    //┴
  } /* P3_MAKE_INFO() */

  //─────────────────
  // ４．認証を実施
  //----------------------------------
  // 引数：(参照)接続管理スロット
  //----------------------------------
  // 戻り値：認証後の指針(論理値)
  // ・false： 処理継続が可能
  // ・true ： 処理継続が不可
  //─────────────────
  bool P4_AUTH(T_SS_SLOT& argSS){
    //┬
    //●認証処理を実施
    if (F4_CHECK_AUTH()){SEND_CONN(argSS, ctx.errMSG); return true;}
    //│＼（処理継続が不可の場合）
    //│ ●エラーをレスポンス
    //│ ▼返却：処理継続が不可
    //│
    //▼返却：処理継続が可能
    return false;
  } /* P4_AUTH() */

  //─────────────────
  // ５．MMPコマンドを実行
  //----------------------------------
  // 引数：(参照)接続管理スロット
  //─────────────────
  void P5_RUN_COMMAND(T_SS_SLOT& argSS){
    //┬
    //●MMPコマンドを実行
    String resMMP = F5_RUN();
    //│
    //●実行結果をレスポンス
    SEND_CONN(argSS, resMMP);
    //┴
  } /* P5_RUN_COMMAND() */

//========================================================
// Ｅ．ルーティング処理（プロセス）
//--------------------------------------------------------
// HANDLE()で明示的に呼び出す
// WebServerのリスナーが必要に応じて実行 ※registRoutes()参照
//========================================================
  //─────────────────
  // ルート１：ＭＭＰコマンド
  //----------------------------------
  // 引数：
  // (参)接続管理スロット
  //─────────────────
  void routeMMP(T_SS_SLOT& argSS){
    //┬
    //○１．接続状態を確認
    if (P1_CONNECT(argSS)) return;
    //│＼（不良の場合）
    //│ ▼終了：早期リターン
    //│
    //●２．フレームを取得
    if (P2_MAKE_FRAME(argSS)) return;
    //│＼（未完成の場合）
    //│ ▼終了：早期リターン
    //│
#if defined(MMP_TYPE_MAIN) // --┨ＭＭＰ本体┠----┐
    //○３．基本情報を取得
    P3_MAKE_INFO();
    //│
    //○４．ユーザ認証を実施
    if (P4_AUTH(argSS)) return;
    //│＼（処理継続が不可の場合）
    //│ ▼終了：早期リターン
#endif // ----------------------------------------┘
    //│
    //●５．MMPコマンドを実行
    P5_RUN_COMMAND(argSS);
    //┴
  } /* routeMMP() */

  //─────────────────
  // ２．ホスト直下
  //─────────────────
  void routeRoot(WebServer& srv){
    SEND_JSON(srv, F("{"
      "\"ok\":true,"
      "\"result\":true,"
      "\"error\":\"\","
      "\"value\":-1,"
      "\"text\":\"MMP WEB API\""
      "}"));
  } /* routeRoot() */

  //─────────────────
  // ３．CORS事前確認
  //----------------------------------
  // ブラウザがWeb APIアクセス前に送信するOPTIONS要求(プリフライト)へ応答
  // → CORS許可ヘッダを付加してブラウザへ許可情報を通知
  // → 本通信で返すデータはないためHTTPステータス204を返却
  //─────────────────
  inline void route204(WebServer& argSrv) {
    //┬
    //●CORS許可用HTTPヘッダ追加
    ADD_CROSS(argSrv);
    //│
    //○HTTPステータスを返却
    //  ※豆知識{200:返すデータあり｜204:返すデータなし}
    // argSrv.send(204);
    argSrv.send(204, "text/plain", "");
    //┴
  } /* route204() */

//========================================================
// Ｆ．初期化・ポーリング用ハンドル
//========================================================
  //━━━━━━━━━━━━━━━━━
  // WEBサーバのルーティング(リスナー)登録
  //━━━━━━━━━━━━━━━━━
  void registRoutes(WebServer& server){
    //┬
    //○┐ルート０：ホスト直下の登録
      //●GETへの応答
      //●CORS事前確認へ応答
      server.on("/", HTTP_GET,     [&server](){routeRoot(server);});
      server.on("/", HTTP_OPTIONS, [&server](){route204(server); });
      //┴
    //│
    //○┐ルート１：ＭＭＰコマンドの登録
    server.onNotFound( [&server](){ // NotFound("/"以外)が処理対象
      //○ＭＭＰ処理へ渡す要求であるかを確認
      if (server.method() == HTTP_OPTIONS){
      //│＼（HTTP層で完結している）
          //●CORS事前確認へ応答
          //│ ▼終了：早期リターン
          route204(server);
          return;
      }   /* if */
      //│
      //●対象スロットをセット
      F0_SETUP(ROUTE_ID, 0);
      //│
      //●ＭＭＰコマンドへの応答
      routeMMP(ssTBL[0]);
      //┴
      } /* [&server]() */
    );  /* server.onNotFound */
    //┴
  }/* registRoutes() */

  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START() {
    //┬
    //○１．前準備の完了状態を確認
    if (ADP_SRV) {
    //│＼（通信デバイスが既に起動している場合）
        //○エラーメッセージを表示
        //○無効化
        //▼返却：早期リターン
        Serial.println("　WEB API    : ＷＥＢサーバサーバは既に起動しています ");
        ENABLED = false; // 無効
        return;
    } /* END-if */
    //│
    //○２．対象の通信経路を宣言
    ctx.routeID = ROUTE_ID; // コンテクストにルートIDをセット
    //│
    //○３．サーバ資源生成
    ADP_SRV = new WebServer(SRV_PORT); // WebServer
    //│
    //○４．接続管理TBLを作成
    ssTBL = new T_SS_SLOT[SS_SLOTS];
    SS_ATTACH_STATIC();
    //│
    //○５．ルーティング登録
    registRoutes(*ADP_SRV);
    //│
    //○６．サーバ開始
    ADP_SRV->begin();
    //│
    //○┐７．成功終了
      //○成功メッセージ
      //○有効化
      Serial.println(String("　[OK] WEB API   -> port ") + String(SRV_PORT));
      ENABLED = true; // 有効
      //┴
    //┴
  } /* START() */

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口（ポーリング入口）
  //━━━━━━━━━━━━━━━━━
  void HANDLE() {
    //┬
    //○１．起動チェック
    if (!ENABLED) return; // 初期化済み
    //│＼（このアダプタが無効の場合）
    //│ ▼終了：早期リターン
    //│
    //○２．新規接続のスロットを登録
    // ➡【該当処理なし】※固定スロット
    //│
    //○４．ルーティング処理
    ADP_SRV->handleClient(); // WEBサーバのリスナに処理を移譲
    //┴
  } /* HANDLE() */

} /* namespace adpWAPI */