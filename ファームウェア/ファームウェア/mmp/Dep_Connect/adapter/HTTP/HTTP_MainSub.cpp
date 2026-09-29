// filename : Dep_Connect/adapter/HTTP/HTTP_MainSub.cpp
//========================================================
// 接続部門／担当：HTTP（メイン・サブ モード）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/29)
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <WebServer.h>
//┴┴

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス【基本型】
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class  AD_HTTP : // 接続識別子：ダミー
public AD_API<String> // 基本型
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

  //───────────────────────────
  // 終了処理：接続元にレスポンスMSGを送信する
  //───────────────────────────
  void SEND_CONN(String argConn_Dummy) override final {
  //┬
  //○┐【前処理】
    //●WiFiの接続状況を確認する
    if (!devWiFi::ENABLED_CONN(true)) return;
    //│＼（機能していない場合）
    //│ ▼終了：早期リターンする
    //┴
  //│
  //○┐【主処理】
    //●CORS許可用HTTPヘッダ追加
    //○接続元にレスポンスMSGを送信する
    ADD_CROSS(*MY_NET);
    MY_NET->send(200, "text/plain; charset=utf-8", ctx.base.Msg);
    //┴
  //│
  //○┐【後処理】
    //●ログ出力
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
    //  ※200:返すデータあり｜204:返すデータなし
    //argSrv.send(204, "text/plain", "");
    argSrv.send(200, "text/plain; charset=utf-8", RCD::NotMod);
    //┴
  } /* route204() */

  //─────────────────
  // ルーティング登録
  //─────────────────
  void registRoutes(WebServer& server){
    //┬
    //○┐ルート０：ホスト直下の登録
      //●GETへの応答
      //●CORS事前確認へ応答
      server.on("/", HTTP_GET,     [&server, this](){route204(server);});
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
        //○JSON形式をセットする
        //○フレーム末尾の"@"を削除する
        isJSON = true;
        retFrame.remove(retFrame.length() - 2);
        retFrame += "!";
        //┴
      //└┐（その他）
        //┴
      } //～if
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
      isJSON ? SEND_CONN_JSON() : SEND_CONN("");
  //──────────────────
  //➡サブ
  //・モード別の主処理
  //・TXTのみ
  #elif (MODE == MODE_SUB)
  //------------------------------------
      //●ＭＭＰコマンドを実行
      //●実行結果をレスポンス
      modeSub::RUN();
      SEND_CONN("");
  //------------------------------------
  #endif //➡メイン｜➡サブ
  //──────────────────
      //┴
    }); /* this{}/onNotFound() */
    //┴
  }/* registRoutes() */

//========================================================
//§モード別実装のインクルード
//========================================================
#if (MODE == MODE_MAIN)
  #include "HTTP_Main.cpp"
#endif

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // コンストラクタ【基本型】
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  AD_HTTP(MmpContext& argCtx) : // 接続識別子：ダミー
  AD_API<String>(     argCtx, AID::HTTP) // 基本型
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
  void handle() override final {
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