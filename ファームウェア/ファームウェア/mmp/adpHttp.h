// filename : adpHttp.h
//========================================================
// 通信アダプタ：ＷｅｂＡＰＩ
//--------------------------------------------------------
//【単接続：単スロット使いまわし型】
//・基本処理      ：受信→認証開始→認証検証→コマンド実行→結果返却(JSON)
//・スロット構成  ：ユーザに関係なくHTTPリクエスト・１スロット(一時型;使いまわし)
//・ポーリング跨ぎ：しない ※リクエスト単位で処理・接続が完結する為
//・認証          ：する   ※リクエスト単位で処理・接続が完結する為
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/11) α版 
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

//########################################################
//# メイン処理
//########################################################
namespace adpHttp {
//========================================================
// Ａ．基本情報
//========================================================
  static constexpr int  ROUTE_ID = ROUTE_ID_HTTP ; // 経路IDを定義
  WebServer*       ns_ACCEPTOR   = nullptr       ; // ユーザ受付資源を定義


//========================================================
// Ｂ．ユーザ認証
//========================================================
  //─────────────────
  // テーブルを領域確保
  //─────────────────
  static TYPE_AUTH_SLOT AUTH_TBL[SS_SLOTS]; 


//========================================================
// Ｃ．接続情報
//========================================================
  //─────────────────
  // テーブルを宣言
  //----------------------------------
  // 資源          ：WebServer* (参照)
  // 資源の実体所有：しない
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
  //----------------------------------
  // スロットを管理から外す
  //----------------------------------
  // 戻り値：なし
  //─────────────────
  void SS_DETACH_SLOT(SLOT_HTTP& argSlot){
  //　➡【該当処理なし】※マルチ接続系が対象
  } /* SS_DETACH_SLOT() */

  //─────────────────
  // 空きスロットを照会
  //----------------------------------
  // 該当条件：未使用
  //----------------------------------
  // 戻り値：
  // ・0,1,2...：空きスロットのID
  // ・-1：空きスロットが無い
  //─────────────────
  int SS_GET_FREE_ID() {
  //　➡【該当処理なし】※マルチ接続系が対象
  } /* SS_GET_FREE_ID() */

  //─────────────────
  // 接続を登録
  //----------------------------------
  // 開始処理で「一度だけ」実行すること
  //─────────────────
  void SS_ATTACH_SLOT(){
  //┬
  //○スロットに新規接続を登録
  ssTBL[0].conn = ns_ACCEPTOR; // HTTP受付資源を管理スロットへ登録
  ssTBL[0].used = true;        // 使用中
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


//========================================================
// Ｄ．レスポンス
//========================================================
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
    //├┐（認証コード発行の場合）
      //○MSGIDを独自IDに書き換え
      //○取得値を文字列型にセット
      //○処理結果をセット
      msgID     = "!AST!"              ; // 認証開始
      jsDat.Str = argMSG.substring(1,6); // 取得値(文字列：認証コード)
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


//========================================================
// Ｅ．プロセス部品
//========================================================
  //─────────────────
  // １．接続状態を確認
  //----------------------------------
  //【詳細】
  // 常時接続の物理ポートなので確認は不要
  //----------------------------------
  // 戻り値：接続状態（論理値）
  // ・false：接続中
  // ・true ：切断中
  //─────────────────
  bool P1_CONNECT(SLOT_HTTP&  argSS){return false;}

  //─────────────────
  // ２．フレームを取得(データ受信)
  //----------------------------------
  // 処理継続を判定は、一律で不可能とする。
  // 終端文字を見つけた場合、フレームをコンテクストに反映する。
  //----------------------------------
  //【詳細】
  // データ受信単位  ：フレーム単位 ※WebServer[conn->uri()]で実施済
  // 受信バッファ    ：しない
  // 受信継続判定    ：しない
  // フレーム終端判定：する
  //----------------------------------
  // 戻り値：処理継続の判定（論理値）
  // ・true ：不可能
  // ・false：可能
  //─────────────────
  bool P21_RECEIVE(SLOT_HTTP&  argSS){
  //┬
  //○受信データを受信バッファに加える
  argSS.rx = argSS.conn->uri();
  //│
  //○受信バッファのオーバーフローを確認
  if (argSS.rx.length() > SS_RX_SIZE) {
    // ＼（オーバーフローになった場合）
      //●エラーコードをレスポンス
      //▼RETURN:不可能(オーバーフローが発生)
      SEND_CONN(argSS, "#DFL!");
      return true;
  } /* END-if */
  //│
  //○取り込み状態を確認
  if (!argSS.rx.endsWith("!")) {
    // ＼（終端に達していない場合）※受信バッファを維持
      //●エラーコードをレスポンス
      //▼RETURN:不可能(フレームが未完成)
      SEND_CONN(argSS, "#CMD!");
      return true;
  } /* END-if */
  //│
  //●受信バッファをURI形式に変換
  //○コンテクストにフレームをセット
  P2_FORMAT_URI(argSS.rx);
  ctx.strFrame = argSS.rx;
  //│
  //▼RETURN:不可能
  return true;
  //┴
  } /* P21_RECEIVE() */

  //─────────────────
  // ２．フレームを取得
  //----------------------------------
  // フレームの作成状況を判定する。
  //----------------------------------
  //【詳細】
  // フレーム化処理  ：しない
  // 受信継続判定    ：する[conn->available()]★★★
  //----------------------------------
  // 戻り値：フレーム作成状況（論理値）
  // ・true ：未完成
  // ・false：完成
  //─────────────────
  bool P2_MAKE_FRAME(SLOT_HTTP&  argSS){
  //┬
  //○受信バッファの内容を破棄
  //●受信バッファに蓄える
  //○受信バッファの内容を破棄
  //▼処理継続の判定を返す
  argSS.rx = "";
  P21_RECEIVE(argSS);
  argSS.rx = "";
  return (ctx.strFrame == "" ? true : false);
  //┴
  } /* P2_MAKE_FRAME() */

  //─────────────────
  // ３．基本情報を取得
  //----------------------------------
  //【詳細】
  // フレーム書式    ：{認証コード}/{コマンドパス}!
  //─────────────────
  void P3_MAKE_INFO(){
  //┬
  //〇受信待ちデータの取り込み
  ctx.authCD  = P3_GET_TOKEN1(ctx.strFrame);
  ctx.cmdPath = P3_GET_TOKEN2(ctx.strFrame);
  //┴
  } /* P3_MAKE_INFO() */

  //─────────────────
  // ４．認証を実施
  //----------------------------------
  //【詳細】
  // 常時接続のため、認証は行わない
  //----------------------------------
  // 戻り値：論理値
  // ・false：認証に成功
  // ・true ：認証に失敗
  //─────────────────
  bool P4_AUTH(SLOT_HTTP&  argSS){
  //┬
  //◇┐認証開始要求に応答
  if (ctx.authCD == "_START_!") {
    //├┐（例外的に認証コードの中身が「接続開始コマンド」文字列の場合）
      //●認証管理に加える
      String newAuthCD = P4_START(AUTH_TBL);
      if(newAuthCD == ""){
        // ＼（管理開始に失敗した場合）
          //●エラーコードをレスポンス
          //▼RETURN:認証開始に失敗
          SEND_CONN(argSS, "#SS0!");
          return true;
      } /* END-if */
      //│
      //○認証コードを書き換え
      //●認証コードをレスポンス
      //▼RETURN:認証に失敗
      ctx.authCD = newAuthCD;
      SEND_CONN(argSS, String("$") + newAuthCD + "$");
      return true;
    //└┐（その他）
      //┴
  } /* END-if */
  //│
  //○ユーザ認証を実施
  ctx.authID = P4_GET_ID(AUTH_TBL, ctx.authCD);
  if (ctx.authID < 0) {
    // ＼（認証に失敗した場合）
      //●エラーコードをレスポンス
      //▼RETURN:認証に失敗
      SEND_CONN(argSS, "#SS1!");
      return true;
    } /* END-if */
    //┴
  //│
  //▼認証に成功
  return false;
  //┴
  } /* P4_AUTH() */


//========================================================
// Ｆ．ルーティング処理
//--------------------------------------------------------
// HTTPサーバが自動的に呼び出す ※registRoutes()参照
//========================================================
  //─────────────────
  // ルート１：ＭＭＰコマンド
  //----------------------------------
  // WebServerのリスナーが自動的に呼び出す
  //----------------------------------
  // 引数：
  // (参)接続情報スロット
  //─────────────────
  void routeMMP(SLOT_HTTP& argSS){
  //┬
  //○１．接続状態を確認
  if (P1_CONNECT(argSS)) return;
    // ＼（切断の場合）
      //▼処理を中断
  //│
  //●２．フレームを取得
  if (P2_MAKE_FRAME(argSS)) return;
    // ＼（フレームが未完成の場合）
      //▼処理を中断
  //│
  //○３．基本情報を取得
  P3_MAKE_INFO();
  //│
  //○４．認証を実施
  if (P4_AUTH(argSS)) return;
    // ＼（認証に失敗した場合）
      //▼処理を中断
  //│
  //●５．MMPコマンドを実行
  String resMMP = P5_RUN();
  //│
  //●６．実行結果をレスポンス
  SEND_CONN(argSS, resMMP);
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
// Ｇ．ポーリング
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
    //┬
    //○１．起動チェック
    if (ns_ACCEPTOR ) return true    ; // サーバの実体化有無を評価
    //│
    //○２．サーバ資源生成
    ns_ACCEPTOR = new WebServer(port);
    //│
    //○３．接続情報TBLを作成
    SS_CREATE_TBL()                  ; // 領域確保
    SS_ATTACH_SLOT()                 ; // 一時スロットを登録
    //│
    //○４．ルーティング登録
    registRoutes(*ns_ACCEPTOR )      ; // ルーティング登録
    //│
    //○５．サーバ開始
    ns_ACCEPTOR->begin()             ; // サーバ起動 ※ポインタ経由
    //│
    //▼６．正常終了
    return true;
    //┴
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
    //┬
    //○１．起動チェック
    if (!ns_ACCEPTOR ) return; // サーバの実体化有無を評価
    //│
    //○２．新規接続のスロットを登録
    //　➡【該当処理なし】※start()で登録済み
    //│
    //○┐３．ルーティング処理
      //●コンテクストをセットアップ
      P0_SETUP_CTX(ROUTE_ID, 0)  ; // コンテクストをセットアップ
      //│
      //○スロット処理をWEBサーバに指示
      ns_ACCEPTOR ->handleClient(); // 具体的な処理はregistRoutes()で定義済み
      //┴
    //┴
  } /* handle() */

} /* namespace adpHttp */