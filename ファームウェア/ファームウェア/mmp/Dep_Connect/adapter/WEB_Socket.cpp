// filename : Dep_Connect/adapter/WEB_Socket.cpp
//========================================================
// 接続部門／担当：WEB Socket
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/24)
//========================================================
//┬
//□┐インクルード
  //□追加ライブラリ：WebSockets by Markus Sattler
  #include <WebSocketsServer.h> // メインモード，サブモード
  #include <WebSocketsClient.h> // ブリッジモード
//┴┴

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス：非同期キュー型
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class  AdapterWEB_Socket:
public AdapterQueueBase<uint8_t> // 接続識別子：uint8_t
{
private:
//========================================================
//§基本情報
//========================================================
  //─────────────────
  // 一般情報
  //─────────────────
  const int ADP_ID = ADP_ID_WSOC;
  int getAID() const override {return ADP_ID;}

  //─────────────────
  // サービス関連情報
  //─────────────────
  static AdapterWEB_Socket* MY_TASK          ; // タスク識別(インスタンス)
  int                       MY_PORT = 8082   ; // ポート番号
//--------------------------
//➡ブリッジ：クライアント
#if (MODE == MODE_BRIDGE)
  WebSocketsClient          MY_NET           ; // クライアント(実体)
//➡ブリッジ以外：サーバ
#else
  WebSocketsServer*         MY_NET  = nullptr; // サーバ(ポインタ)
#endif /* ➡ブリッジ｜➡ブリッジ以外 */
//--------------------------

//========================================================
//§返信処理
//========================================================
  //─────────────────
  // クライアントにレスポンス
  //─────────────────
  void SEND_CONN(uint8_t argConn) override final {
//-----------------------------------------
//➡ブリッジ以外
#if (MODE != MODE_BRIDGE)
    //┬
    //○クライアントにレスポンス
    if (MY_NET) MY_NET->sendTXT(argConn, ctx.resMSG.c_str());
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
#endif /* ➡ブリッジ以外 */
//-----------------------------------------
  } /* SEND_CONN() */

//========================================================
//§受信処理
//========================================================
  //─────────────────
  // タスク関数
  //─────────────────
  static void ON_RECIVE(
//-----------------------------------------
//➡ブリッジ以外
#if (MODE != MODE_BRIDGE)
    uint8_t   argNUM , // クライアント番号
#endif /* ➡ブリッジ以外 */
//-----------------------------------------
    WStype_t  argTYPE, // イベント種別
    uint8_t * argDATA, // 受信データ
    size_t    argLEN   // 受信データ長
  ){
  //┬
  //○┐【前処理】
    //○インスタンスを確認
    if (!MY_TASK) return;
    //│＼（当該インスタンスではない場合）
    //│ ▼終了：早期リターン
    //│
    //○イベントの種類を確認
    if (argTYPE != WStype_TEXT) return;
    //│＼（テキスト以外の場合）
    //│ ▼終了：早期リターン
    //│
    //○受信データを確認
    if (argDATA == nullptr || argLEN < 1) return;
    //│＼（空の場合）
    //│ ▼終了：早期リターン
    //┴
  //│
  //○┐【主処理】
    //○┐キュー情報を取得
      //○接続識別子を取得
//-----------------------------------------
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
      uint8_t qConn  = 0; // ダミー値
//➡ブリッジ以外
#else
      uint8_t qConn  = argNUM;
#endif /* ➡ブリッジ｜➡ブリッジ以外 */
//-----------------------------------------
      //┴
    //│
    //●キューを登録
    MY_TASK->pushQueue(qConn, String((char*)argDATA), 0);
    //┴
  //│
  //○┐【後処理】
  //┴┴
  } /* ON_RECIVE() */

//========================================================
//§転送処理(ブリッジ／スレーブ)
//========================================================
//############################
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
  //━━━━━━━━━━━━━━━━━
  // 転送実施
  //━━━━━━━━━━━━━━━━━
  void trans() override final {
  //┬
  //○┐【前処理】
    //○宛先情報を取得
    String transIP = ctx.bridge.Dat1;
    //┴
  //│
  //○┐【主処理】
    //◇┐クライアントを起動
    if (!MY_NET.isConnected()) {
      //├┐（未接続の場合）
        //│
        //○受信タスク（コールバック）を登録
        MY_TASK  = this;
        MY_NET.onEvent(ON_RECIVE); 
        //│
        //○クライアントを起動（成功するまで一定時間リトライ）
        MY_NET.begin(transIP.c_str(), MY_PORT, "/");
        unsigned long startTime = millis();
        while(!MY_NET.isConnected() && millis() - startTime < LIMIT::TIME_CONNECT)
        {MY_NET.loop(); delay(200);}
        if (!MY_NET.isConnected()) {ctx.bridge.MSG = RCD::Trn1Err; return;}
        //│＼（接続に失敗した場合）
        //│ ○完了MSGにエラーCDをセット
        //│ ▼終了：早期リターン
        //┴
      //└┐（その他）
        //┴
    } /* if */
    //│
    //○退避したフレームでリクエスト(非同期でデータ受信)
    MY_NET.sendTXT(ctx.bridge.Frame);
    //┴
  //│
  //○┐【後処理】
  //┴┴
  } /* trans() */
#endif /* ➡ブリッジ */
//############################

//========================================================
//§ハンドル前処理
//========================================================
  //─────────────────
  // 前処理(一般)
  //─────────────────
  bool handle_Setup() override final {
//-----------------------------------------
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
    //※クライアントは未接続でも loop() を回し続けて接続状態の変化を検知
    //※接続の有無に関わらず、後続へ進める（false）
    //┬
    //○WebSocketの処理を進める（イベント発火）
    MY_NET.loop();
    //│
    //▼終了：正常
    return false;
    //┴
//➡ブリッジ以外
#else
    //┬
    //○サーバはインスタンス未生成（Null）の場合は進行不可（true）
    if (!MY_NET) return true;
    //│
    //○WebSocketの処理を進める（イベント発火）
    MY_NET->loop();
    //│
    //▼終了：正常
    return false;
    //┴
#endif /* ➡ブリッジ｜➡ブリッジ以外 */
//-----------------------------------------
  } /* handle_Setup() */

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // コンストラクタ：非同期キュー型
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  AdapterWEB_Socket(MmpContext& argCtx):
  AdapterBase<uint8_t>(argCtx),     // 接続識別子：uint8_t
  AdapterQueueBase<uint8_t>(argCtx) // 接続識別子：uint8_t
  {
  //┬
  //○┐【前処理】
    //●WiFi接続状況を確認
    if (!devWiFi::isConnect(true)) return;
    //┴
  //│
//--------------------------
//➡ブリッジ：[サーバ][受信タスク]が不要
#if (MODE == MODE_BRIDGE)
  //○┐【主処理】
    //┴
  //│
  //○┐【後処理】
    //○メッセージ表示
    Log::prtln(" [OK] WEB Socket");
  //┴┴
//➡ブリッジ以外：[サーバ][受信タスク]が必要
#else
  //○┐主処理
    //○サーバを生成
    MY_NET = new WebSocketsServer(MY_PORT); // サーバ生成
    //│
    //●受信タスクを登録
    MY_TASK = this                        ; // タスク識別を取得
    MY_NET->onEvent(ON_RECIVE)            ; // コールバック関数で登録
    //│
    //○サーバを起動
    MY_NET->begin()                       ; // サーバ起動
    //┴
  //│
  //○┐【後処理】
    //○メッセージ表示
    char msg[128];
    snprintf(msg, sizeof(msg), " [OK] WEB Socket (PORT %d)", MY_PORT);
    Log::prtln(String(msg));
  //┴┴
#endif /* ➡ブリッジ｜➡ブリッジ以外 */
//--------------------------
  } /* constractor AdapterWEB_Socket() */

}; /* class AdapterWEB_Socket */

//========================================================
//§インスタンス管理
//========================================================
AdapterWEB_Socket* AdapterWEB_Socket::MY_TASK = nullptr;