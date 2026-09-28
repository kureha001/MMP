// filename : Dep_Connect/adapter/HTTP/HTTP.cpp
//========================================================
// 接続部門／担当：HTTP（ベース：メイン・サブ モード）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/28)
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <WebServer.h>
//┴┴

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス【基本型】
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class  AD_HTTP :
public AD_API
{
private:
//========================================================
//§基本情報
//========================================================
  int        MY_PORT = 8080   ; // ポート番号
  WebServer* MY_NET  = nullptr; // WEBサーバ(ポインタ)

//========================================================
//§最終処理
//========================================================
  //─────────────────
  // CORS許可用HTTPヘッダ追加
  //----------------------------------
  // ブラウザ上のJavaScriptから呼び出すための許可設定
  // → Webブラウザのセキュリティ制約(CORS)を通過させる
  //─────────────────
  inline void ADD_CROSS(WebServer& argSrv) {
    //┬
    //○アクセス元Webページ　 ：制限なし
    //○HTTPメソッド　　　　　：データ取得・事前確認
    //○HTTPリクエストヘッダ　：データ形式・JavaScript(Ajax)向け識別・認証情報
    //○CORS確認結果の記憶時間：600秒=10分
    argSrv.sendHeader("Access-Control-Allow-Origin", "*");
    argSrv.sendHeader("Access-Control-Allow-Methods", "GET,OPTIONS");
    argSrv.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Requested-With, Authorization");
    argSrv.sendHeader("Access-Control-Max-Age", "600");
    //┴
  } /* ADD_CROSS() */

  //──────────────────
  //➡メイン
  //・JSONレスポンスに対応
  #if (MODE == MODE_MAIN)
  //------------------------------------
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // JSON形式でレスポンス
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
    //──────────────────────────
    // 接続元にSON形式でレスポンスする
    //──────────────────────────
    // JSON形式でレスポンス
    inline void JSON_SEND(const String& argJSON) {
      //┬
      //○JSONをレスポンス
      ADD_CROSS(*MY_NET);
      MY_NET->send(200, "application/json; charset=utf-8", argJSON);
      //│
      //●ログ出力
      adpFnBase::SHOW_LOG();
      //┴
    } /* JSON_SEND() */

    //──────────────────────────
    // コマンド実行結果の型判定(数値型)
    //──────────────────────────
    static bool JSON_IS_VAL(const String& argBody){
      if (argBody.length() != 4) return false;
      int start = (argBody[0]=='-') ? 1 : 0;
      for (int i=start; i<4; ++i){
        if (!isDigit((unsigned char)argBody[i])) return false;
      } // for
      return true;
    } /* JSON_IS_VAL() */

    //──────────────────────────
    // コマンド実行結果の型判定(文字列型)
    //──────────────────────────
    static bool JSON_IS_STR(const String& argBody){
      if (argBody.startsWith("#")) return false;
      if (argBody.startsWith("!")) return false;
      return true;
    } /* JSON_IS_STR() */

    //──────────────────────────
    // コマンド実行結果を変換(数値)
    //──────────────────────────
    static int JSON_CONV_VAL(const String& argBody){
      bool neg = (argBody[0]=='-');
      int v = 0;
      for (int i = neg ? 1 : 0; i < 4; ++i) v = v*10 + (argBody[i]-'0');
      return neg ? -v : v;
    } /* JSON_CONV_VAL() */

    //──────────────────────────
    // レスポンスMSGに該当する説明文を取得
    //──────────────────────────
    static const char* JSON_MSG(const String& argID){

      // 共通のコード
      if (argID == RCD::OK      ) return "OK:戻り値無し"            ;
      if (argID == RCD::NotMod  ) return "NG:機能モジュールが無い"  ;
      if (argID == RCD::NotCmd  ) return "NG:コマンド名が不正"      ;
      if (argID == RCD::ChkErr  ) return "NG:引数チェックで違反"    ;
      if (argID == RCD::IniErr  ) return "NG:データが未初期化"      ;
      if (argID == RCD::DevErr  ) return "NG:使用不可のデバイス"    ;
      if (argID == RCD::FilErr  ) return "NG:ファイル操作が異常終了";
      if (argID == RCD::NoDErr  ) return "NG:データ項目名が不正"    ;
      if (argID == RCD::ValErr  ) return "NG:数値が基底範囲外"      ;

      // ユーザ認証のコード
      if (argID == RCD::AuthErr1) return "NG:認証管理の開始に失敗"  ;
      if (argID == RCD::AuthErr2) return "NG:認証に失敗"            ;

      // アダプタ独自のコード
      if (argID == RCD::OK_Auth ) return "OK:ユーザ認証に成功"      ;
      if (argID == RCD::OK_VAL  ) return "OK:数値"                  ;
      if (argID == RCD::OK_STR  ) return "OK:文字列"                ;
  
      return "NG:その他のエラー";
    } /* JSON_MSG() */

    //──────────────────────────
    // 接続元にMSGをJSON形式でレスポンスする
    //──────────────────────────
    struct JSON_DATA{
      bool    Res = false; // MMPの処理結果      {OK:true | NG:false}
      String  Msg = ""   ; // エラーMSG          {正常の場合は空}
      int     Val = -1000; // 戻値が数値の場合   {-999～9999、対象外は-1000 }
      String  Str = ""   ; // 戻値が文字列の場合 {４バイトの文字列、対象外は空}
    }; /* JSON_DATA */
    //─────────────────
    void SEND_CONN_JSON(){
      //┬
      //○【前処理】
      JSON_DATA jsDat ;
      String    js    ;
      String    msgID = ctx.base.Msg;
      //│
      //◇┐JSON内容編集
      if (ctx.base.Cmd == SP_CMD_START){
        //├┐（認証コード発行の場合）
          //○MSGIDを独自IDに書き換え
          //○取得値を文字列型にセット
          //○処理結果をセット
          msgID     = RCD::OK_Auth; // 認証開始
          jsDat.Str = ctx.base.Msg; // 取得値(文字列)
          jsDat.Res = true        ; // 正常
          //┴

      } else if (msgID == RCD::OK) {
        //├┐（正常系：戻り値なし の場合）
          //○処理結果を正常にセット
          jsDat.Res = true ; // 正常
          //┴

      } else {
        //└┐（その他）
          //◇┐データ型に応じて編集
          String body = msgID.substring(0, msgID.length()-1);
          if (JSON_IS_VAL(body)) {
            //├┐（戻り値が数値型の場合）
              //○MSGIDを独自IDに書き換え
              //○処理結果をセット
              //●取得値を数値型にセット
              msgID = RCD::OK_VAL            ; // 数値型
              jsDat.Val = JSON_CONV_VAL(body); // 取得値(数値)
              jsDat.Res = true               ; // 正常
              //┴

          } else if (JSON_IS_STR(msgID)) {
            //├┐（戻り値が文字列型の場合）
              //○MSGIDを独自IDに書き換え
              //○処理結果をセット
              //●取得値を数値型にセット
              msgID = RCD::OK_STR              ; // 文字列型
              jsDat.Str = ctx.base.Msg         ; // 取得値(文字列)
              jsDat.Res = true                 ; // 正常
              //┴

          } else {
            //└┐（その他）
              //○処理結果をセット
              jsDat.Res = false                 ; // 異常
              //┴
          } //～if 
          //┴
      } //～if 
      //│
      //○メッセージを取得
      jsDat.Msg = JSON_MSG(msgID);
      //│
      //○JSON形式に編集
      js.reserve(160) ; // 予備確保
      js += F("{\"ok\":true"   )                                        ; // 処理結果：HTTP通信の成功
      js += F(",\"source\":\"" ); js += ctx.base.Msg.c_str(); js += '"' ; // MMPの戻り値
      js += F(",\"result\":"   ); js += (jsDat.Res ? "true" : "false")  ; // 処理結果：MMPコマンドの成功
      js += F(",\"message\":\""); js += jsDat.Msg; js += '"'            ; // メッセージ
      js += F(",\"value\":"    ); js += String(jsDat.Val)               ; // 戻値（数値）
      js += F(",\"string\":\"" ); js += jsDat.Str                       ; // 戻値（文字列）
      js += "\"}"               ;
      //│
      //○通信経路にJSON形式でレスポンス
      JSON_SEND(js);
      //┴
    } /* SEND_CONN_JSON() */
  //------------------------------------
  #endif //➡メイン
  //──────────────────

  //───────────────────────────
  // 接続元にMSGをレスポンスする
  //───────────────────────────
  void SEND_CONN() {
  //┬
  //○┐【前処理】
    //●WiFiの接続状況を確認する
    if (!devWiFi::ENABLED_CONN(true)) return;
    //│＼（機能していない場合）
    //│ ▼終了：早期リターンする
    //┴
  //│
  //○┐【主処理】
    //○接続元にレスポンスMSGを送信する
    ADD_CROSS(*MY_NET);
    MY_NET->send(200, "text/plain; charset=utf-8", ctx.base.Msg);
    //┴
  //│
  //○┐【後処理】
    //●ログ出力
    adpFnBase::SHOW_LOG();
    adpFnBase::SHOW_LOG();
  //┴
  } /* SEND_CONN() */

//========================================================
//§受信処理
//========================================================
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
    JSON_SEND(F("{"
      "\"ok\":true,"
      "\"result\":true,"
      "\"error\":\"\","
      "\"value\":-1,"
      "\"text\":\"MMP HTTP\""
      "}"));
  } /* routeRoot() */

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
      //│ ▼終了：早期リターンする
      //│
      //○フレーム求める
      String retFrame = MY_NET->uri();
      //│
      //◇┐レスポンス形式を求める
      bool isJSON = false;
      if (retFrame.endsWith("@!")) {
      //├┐（JSON形式が指定されている場合）
        //○JSON形式にセット
        //○フレーム末尾の"#"を削除する
        isJSON = true;
        retFrame.remove(retFrame.length() - 2);
        retFrame += "!";
        //┴
      } else isJSON = false;
      //└┐（その他）
        //○標準形式にセット
        //┴
      //│
      //●フレームに従いコンテキストを初期化する
      adpFnBase::SETUP_CTX(MY_AID, retFrame);
      //│
  //──────────────────
  //➡メイン
  //・モード別の主処理
  //・JSON／TXTの選択が可能
  #if (MODE == MODE_MAIN)
  //------------------------------------
      //●ＭＭＰコマンドを実行
      //●実行結果をレスポンス
      modeMain::RUN();
      isJSON ? SEND_CONN_JSON() : SEND_CONN();
  //──────────────────
  //➡サブ
  //・モード別の主処理
  //・TXTのみ
  #elif (MODE == MODE_SUB)
  //------------------------------------
      //●ＭＭＰコマンドを実行
      //●実行結果をレスポンス
      modeSub::RUN();
      SEND_CONN();
  //------------------------------------
  #endif //➡メイン｜➡サブ
  //──────────────────
      //┴
    }); /* this{}/onNotFound() */
    //┴
  }/* registRoutes() */

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // コンストラクタ【基本型】
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  AD_HTTP(MmpContext& argCtx) :
  AD_API(argCtx, AID::HTTP) 
  {
  //┬
  //○┐【前処理】
    //●前処理（一般用）を実行する...進行判定を得る
    if (SETUP_NORMAL()) return;
    //│＼（異常の場合）
    //│ ▼終了：早期リターンする
    //┴
  //│
  //○┐【主処理】
    //○サービス資源を生成
    MY_NET = new WebServer(MY_PORT); // サーバ生成
    registRoutes(*MY_NET)           ; // ルーティング登録
    MY_NET->begin()                 ; // サーバ起動
    //┴
  //│
  //○┐【後処理】
    //○メッセージ表示
    char msg[128];
    snprintf(msg, sizeof(msg), " [OK] HTTP / PORT.%d", MY_PORT);
    Log::prtln(String(msg));
  //┴┴
  } /* constractor AD_HTTP() */

  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  void handle() override {
  //┬
  //○┐【前処理】
    //●WiFiの接続状況を確認する
    if (!devWiFi::ENABLED_CONN(true)) return;
    //│＼（異常の場合）
    //│ ▼終了：早期リターン
    //┴
  //│
  //○┐【主処理】
    //●ルーティングを指示（その後も同期処理）
    MY_NET->handleClient();
    //┴
  //│
  //○┐【後処理】
  //┴┴
  } /* handle() */

}; /* class AD_HTTP */
