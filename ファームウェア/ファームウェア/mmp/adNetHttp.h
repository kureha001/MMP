// filename : adNetHttp.h
//========================================================
// 通信アダプタ：ＷｅｂＡＰＩ
//--------------------------------------------------------
// ・HTTP(WEB API)サーバの起動
// ・WEB APIのポーリング処理
//   受信→認証開始→認証検証→コマンド実行→結果返却(JSON)
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/07) α版 
// ・ファイル名を変更
// ・ファイル名を変更
// ・全体的にリファクタリング
// ・各種リソース(処理/構造体など)を共通側に定義
// ・セッション管理方式を大幅に変更
// ・ユーザIDを関数で求めるよう修正
//========================================================
#pragma once
#include <WebServer.h>
#include "ad.h"  // 通信アダプタ共通

//━━━━━━━━━━━━━━━━━
// グローバル資源(宣言)
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 統一入口：fnPerser.hで定義
  //─────────────────
  extern String MMP_REQUEST(const String& cmdPath, int usrID);

  //─────────────────
  // サーバ情報
  //─────────────────
  //　➡【該当処理なし】

//========================================================
// メイン処理
//========================================================
namespace {
//━━━━━━━━━━━━━━━━━
// ネームスペース資源(宣言)
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 通信の基本情報
  //─────────────────
  static const int ROUTE_ID      = ROUTE_ID_HTTP;
  WebServer*       ns_ACCEPTOR   = nullptr; // ユーザ受付資源の種類を定義(WEBサーバ)

//━━━━━━━━━━━━━━━━━
// 接続管理
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 接続管理TBL
  //─────────────────
  static SLOT_HTTP* ssTBL = nullptr;

  //─────────────────
  // スロットを開放(キー：接続ID)
  //─────────────────
  //　➡【該当処理なし】

  //─────────────────
  // スロットを開放(キー：認証コード)
  //─────────────────
  //　➡【該当処理なし】

  //─────────────────
  // 空きスロットを照会
  //─────────────────
  //　➡【該当処理なし】

  //─────────────────
  // 接続管理TBLを作る
  //----------------------------------
  // 戻り値：なし
  //─────────────────
  void CREATE_SS_TBL(){
  //┬
  //○領域サイズをセット
  int maxCount = 1;  // 容量：一時データ1個
  //│
  //○領域を確保
  ssTBL = new SLOT_HTTP[maxCount];
  //│
  //◎┐TBL全体を初期化
  for (int id = 0; id < maxCount ; id++) INIT_SLOT_HTTP(ssTBL[id]);
    //●このスロットを初期化
    //┴
  //┴
  } /* CREATE_SS_TBL() */

  //─────────────────
  // 接続を接続スロットに割り当てる
  //----------------------------------
  // 開始処理で「一度だけ」実行すること
  //─────────────────
  void ATTACH_SS_SLOT(){
  //┬
  //○スロットに新規接続を登録
  ssTBL[0].used = true        ; // 管理スロットを使用中へ変更
  ssTBL[0].conn = ns_ACCEPTOR ; // HTTP受付資源を管理スロットへ登録
  //┴
  } /* ATTACH_SS_SLOT() */

//━━━━━━━━━━━━━━━━━
// ヘルパー機能
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // CORS許可用HTTPヘッダ追加
  //----------------------------------
  // ブラウザ上のJavaScriptからWeb APIを呼び出すための許可設定
  // → Webブラウザのセキュリティ制約(CORS)を通過させる
  //─────────────────
  inline void add_cors(WebServer& argSrv) {
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
  } /* add_cors() */

  //─────────────────
  // CORS事前確認へ応答
  //----------------------------------
  // ブラウザがWeb APIアクセス前に送信するOPTIONS要求(プリフライト)へ応答
  // → CORS許可ヘッダを付加してブラウザへ許可情報を通知
  // → 本通信で返すデータはないためHTTPステータス204を返却
  //─────────────────
  inline void send_204(WebServer& argSrv) {
    //┬
    //●CORS許可用HTTPヘッダ追加
    add_cors(argSrv);
    //│
    //○HTTPステータスを返却
    argSrv.send(204); // 豆知識{200:返すデータあり｜204:返すデータなし}
    //┴
  } /* send_204() */

  //─────────────────
  // JSON形式でレスポンス
  //─────────────────
  inline void send_json(WebServer& argSrv, const String& argJSON, int argCode=200) {
    add_cors(argSrv);
    argSrv.send(argCode, "application/json; charset=utf-8", argJSON);
  } /* send_json() */

  //─────────────────

  //─────────────────
  // MMPエラーコードを表示用メッセージへ変換
  //─────────────────
  static const char* map_error(const String& argErrID){
    if (argErrID == "#URI!") return "ERR:URIが不正"             ;
    if (argErrID == "#CMD!") return "ERR:コマンド名が不正"      ;
    if (argErrID == "#CHK!") return "ERR:引数チェックで違反"    ;
    if (argErrID == "#INI!") return "ERR:データが未初期化"      ;
    if (argErrID == "#DEV!") return "ERR:使用不可のデバイス"    ;
    if (argErrID == "#FIL!") return "ERR:ファイル操作が異常終了";
    if (argErrID == "#NOD!") return "ERR:データ項目名が不正"    ;
    if (argErrID == "#SID!") return "ERR:未定義の経路ID"        ;
    if (argErrID == "#FUL!") return "ERR:認証スロットが満杯"    ;
    if (argErrID == "#KEY!") return "ERR:認証キーが不正"        ;
    if (argErrID == "#RET!") return "ERR:コマンド戻値が破損"    ;
    return "ERR:その他のエラー";
  } /* map_error() */

  //─────────────────
  // MMPコマンドのレスポンスが数値であるかを判定
  //─────────────────
  static bool isDec4Signed(const String& argBody){
    if (argBody.length() != 4) return false;
    int start = (argBody[0]=='-') ? 1 : 0;
    for (int i=start; i<4; ++i){
      if (!isDigit((unsigned char)argBody[i])) return false;
    } // for
    return true;
  } /* isDec4Signed() */

  //─────────────────
  // MMPコマンドのレスポンスから数値を取得
  //─────────────────
  static int parseDec4Signed(const String& argBody){
    bool neg = (argBody[0]=='-');
    int v = 0;
    for (int i = neg ? 1 : 0; i < 4; ++i) v = v*10 + (argBody[i]-'0');
    return neg ? -v : v;
  } /* parseDec4Signed() */

  //─────────────────
  // ユーザにレスポンス(JSON形式)
  //----------------------------------
  // タイムスタンプ更新あり(認証情報側)
  //─────────────────
  static void sendJson(
    bool          pRes   , // 処理結果             {OK:true | NG:false}
    const String& pErr   , // エラーMSG            {正常の場合は空}
    int           pVal   , // 戻値が数値の場合     {-999～9999、対象外は-1000 }
    const String& pTxt   ,  // 戻値が文字列の場合   {4バイトの文字列、対象外は空}
    const String& pAuthCD = ""
  ){
  //┬
  //○前処理
  String js       ; // JSON文字列
  js.reserve(160) ; // 予備確保
  //│
  //○JSON形式に編集
  js += F("{\"ok\":true"  )                                 ; // 処理結果：HTTP通信の成功
  js += F(",\"result\":"  ); js += (pRes ? "true" : "false"); // 処理結果：MMPコマンドの成功
  js += F(",\"error\":\"" ); js += pErr; js += '"'          ; // エラーMSG
  js += F(",\"value\":"   ); js += String(pVal)             ; // 戻値（数値）
  js += F(",\"text\":\""  ); js += pTxt                     ; // 戻値（文字列）
  js += "\"}"              ;
  //│
  //○通信経路にJSON形式でレスポンス
  send_json(*argSS.conn , js);
  //│
  //○┐後処理
    //●タイムスタンプを更新
    if (pAuthCD != "") UpdateAuthSlot(ROUTE_ID, pAuthCD); // 認証情報側
    //┴
  //┴
  } /* sendJson() */
  //─────────────────

//━━━━━━━━━━━━━━━━━
// ルート別の処理
//----------------------------------
// 実行元：registRoutes()
//----------------------------------
// ルーティング登録に従いHTTPサーバが実行
// handle()の「3) ルーティング処理」を参照
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ルート０：ホスト直下
  //----------------------------------
  // 引数：
  // ・OBJ(参)ストリームO：通信ストリームのオブジェクト
  //─────────────────
  void routeRoot(WebServer& srv){
    send_json(srv, F("{"
      "\"ok\":true,"
      "\"result\":true,"
      "\"error\":\"\","
      "\"value\":-1,"
      "\"text\":\"MMP WEB API\""
      "}"));
  } /* routeRoot() */

  //─────────────────
  // ルート１：ＭＭＰコマンド
  //----------------------------------
  // 引数：
  // (参)接続管理スロット
  //----------------------------------
  //【マルチ接続・一時型】
  // WebAPIの各HTTPリクエストを1ユーザとして扱う。
  // 通信状態はリクエスト処理中のみ保持する。
  // 都度接続のため、ユーザー認証を行う。
  //----------------------------------
  //・フレーム書式 ：認証コード/コマンドパス!
  //・状態管理     ：WebAPI経路で1スロット（共有）
  //・認証機能
  //　├ 接続スロット：あり(１つ：一時利用)
  //　└ 認証コード  ：あり
  //─────────────────
  void routeMMP(SLOT_HTTP& argSS){
  //┬
  //○┐０．前処理
    //○0-1.処理継続可否を確認
    //　➡【該当処理なし】※WEBサーバのリスナーが実行するので確認は不要
    //│
    //○0-2.準備完了フラグを用意
    bool isReady = false;
    //│
    //○0-3.レスポンス内容の編集ワークを用意
    //○0-4.レスポンス内容の編集ワークを用意
    bool    jResult  = false; // MMPの処理結果      {OK:true | NG:false}
    String  jError   = ""   ; // エラーMSG          {正常の場合は空}
    int     jValue   = -1000; // 戻値が数値の場合   {-999～9999、対象外は-1000 }
    String  jText    = ""   ; // 戻値が文字列の場合 {４バイトの文字列、対象外は空}
    //┴
  //│
  //◎┐１．受信待ちデータの取り込み
    //○1-1.取り込みの継続を確認
    // ＼（受信待ちデータがない場合）
      //▼取り込みを終了
    //│
    //○1-2.受信バッファに受信データを加える
    String ch = argSS.conn.uri();
    if (argSS.rx.length() < SS_RX_SIZE) {argSS.rx += ch;}
    else {
    // ＼（受信バッファが許容サイズを超過した場合）
      //○データ容量超過状態へ移行
      argSS.isOverflow = true;
      //│
      //○受信バッファの内容を破棄
      argSS.rx         = "";
      //┴
    } /*【1-2.受信バッファに１バイト取り込み】*/
    //│
    //○┐1-3.フレーム完成時の処理フローを制御
      //├→（コマンド終端を検出した場合）
        //◇┐1-3-1.処理フローを制御
        if (!argSS.isOverflow) {
        //├→（容量超過状態ではない場合）
          //○準備完了フラグを[ON]
          isReady = true;
          //┴
        //└┐（その他；受信バッファ溢れ中の場合）
          //○データ容量超過状態を解除
          argSS.isOverflow = false;
          //│
          //○受信バッファをクリア
          argSS.rx         = ""   ;
          //│
          //○┐エラーコードをレスポンス
            //○レスポンス内容を編集
            //○ユーザにレスポンス
            jError = map_error("#DFL!");
            sendJson(jResult, jError, jValue, jText, "");
            //┴
          //│
          //▼ルーティング処理を中断
          return;
        } /*【1-3-1.処理フローを制御】*/
    //┴
  //│
  //○┐２．事前準備
    //○2-1.後続処理の継続を判断
      if (!isReady) return;
    // ＼（準備が未完了の場合）
      //▼ルーティング処理を中断
    //│
    //●2-2.受信バッファをURI形式に変換
    FormatURI(argSS.rx);
    //│
    //○┐2-3.事前データを用意
      //○2-3-1.認証コードを取得
      //○2-3-2.コマンドパスを取得
      String authCD  = GetToken1(argSS.rx);
      String cmdPath = GetToken2(argSS.rx);
      //┴
    //│
    //○2-4.受信バッファを破棄
    argSS.rx = "";
    //┴
  //│
  //○┐３．認証を実施
    //○3-1.認証開始要求を処理
    if (authCD == "_START_!") {
      //├→（例外的に認証コードの中身が「接続開始コマンド」の文字列の場合）
        //●3-1-1.この接続を認証管理に加える
        String newAuthCD = AUTH_START(argRID);
        if(newAuthCD == ""){
        // ＼（管理開始に失敗した場合）
          //○┐エラーコードをレスポンス
            //○レスポンス内容を編集
            //○ユーザにレスポンス
            jError = map_error("#FUL!");
            sendJson(jResult, jError, jValue, jText, newAuthCD);
            //┴
          //│
          //▼ルーティング処理を中断
          return;
        } /* end-if */
        //│
        //○3-1-2.認証コードをスロットに反映
        argSS.authCD = newAuthCD;
        //│
        //○┐3-1-3.認証コードをレスポンス
          //○レスポンス内容を編集
          //○ユーザにレスポンス
          jResult = true     ; // 正常
          jText   = newAuthCD; // 認証コード
          argSS.conn .print(newAuthCD);
          //┴
        //│
        //▼3-1-4.ルーティング処理を中断
        return;
      //┴
    } /*【3-1.この接続を認証管理の対象に登録】*/
    //│
    //○┐3-2.認証の実施
      //●3-2-1.認証情報と照合
      int authID = AUTH_GET_ID(ROUTE_ID, authCD);
      if (authID < 0) {
      // ＼（存在しない場合）
        //○┐エラーコードをレスポンス
          //○レスポンス内容を編集
          //○ユーザにレスポンス
          jError = map_error("#KEY!");
          sendJson(jResult, jError, jValue, jText, authCD);
          //┴
        //│
        //▼ルーティング処理を中断
        return;
      } /*【3-2-1.認証情報と照合】*/
      //│
      //○3-2-2.接続管理と照合
      //　➡【該当処理なし】
      //┴
    //┴
  //│
  //○┐４．MMPコマンドを実行
    //○┐4-1.コマンドを実行
      //●対象ユーザを特定
      //●コマンドパーサーへ処理を移譲
      int usrID = GET_USER_ID(ROUTE_ID, authID); // ※認証情報から照会
      String mmpResp = MMP_REQUEST(cmdPath, usrID);
      //┴
    //│
    //○┐4-2.実行結果をレスポンス
      //◇┐4-2-1.レスポンス内容を編集
      if (mmpResp.length() < 1 || mmpResp[mmpResp.length()-1] != '!') {
      //├→（異常系：データ欠損 の場合）
        //○処理結果
        //●エラーMSG
        jResult = false             ; // 異常
        jError  = map_error("#RET!");
        //┴
      } else if (mmpResp == "!!!!!") {
      //├→（正常系：戻り値なし の場合）
        //○処理結果
        jResult = true ; // 正常
        //┴
      } else if (mmpResp.length() == 5 && mmpResp[0] == '#') {
      //├→（異常系：各種エラー の場合）
        //○処理結果
        //●エラーMSG
        jResult = false          ; // 異常
        jError  = map_error(mmpResp);
        //┴
      } else {
      //└┐（その他）
        //◇┐データ型に応じて編集
        String body = mmpResp.substring(0, mmpResp.length()-1);
        if (isDec4Signed(body)) {
        //├→（正常系：戻り値が数値）
          //○処理結果
          //●数値
          jResult = true                 ; // 正常
          jValue  = parseDec4Signed(body); // 取得値(数値)
          //┴
        } else {
        //└┐（その他；正常系：戻り値が文字列）
          //○処理結果
          //○テキスト値
          jResult = true ; // 正常
          jText   = body ; // 取得値(文字列)
          //┴
        } /* end-if */
        //┴
      } /* end-if */
      //│
      //○4-2-2.ユーザにレスポンス
      sendJson(jResult, jError, jValue, jText, authCD);
      //┴
    //┴
  //┴
  } /* routeMMP() */

//━━━━━━━━━━━━━━━━━
// ルーティング登録
//----------------------------------
// 実行元：start()
//─────────────────
// 処理分岐はHTTPサーバが自動的におこなう
// その為、プログラムロジックで明示的に呼び出す必要はない
// ここでは要求毎の処理手続きをHTTPサーバへ登録する
// HTTPサーバは、要求条件に一致した登録内容だけを実行する
//━━━━━━━━━━━━━━━━━
  void registRoutes(WebServer& server){
    //┬
    //○┐ルート０：ホスト直下の登録
      //●GETへの応答
      //●CORS事前確認へ応答
      server.on("/", HTTP_GET,     [&server](){routeRoot(server);});
      server.on("/", HTTP_OPTIONS, [&server](){send_204(server); });
      //┴
    //│
    //○┐ルート１：ＭＭＰコマンドの登録
    server.onNotFound( [&server](){ // NotFound("/"以外)が処理対象
      //○ＭＭＰ処理へ渡す要求であるかを確認
      if (server.method() == HTTP_OPTIONS){
      // ＼（HTTP層で完結している）
        //●CORS事前確認へ応答
        send_204(server);
        //▼RETURN
          return;
      }   /* if */
      //│
      //●ＭＭＰコマンドへの応答
      routeMMP(ssTBL[0]);
      //┴
      } /* [&server]()       */
    );  /* server.onNotFound */
    //┴
  }     /* registRoutes()    */
  //─────────────────

} /* namespace(匿名) */


//========================================================
// ハンドラ関連処理
//========================================================
namespace srvHttp {
  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //----------------------------------
  // 実行元：adNet.h - InitNet_Service()
  //━━━━━━━━━━━━━━━━━
  bool start(uint16_t port) {

    // 1) 二重起動防止
    if (ns_ACCEPTOR ) return true     ; // ユーザ受付が準備済みか

    // 2) サーバを起動
    ns_ACCEPTOR  = new WebServer(port); // HTTP要求受付資源を生成
    registRoutes(*ns_ACCEPTOR )       ; // ルーティング登録
    ns_ACCEPTOR ->begin()             ; // サーバ起動 ※ポインタ経由

    // 3) 接続管理TBLを作成
    CREATE_SS_TBL()  ; // 領域確保
    ATTACH_SS_SLOT() ; // 一時スロットとしてスロットを固定

    // 4) 正常終了
    return true;
  } /* start() */

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口（ポーリング入口）
  //----------------------------------
  // 実行元：mmp.ino - loop()
  //----------------------------------
  // サーバが機能していることが条件
  // 明示的にルーティング指示しない
  // サーバが必要に応じてルーティング登録した内容に従う
  //━━━━━━━━━━━━━━━━━
  void handle() {

    // 1) 起動チェック
    if (!ns_ACCEPTOR ) return; // サーバの実体化有無を評価

    // 2) {新規接続時｜再接続時}の接続管理スロット作成
    //　➡【該当処理なし】

    // 3) ルーティング処理
    //　➡ WebServer自身が受付・解析・分岐まで担当(内容はregistRoutes()で定義済み)
    ns_ACCEPTOR ->handleClient(); // 具体的な処理はregistRoutes()で定義済み

} /* handle() */

} /* namespace srvHttp */