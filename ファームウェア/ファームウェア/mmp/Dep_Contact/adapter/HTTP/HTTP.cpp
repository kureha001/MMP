// filename : Dep_Contact/adapter/HTTP/HTTP.cpp
//========================================================
// 接客部門／担当(通信アダプタ)：HTTP(ベース：メイン/サブ)
//--------------------------------------------------------
//・通信タイプ： 基本 ※ハンドルの実装が必要
//・接続識別子： uint8_t （ダミー）
//--------------------------------------------------------
// Ver 1.4.0 (2026/10/12)
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <WebServer.h>
//┴┴

class AD_HTTP:public AD_API<uint8_t>{
private:
//========================================================
//§基本情報
//========================================================
  int        MY_PORT   = 8080   ; // ポート番号
  WebServer* MY_NET    = nullptr; // WEBサーバ(ポインタ)
  bool       IS_JSON   = false  ; // レスポンス形式(JSON形式)フラグ
  String     POP_FRAME = ""     ; // 取得フレームのバッファ

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
  // 終了処理：接続元に処理結果を送信する
  //───────────────────────────
  void SEND_RESULT(uint8_t argConn_Dummy) override final {
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
    //○接続元に処理結果を送信する
    ADD_CROSS(*MY_NET);
    MY_NET->send(200, "text/plain; charset=utf-8", ctx.base.Result);
    //┴
  //│
  //○┐【後処理】
    //●コンテクスト・ログを出力する
    adpFnBase::LOG_CTX();
  //┴
  } /* SEND_RESULT() */

//========================================================
//§受信処理
//========================================================
  //───────────────────────────
  // 受信データの取込 ※結果はメンバ変数にセット
  //───────────────────────────
  void ON_RECIVE() {
  //┬
  //○┐【前処理】
    //○JSON形式フラグをOFFにする
    IS_JSON = false;
    //┴
  //│
  //○┐【主処理】
    //○受信データを取込む
    POP_FRAME = MY_NET->uri();
    //│
    //○レスポンス形式を確認する
    if (!POP_FRAME.endsWith("@!")) return;
    //│
    //○JSON形式をONにする
    IS_JSON = true;
    //│
    //○フレーム末尾の"@"を削除する
    POP_FRAME.remove(POP_FRAME.length() - 2);
    POP_FRAME += "!";
    //┴
  //│
  //○┐【後処理】
  //┴┴
  }

  //───────────────────────────
  // CORS事前確認
  //------------------------------------------------------
  // ブラウザがアクセス前に送信するOPTIONS要求(プリフライト)へ応答
  // → CORS許可ヘッダを付加してブラウザへ許可情報を通知
  // → 本通信で返すデータはないためHTTPステータス204を返却
  //───────────────────────────
  inline void route204(WebServer& argSrv) {
    //┬
    //●CORS許可用HTTPヘッダ追加
    ADD_CROSS(argSrv);
    //│
    //○HTTPステータスを返却
    //│※200:返すデータあり／204:返すデータなし
    //argSrv.send(204, "text/plain", "");
    argSrv.send(200, "text/plain; charset=utf-8", RCD::NotMod);
    //┴
  } /* route204() */

  //───────────────────────────
  // ルーティング登録
  //───────────────────────────
  void registRoutes(WebServer& server){
    //----------------------------------------------------
    //┬
    //○ホスト直下の応答：GET
    server.on("/", HTTP_GET,     [&server, this](){route204(server);});
    //┴
    //----------------------------------------------------
    //┬
    //○ホスト直下の応答：CORS事前確認
    server.on("/", HTTP_OPTIONS, [&server, this](){route204(server); });
    //┴
    //----------------------------------------------------
    //┬
    //○┐ＭＭＰコマンドの応答
    server.onNotFound([&server, this](){
      //○┐【前処理】
        //○フレームをクリアする
        POP_FRAME = "";
        //│
        //○アクセス内容を確認
        if (server.method() == HTTP_OPTIONS)
        {route204(server); return;}
        //│＼（HTTP層で完結している）
        //│ ●CORS事前確認へ応答
        //│ ▼終了：早期リターンする
        //┴
      //│
      //●受信データを取込む
      ON_RECIVE();
      //┴
    }); /* onNotFound */
    //┴
    //----------------------------------------------------
  }/* registRoutes() */

//========================================================
//§モード別実装のインクルード
//========================================================
#if MODE == MODE_MAIN
  // 関数：HANDLE_CORE(),JSON関連が多数
  #include "HTTP_Main.cpp"
#else
  // 関数：HANDLE_CORE()
  #include "HTTP_Sub.cpp"
#endif

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  AD_HTTP(MmpContext& argCtx):
  AD_API<uint8_t>(argCtx, AID::HTTP){
  //┬
  //○┐【前処理】
    //●前処理（一般用）を実行する...進行判定を得る
    if (SETUP()) return;
    //│＼（異常の場合）
    //│ ▼終了：早期リターンする
    //┴
  //│
  //○┐【主処理】
    //○サービス資源を生成
    MY_NET = new WebServer(MY_PORT); // サーバ生成
    registRoutes(*MY_NET)          ; // ルーティング登録
    MY_NET->begin()                ; // サーバ起動
    //┴
  //│
  //○┐【後処理】
    //●起動ログMSGを表示する
    String msg = " [OK] HTTP      (PORT." + String(MY_PORT) + ")";
    Log::prtln(msg);
  //┴┴
  } /* constractor AD_HTTP() */

  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  void handle() override final {
  //┬
  //○┐【前処理】
    //●進行判定を確認する（一般用）
    if (this->SETUP()) return;
    //│＼（進行不可の場合）
    //│ ▼終了：早期リターン
    //┴
  //│
  //○┐【主処理】
    //●フレームを求める
    MY_NET->handleClient();
    if (POP_FRAME == "") return;
    //│＼（リクエストがない場合）
    //│ ▼終了：早期リターン
    //│
    //●コンテキストを更新する
    adpFnBase::SETUP_CTX(
      MY_AID,   // 通信アダプタID
      0,        // 接続スロットID(ダミー値)
      POP_FRAME // 求めたフレーム
    );
    //│
    //●ハンドラの主処理を実施する
    HANDLE_CORE();
    //┴
  //│
  //○┐【後処理】
  //┴┴
  } /* handle() */

}; /* class AD_HTTP */