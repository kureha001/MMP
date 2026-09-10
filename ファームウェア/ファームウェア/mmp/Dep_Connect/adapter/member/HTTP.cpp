// filename : Dep_Connect/adapter/base/HTTP.cpp
//========================================================
// 接続部門／業務課／担当(標準型)：HTTP GET 担当
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/10)
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <WebServer.h>  // メインモード，サブモード
  #include <HTTPClient.h> // ブリッジモード
//┴┴

//========================================================
// 組織図
//========================================================
//┬
//□┐接続部門
  //□┐業務課
    //□担当
    #include "_index_.h"
//┴┴┴

//########################################################
//# 処理詳細
//########################################################
class AdapterHTTP : public AdapterBase {
public:
  //━━━━━━━━━━━━━━━━━
  // 抽象基底クラスからコンテクストを継承
  //━━━━━━━━━━━━━━━━━
  using AdapterBase::AdapterBase;

private:
//========================================================
// アダプタの基本
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 一般情報
  //━━━━━━━━━━━━━━━━━
    const int ADP_ID  = ADP_ID_HTTP;
    bool      IS_JSON = false;

  //━━━━━━━━━━━━━━━━━
  // サービス関連情報
  //━━━━━━━━━━━━━━━━━
#if (MODE == MODE_BRIDGE)
    HTTPClient  MY_NET          ; // HTTPクライアント(実体)
#else
    WebServer* MY_NET  = nullptr; // WEBサーバ(ポインタ)
    int        MY_PORT = 8080   ; // ポート番号
#endif

//========================================================
// レスポンス
//========================================================
#if (MODE != MODE_BRIDGE)
    //─────────────────
    // CORS許可用HTTPヘッダ追加
    //----------------------------------
    // ブラウザ上のJavaScriptから呼び出すための許可設定
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
#endif

  //─────────────────
  // JSON形式でレスポンス
  //─────────────────
  inline void SEND_JSON(const String& argJSON) {
#if (MODE != MODE_BRIDGE)
    //┬
    //○JSONをレスポンス
    ADD_CROSS(*MY_NET);
    MY_NET->send(200, "application/json; charset=utf-8", argJSON);
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
#endif
  } /* SEND_JSON() */

#if (MODE != MODE_BRIDGE)
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
  // クライアントに送信(JSON形式)
  //─────────────────
  struct JSON_DATA{
    bool    Res = false; // MMPの処理結果      {OK:true | NG:false}
    String  Msg = ""   ; // エラーMSG          {正常の場合は空}
    int     Val = -1000; // 戻値が数値の場合   {-999～9999、対象外は-1000 }
    String  Str = ""   ; // 戻値が文字列の場合 {４バイトの文字列、対象外は空}
  }; /* JSON_DATA */
  //─────────────────
  void SEND_CONN_JSON(){
    //┬
    //○前処理
    JSON_DATA jsDat ;
    String    js    ;
    String    msgID = ctx.resMSG;
    //│
    //◇┐JSON内容編集
    if (ctx.cmdPath == SP_CMD_START){
      //├┐（認証コード発行の場合）
        //○MSGIDを独自IDに書き換え
        //○取得値を文字列型にセット
        //○処理結果をセット
        msgID     = "!SS0!"   ; // 認証開始
        jsDat.Str = ctx.resMSG; // 取得値(文字列)
        jsDat.Res = true      ; // 正常
        //┴

    } else if (msgID == "!!!!!") {
      //├┐（正常系：戻り値なし の場合）
        //○処理結果を正常にセット
        jsDat.Res = true ; // 正常
        //┴

    } else {
      //└┐（その他）
        //◇┐データ型に応じて編集
        String body = msgID.substring(0, msgID.length()-1);
        if (SEND_IS_VALUE(body)) {
          //├┐（戻り値が数値型の場合）
            //○MSGIDを独自IDに書き換え
            //○処理結果をセット
            //●取得値を数値型にセット
            msgID = "!VAL!"                  ; // 数値型
            jsDat.Val = SEND_CONV_VALUE(body); // 取得値(数値)
            jsDat.Res = true                 ; // 正常
            //┴

        } else if (SEND_IS_STRING(msgID)) {
          //├┐（戻り値が文字列型の場合）
            //○MSGIDを独自IDに書き換え
            //○処理結果をセット
            //●取得値を数値型にセット
            msgID = "!STR!"                  ; // 文字列型
            jsDat.Str = ctx.resMSG           ; // 取得値(文字列)
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
    js += F(",\"source\":\"" ); js += ctx.resMSG.c_str(); js += '"' ; // MMPの戻り値
    js += F(",\"result\":"   ); js += (jsDat.Res ? "true" : "false"); // 処理結果：MMPコマンドの成功
    js += F(",\"message\":\""); js += jsDat.Msg; js += '"'          ; // メッセージ
    js += F(",\"value\":"    ); js += String(jsDat.Val)             ; // 戻値（数値）
    js += F(",\"string\":\"" ); js += jsDat.Str                     ; // 戻値（文字列）
    js += "\"}"               ;
    //│
    //○通信経路にJSON形式でレスポンス
    SEND_JSON(js);
    //┴
  } /* SEND_CONN_JSON() */
#endif

  //━━━━━━━━━━━━━━━━━
  // クライアントに送信(通常の5バイト)
  //━━━━━━━━━━━━━━━━━
  void SEND_CONN(){
#if (MODE != MODE_BRIDGE)
    //┬
    //○テキストをレスポンス
    ADD_CROSS(*MY_NET);
    MY_NET->send(200, "text/plain; charset=utf-8", ctx.resMSG);
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
#endif
  } /* SEND_CONN() */

#if (MODE != MODE_BRIDGE)
//========================================================
// リクエスト管理
//========================================================
  //─────────────────
  // リクエストの登録
  //----------------------------------
  // コールバック関数として機能
  //─────────────────
    //─────────────────
    // CORS事前確認
    //----------------------------------
    // ブラウザがアクセス前に送信するOPTIONS要求(プリフライト)へ応答
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

    //─────────────────
    // ルート０：ホスト直下
    //─────────────────
    void routeRoot(WebServer& srv){
        SEND_JSON(F("{"
        "\"ok\":true,"
        "\"result\":true,"
        "\"error\":\"\","
        "\"value\":-1,"
        "\"text\":\"MMP HTTP\""
        "}"));
    }
    //─────────────────
    // ルーティング登録
    //─────────────────
    void registRoutes(WebServer& server){
        //┬
        //○┐ルート０：ホスト直下の登録
          //●GETへの応答
          //●CORS事前確認へ応答
          server.on("/", HTTP_GET,     [&server, this](){routeRoot(server);});
          server.on("/", HTTP_OPTIONS, [&server, this](){route204(server); });
          //┴
        //│
        //○┐ルート１：ＭＭＰコマンドの登録
        server.onNotFound([&server, this](){
          //│
          //○ＭＭＰ処理へ渡す要求であるかを確認
          if (server.method() == HTTP_OPTIONS){route204(server); return;}
          //│＼（HTTP層で完結している）
          //│ ●CORS事前確認へ応答
          //│ ▼終了：早期リターン
          //│
          //◇┐レスポンスのスタイルを確認
          String strFrame = MY_NET->uri();
          if (strFrame.endsWith("!!")) {
          //├┐（フレームのスタイルがJSON指定の場合）
            //○JSONスタイルにセット
            //○フレームの末尾を補正する
            IS_JSON  = true;
            strFrame.remove(strFrame.length() - 1);
            //┴
          } else IS_JSON = false;
          //└┐（その他）
            //○標準スタイルにセット
            //┴
          //│
          //●コマンドを実行
          ctx.adpID    = ADP_ID;
          ctx.strFrame = strFrame;
          if (!ctx.strFrame.endsWith ("!")) ctx.strFrame += "!";
          if (ctx.strFrame.startsWith("/")) ctx.strFrame.remove(0, 1);
          ctx.resMSG   = ""  ; // レスポンスMSG
          ctx.cmdPath  = ""  ; // コマンドパス
          ctx.authCD   = ""  ; // 認証コード
          ctx.accID    = -1  ; // アクセスID
          //│
          //●モード別に後続処理
           if (MODE == MODE_MAIN) modeMain::RUN();
           if (MODE == MODE_SUB ) modeSub ::RUN();
          //│
          //●実行結果をレスポンス
          IS_JSON ? SEND_CONN_JSON() : SEND_CONN();
          //┴
        }); /* server.onNotFound */
        //┴
    }/* registRoutes() */
#endif

//========================================================
// 担務（公開機能）
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterHTTP(MmpContext& argCtx) : AdapterBase(argCtx) {
#if (MODE == MODE_BRIDGE)
    //┬
    //○メッセージ表示
    Serial.printf(" [OK] HTTP Client\n");
    //┴
#else
    //┬
    //○サービス資源を生成
    MY_NET = new WebServer(MY_PORT); // サーバ生成
    registRoutes(*MY_NET)           ; // ルーティング登録
    MY_NET->begin()                 ; // サーバ起動
    //│
    //○メッセージ表示
    Serial.printf(" [OK] WEB Server-> port %d\n", MY_PORT);
    //┴
#endif
  } /* constractor AdapterHTTP() */


#if (MODE == MODE_BRIDGE)
  //━━━━━━━━━━━━━━━━━
  // 転送受付
  //━━━━━━━━━━━━━━━━━
  void trans() {
    //┬
    //○リクエストを転送（HTTPクライアントを接続）
    String ip     = ctx.bridge.Dat1;
    String port   = ctx.bridge.Dat2;
    String cmd    = ctx.strFrame;
    String strURL = String("http://") + ip + ":" + port + "/" + cmd;
    MY_NET.begin(strURL);
    //│
    //●レスポンスを取得
    if (MY_NET.GET() <= 0) {
    //│＼（取得できない場合）
    //│ ○レスポンスMSGにエラーIDをセット
    //│ ▼終了：早期リターン
      ctx.resMSG = "#CNT!";
      return;
    } /* END-if */
    //│
    //○レスポンスを取得
    String strRes = MY_NET.getString();
    //│
    //○HTTPクライアントを切断
    MY_NET.end();
    //│
    //○レスポンスをコンテクストに反映
    ctx.strFrame = strRes;
    ctx.resMSG   = strRes;
    //┴
  };
#endif

  //━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━
  void handle() override {
#if (MODE == MODE_BRIDGE)
    //┬
    //○転送依頼を確認
    if (ctx.bridge.Stat != 1 || ADP_ID != ctx.bridge.adpID) return;
    //│＼（自分宛に転送依頼がない場合）
    //│ ▼終了：早期リターン
    //│
    //○進行状況を［処理中］にセット
    //●転送を受付
    ctx.bridge.Stat == 2;
    trans();
    //┴
#else
    //┬
    //○ルーティングを指示（その後も同期処理）
    MY_NET->handleClient();
    //┴
#endif
  } /* handle() */
}; /* class AdapterHTTP */
