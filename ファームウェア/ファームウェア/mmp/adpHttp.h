// filename : adpHttp.h
//========================================================
// 通信アダプタ：ＷｅｂＡＰＩ
//--------------------------------------------------------
// ・HTTP(WEB API)サーバの起動
// ・WEB APIのポーリング処理
//   受信→認証開始→認証検証→コマンド実行→結果返却(JSON)
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/11) α版 
//・ファイル名を変更
//・インクルードファイルを最適化
//・コメントを強化
//・namespaceを固有の1つにまとめた
//・各種リソース(処理/構造体など)を共通側に定義
//・ルート１：ＭＭＰコマンド
//  - 引数をスロット(オブジェクト)に変更
//  - プロセスを見直し
//  - 接続情報を大幅変更
//・必要な情報はすべてコンテストに集約
//・スロットを再設計
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <WebServer.h>
  //│
  //■ＭＭＰシステム
  #include "adp.h" // 通信アダプタ共通
  //┴
//┴

//========================================================
// グローバル資源
//========================================================
  //━━━━━━━━━━━━━━━━━
  // コンテクスト
  //━━━━━━━━━━━━━━━━━
  extern MmpContext ctx;       // 所在：mmpCtx.h、実装：mmp.ino

  //━━━━━━━━━━━━━━━━━
  // コマンドパーサ
  //━━━━━━━━━━━━━━━━━
  extern String MMP_REQUEST(); // 所在：parser.h

//========================================================
// ＷｅｂＡＰＩ通信アダプター
//========================================================
namespace adpHttp {
//━━━━━━━━━━━━━━━━━
// 基本情報
//━━━━━━━━━━━━━━━━━
  static constexpr int  ROUTE_ID = ROUTE_ID_HTTP ; // 経路IDを定義
  WebServer*       ns_ACCEPTOR   = nullptr       ; // ユーザ受付資源を定義

//━━━━━━━━━━━━━━━━━
// ユーザ認証
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // テーブルを領域確保
  //─────────────────
  static TYPE_AUTH_SLOT AUTH_TBL[SS_SLOTS]; 

//━━━━━━━━━━━━━━━━━
// 接続情報
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // テーブルを宣言
  //─────────────────
  static SLOT_HTTP* ssTBL = nullptr;

  //─────────────────
  // 接続情報TBLを作る
  //----------------------------------
  // 戻り値：なし
  //─────────────────
  void SS_CREATE_TBL(){
  //┬
  //○領域を確保
  //○TBL全体を初期化
  ssTBL = new SLOT_HTTP[1]; // 容量：一時データ1個
  INIT_SLOT_HTTP(ssTBL[0]);
  //┴
  } /* SS_CREATE_TBL() */

  //─────────────────
  // スロットを開放
  //─────────────────
  //　➡【該当処理なし】※マルチ接続系が対象

  //─────────────────
  // 空きスロットを照会
  //─────────────────
  //　➡【該当処理なし】※マルチ接続系が対象

  //─────────────────
  // 接続を登録
  //----------------------------------
  // 開始処理で「一度だけ」実行すること
  //─────────────────
  void SS_ATTACH_SLOT(){
  //┬
  //○スロットに新規接続を登録
  ssTBL[0].conn   = ns_ACCEPTOR   ; // HTTP受付資源を管理スロットへ登録
  ssTBL[0].roomNo = GET_ROOM_No() ; // ルーム番号をセット
  //┴
  } /* SS_ATTACH_SLOT() */

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


//━━━━━━━━━━━━━━━━━
// レスポンス
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // JSON形式でレスポンス
  //─────────────────
  inline void SEND_JSON(
    WebServer&    argSrv , // 送信先
    const String& argJSON
  ) {
    ADD_CROSS(argSrv);
    argSrv.send(
        200,
        "application/json; charset=utf-8",
        argJSON
    );
  } /* SEND_JSON() */

  //─────────────────
  // コマンドパーサーからの戻り値が数値型であるか判定
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
  // コマンドパーサーからの戻り値を数値に変換
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

    // コマンドパーサーの戻り値
    if (argID == "!!!!!") return "OK:戻り値無し"            ;
    if (argID == "!VAL!") return "OK:数値"                  ;
    if (argID == "!STR!") return "OK:文字列"                ;
    if (argID == "#CMD!") return "NG:コマンド名が不正"      ;
    if (argID == "#CHK!") return "NG:引数チェックで違反"    ;
    if (argID == "#INI!") return "NG:データが未初期化"      ;
    if (argID == "#DEV!") return "NG:使用不可のデバイス"    ;
    if (argID == "#FIL!") return "NG:ファイル操作が異常終了";
    if (argID == "#NOD!") return "NG:データ項目名が不正"    ;

    // アダプタ独自のエラー
    if (argID == "!AST!") return "OK:認証開始(認証コード)"     ;
    if (argID == "#DFL!") return "NG:URIが不正"                ;
    if (argID == "#SS0!") return "NG:認証管理の開始に失敗"     ;
    if (argID == "#SS1!") return "NG:ユーザ認証に失敗"         ;
    if (argID == "#SS2!") return "NG:接続認証に失敗"           ;
 
    return "NG:その他のエラー";
  } /* SEND_MSG() */

  //─────────────────
  // データ送信
  //─────────────────
  struct JSON_DATA{
    bool    Res = false; // MMPの処理結果      {OK:true | NG:false}
    String  Msg = ""   ; // エラーMSG          {正常の場合は空}
    int     Val = -1000; // 戻値が数値の場合   {-999～9999、対象外は-1000 }
    String  Str = ""   ; // 戻値が文字列の場合 {４バイトの文字列、対象外は空}
  }; /* JSON_DATA */
  //─────────────────
  void SEND_CONN(
    SLOT_HTTP&    argSS  , // 送信先
    const String& argMSG   // 送信メッセージ
  ){
  //┬
  //○前処理
  JSON_DATA jsDat     ;
  String    js;
  //│
  //◇┐JSON内容編集
  String msgID = argMSG;
    if (
        argMSG.length() == 7 &&  // 全長7文字
        argMSG[0] == '$'     &&  // 先頭記号
        argMSG[6] == '$'         // 末尾記号
    ){
    //├→（認証コード発行の場合）
      //○MSGIDを独自IDに書き換え
      //○取得値を文字列型にセット
      //○処理結果をセット
      msgID     = "!AST!"              ; // 認証開始
      jsDat.Str = argMSG.substring(1,6); // 取得値(文字列：認証コード)
      jsDat.Res = true                 ; // 正常
      //┴
    } else if (argMSG == "!!!!!") {
    //├→（正常系：戻り値なし の場合）
      //○処理結果を正常にセット
      jsDat.Res = true ; // 正常
      //┴
    } else {
    //└┐（その他）
      //◇┐データ型に応じて編集
      String body = argMSG.substring(0, argMSG.length()-1);
      if (SEND_IS_VALUE(body)) {
      //├→（戻り値が数値型の場合）
        //○MSGIDを独自IDに書き換え
        //○処理結果をセット
        //●取得値を数値型にセット
        msgID = "!VAL!"                  ; // 数値型
        jsDat.Val = SEND_CONV_VALUE(body); // 取得値(数値)
        jsDat.Res = true                 ; // 正常
        //┴
      } else {
      //└┐（その他；戻り値が文字列型の場合）
        //○MSGIDを独自IDに書き換え
        //○処理結果をセット
        //○取得値を文字列型にセット
        msgID = "!STR!"  ; // 文字列型
        jsDat.Str = body ; // 取得値(文字列)
        jsDat.Res = true ; // 正常
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
  js += F(",\"result\":"   ); js += (jsDat.Res ? "true" : "false"); // 処理結果：MMPコマンドの成功
  js += F(",\"message\":\""); js += jsDat.Msg; js += '"'          ; // メッセージ
  js += F(",\"value\":"    ); js += String(jsDat.Val)             ; // 戻値（数値）
  js += F(",\"string\":\"" ); js += jsDat.Str                     ; // 戻値（文字列）
  js += "\"}"              ;
  //│
  //○通信経路にJSON形式でレスポンス
  SEND_JSON(*argSS.conn, js);
  //┴
  } /* SEND_CONN() */
  //─────────────────

//━━━━━━━━━━━━━━━━━
// ルーティング処理
//----------------------------------
// HTTPサーバが自動的に呼び出す ※registRoutes()参照
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ルート１：ＭＭＰコマンド
  //----------------------------------
  // 引数：
  // (参)接続情報スロット
  //----------------------------------
  //【単接続：単スロット使いまわし型】
  //・ユーザに関係なくHTTPリクエスト・１スロット(一時型;使いまわし)の構成
  //・通信状態は保持しない(リクエスト単位で処理を完結)
  //・都度処理を完結する為、毎回ユーザー認証する
  //----------------------------------
  //【処理フロー】
  //・状態管理       ：しない ※リスナー自身が実行する為
  //・フレーム分割   ：認証コードとコマンドパスに分割する
  //・ユーザ認証     ：する
  //・接続認証       ：しない ※リクエスト単位で処理する為
  //----------------------------------
  //【処理詳細】
  // 資源            ：WebServer* (参照)
  // 資源の実体所有  ：する
  // フレーム書式    ：{認証コード}/{コマンドパス}!
  // データ受信単位  ：フレーム単位
  // フレーム化処理  ：しない ※WebServer[conn->uri()]でフレーム化済み
  // 受信バッファ    ：しない
  // 受信継続判定    ：available()
  // フレーム終端判定：しない ※WebServer[conn->uri()]でフレーム化済み
  //─────────────────
  void routeMMP(SLOT_HTTP& argSS){
  //┬
  //○┐０．前処理
    //○0-1.チェックアウトを確認
    //　➡【該当処理なし】※リスナが実行しているので不問
    //│
    //○0-2.ワーク変数を用意
    bool isReady = false;
    //│
    //○0-3.コンテクストを初期化
    ctx.cmdPath = ""          ; // コマンドパス（この後で取得）
    ctx.roomNo  = argSS.roomNo; // ルーム番号  （接続情報）
    ctx.zoneNo  = -1          ; // ゾーン番号  （この後で取得）
    ctx.accNo   = -1          ; // アクセスID  （この後で取得）
    //┴
  //│
  //○┐１．受信待ちデータの取り込み
    //○1-1.取り込みの継続を確認
    //　➡【該当処理なし】※ストリーム系が対象
    //│
    //○1-2.受信データを受信バッファに加える
    String ch = argSS.conn->uri();
    if (argSS.rx.length() < SS_RX_SIZE) {argSS.rx += ch;}
    else {
    // ＼（オーバーフローした場合）
      //○オーバーフロー中へ移行
      //○受信バッファの内容を破棄
      argSS.isOverflow = true;
      argSS.rx         = "";
      //┴
    } /*【1-2.受信データを受信バッファに加える】*/
    //│
    //◇┐1-3.フレームの完成を確認
      //├→（コマンド終端を検出した場合）
        //○オーバーフローを確認
        if (argSS.isOverflow) {
        // ＼（オーバーフロー中の場合）
          //○オーバーフロー中を解除
          //○受信バッファを破棄
          argSS.isOverflow = false;
          argSS.rx         = ""   ;
          //│
          //●エラーコードをレスポンス
          //▼ルーティング処理を中断
          SEND_CONN(argSS, "#DFL!");
          return;
        } /*【1-3-1.処理フローを制御】*/
        //│
        //○準備完了フラグを[ON]
        //▼取り込みを終了
        isReady = true;
        //break;
      //┴
    //┴
  //│
  //○２．後続処理の継続を判断
  if (!isReady) return;
  // ＼（準備が未完了の場合）
    //▼ルーティング処理を中断
  //│
  //○┐３．事前準備
    //●3-1.受信バッファをURI形式に変換
    //○3-2.事前データを用意
    //○2-3.受信バッファを破棄
    FormatURI(argSS.rx);
    String authCD  = GetToken1(argSS.rx);
    String cmdPath = GetToken2(argSS.rx);
    argSS.rx = "";
    //┴
  //│
  //○┐４．認証を実施
    //◇┐4-1.認証開始要求に応答
    if (authCD == "_START_!") {
      //├→（例外的に認証コードの中身が「接続開始コマンド」の文字列の場合）
        //●4-1-1.この接続を認証管理に加える
        String newAuthCD = AUTH_START(AUTH_TBL);
        if(newAuthCD == ""){
        // ＼（管理開始に失敗した場合）
          //●エラーコードをレスポンス
          //▼ルーティング処理を中断
          SEND_CONN(argSS, "#SS0!"); // 認証開始に失敗
          return;
        } /* END-if*/
        //│
        //○4-1-2.認証コードをスロットに反映
        //●4-1-3.認証コードをレスポンス
        //▼4-1-4.ルーティング処理を中断
        argSS.authCD = newAuthCD;
        SEND_CONN(argSS, String("$") + newAuthCD + "$"); // 認証コード(前後に識別文字を付加)
        return;
      //┴
    } /*【4-1.認証開始要求を処理】*/
    //│
    //○4-2.ユーザ認証を実施
      ctx.zoneNo = AUTH_GET_ID(AUTH_TBL, authCD);
      if (ctx.zoneNo < 0) {
      // ＼（認証に失敗した場合）
        //●エラーコードをレスポンス
        //▼ルーティング処理を中断
        SEND_CONN(argSS, "#SS1!"); // セッションエラー(ユーザ認証)
        return;
      } /* 4-2-1.ユーザ認証を実施 */
      //┴
    //┴
  //│
  //○┐５．MMPコマンドを実行
    //●5-1.コンテクストの内容を確定
    //●5-2.コマンドパーサーへ処理を移譲
    //●5-3.実行結果をレスポンス
    ctx.accNo      = GET_ACC_NO(); // コンテクスト：アクセスID
    String mmpResp = MMP_REQUEST();
    SEND_CONN(argSS, mmpResp);
    //┴
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
// ポーリング
//========================================================
  //━━━━━━━━━━━━━━━━━
  // ルーティング登録
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
      // ＼（HTTP層で完結している）
        //●CORS事前確認へ応答
        route204(server);
        //▼RETURN
        return;
      }   /* if */
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
  //----------------------------------
  // 実行元：iniNet.h - InitNet_Service()
  //━━━━━━━━━━━━━━━━━
  bool start(uint16_t port) {

    // 1) 二重起動防止
    if (ns_ACCEPTOR ) return true    ; // サーバの実体化有無を評価

    // 2) サーバ資源生成
    ns_ACCEPTOR = new WebServer(port);

    // 3) 接続情報TBLを作成
    SS_CREATE_TBL()                  ; // 領域確保
    SS_ATTACH_SLOT()                 ; // 一時スロットを登録

    // 4) ルーティング登録
    registRoutes(*ns_ACCEPTOR )      ; // ルーティング登録

    // 5) サーバ開始
    ns_ACCEPTOR->begin()             ; // サーバ起動 ※ポインタ経由

    // 6) 正常終了
    return true;
  } /* start() */

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口（ポーリング入口）
  //----------------------------------
  // 実行元：mmp.ino - loop()
  //----------------------------------
  // 明示的にルーティング指示しない
  // サーバ(リスナー)がにルーティング登録した内容に従う
  //━━━━━━━━━━━━━━━━━
  void handle() {

    // 1) 起動チェック
    if (!ns_ACCEPTOR ) return; // サーバの実体化有無を評価

    // 2) 経路を指定
    ctx.floorNo = ROUTE_ID    ; // フロア番号（経路ID）

    // 3) 新規接続のスロットを登録
    //　➡【該当処理なし】※start()で登録済み

    // 4) ルーティング処理
    //　➡ WebServer自身が受付・解析・分岐まで担当(内容はregistRoutes()で定義済み)
    ns_ACCEPTOR ->handleClient(); // 具体的な処理はregistRoutes()で定義済み

  } /* handle() */
} /* namespace adpHttp */