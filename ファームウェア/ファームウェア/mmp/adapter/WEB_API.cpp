// filename : adapter/WEB_API.cpp
//========================================================
// 経路アダプタ：WEB API
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <WebServer.h> // ユーザ受付資源
  //┴
//┴

//########################################################
//# クラス：経路アダプタ(WEB API)
//########################################################
class AdapterWEB_API : public AdapterBase {
public:
  //━━━━━━━━━━━━━━━━━
  // 抽象基底クラスからコンテクストを継承
  //━━━━━━━━━━━━━━━━━
  using AdapterBase::AdapterBase;

private:
//========================================================
// Ａ．アダプタの基本
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // ステータス
    //─────────────────
    const int ADP_ID = ADP_ID_WAPI;

    //─────────────────
    // 使用するサービス
    //─────────────────
    static WebServer* ADP_SRV   ; // WEBサーバ
    static int        SRV_PORT  ; // ポート番号

//========================================================
// Ｂ．レスポンス
//========================================================
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

  //─────────────────
  // JSON形式でレスポンス
  //─────────────────
  inline void SEND_JSON(const String& argJSON) {
    //┬
    //○JSONをレスポンス
    ADD_CROSS(*ADP_SRV);
    ADP_SRV->send(200, "application/json; charset=utf-8", argJSON);
    //│
    //●ログ出力
    adpBase::SHOW_LOG();
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
  void SEND_CONN(bool argMode){
    //┬
    //○動作モードを確認
    if (!argMode && ctx.sysMode != MODE_MAIN) return;
    //│＼（出力制限がなく、メインモード以外の場合）
    //│ ▼終了：早期リターン
    //│
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
  } /* SEND_CONN() */

//========================================================
// Ｃ．リクエスト管理
//========================================================
  //─────────────────
  // リクエストの登録
  //----------------------------------
  // コールバック関数として機能
  //─────────────────
    //─────────────────
    // CORS事前確認
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

    //─────────────────
    // ルート０：ホスト直下
    //─────────────────
    void routeRoot(WebServer& srv){
        SEND_JSON(F("{"
        "\"ok\":true,"
        "\"result\":true,"
        "\"error\":\"\","
        "\"value\":-1,"
        "\"text\":\"MMP WEB API\""
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
          //●コマンドを実行
          adpBase::RUN(ADP_ID, ADP_SRV->uri());
          //│
          //●実行結果をレスポンス
          SEND_CONN(false);
          //┴
        }); /* server.onNotFound */
        //┴
    }/* registRoutes() */

//========================================================
// Ｄ．データ受信
//========================================================

//========================================================
// Ｅ．公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterWEB_API(MmpContext& argCtx) : AdapterBase(argCtx) {
    //┬
    //○サービス資源を生成
    ADP_SRV = new WebServer(SRV_PORT); // サーバ生成
    registRoutes(*ADP_SRV)           ; // ルーティング登録
    ADP_SRV->begin()                 ; // サーバ起動
    //│
    //○メッセージ表示
    Serial.println(String(" [OK] WEB API   -> port ") + String(SRV_PORT));
    //┴
  } /* constractor AdapterWEB_API() */

  //━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━
  void handle() override {
    //┬
    //○ルーティングを指示（その後も同期処理）
    ADP_SRV->handleClient();
    //┴
  } /* handle() */

}; /* class AdapterWEB_API */


//########################################################
//# スタティック資源の実体
//########################################################
//┬
//■サーバ／サービス
WebServer* AdapterWEB_API::ADP_SRV = nullptr; // サーバ
int        AdapterWEB_API::SRV_PORT = 8080  ; // サービス・ポート
//│
//■送受信バッファ
//│
//■スレッド／コールバック
//│
//■リクエスト
//┴
