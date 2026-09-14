// filename : Dep_Connect/adapter/base/TCP.cpp
//========================================================
// 接続部門／業務課／担当(標準型)：TCP 担当
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/14)
// ・標準スロットを廃止
// ・プリプロセッサを最適化
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <WiFi.h> // ユーザ受付資源
  #include <queue>
  #include <mutex>
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
class AdapterTCP : public AdapterQueueBase<WiFiClient> {
public:
  //━━━━━━━━━━━━━━━━━
  // 抽象基底クラスからコンテクストを継承
  //━━━━━━━━━━━━━━━━━
  using AdapterQueueBase::AdapterQueueBase;

private:
//========================================================
// アダプタの基本
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 一般情報
  //━━━━━━━━━━━━━━━━━
    const int  ADP_ID = ADP_ID_TCP;
    int getAID() const override {return ADP_ID;} // 基底クラスに連携

  //━━━━━━━━━━━━━━━━━
  // サービス関連情報
  //━━━━━━━━━━━━━━━━━
//--------------------------
// ブリッジはWiFiクライアント
//--------------------------
#if (MODE == MODE_BRIDGE)
    WiFiClient  MY_NET           ; // WiFiクライアント(実体)
//--------------------------
// ブリッジ以外はWiFiサーバ
//--------------------------
#else
    WiFiServer* MY_NET  = nullptr; // WiFiサーバ(ポインタ)
    int         MY_PORT = 8081   ; // ポート番号
#endif
//--------------------------

//========================================================
// 接続管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
  int SLOTs = 0; // コンストラクタで決定
  struct T_SLOT{
    bool       used = false;
    WiFiClient CONN; // TCP接続の実体
  };
  T_SLOT* TBL = nullptr;

  //─────────────────
  // 初期化
  //----------------------------------
  // 引数：(参照)接続管理スロット
  //─────────────────
  void SLOT_INI(T_SLOT& argSlot){
    argSlot.used = false;
    if (argSlot.CONN) argSlot.CONN.stop();
  } /* SLOT_INI() */
    
  //─────────────────
  // 空きSID取得
  //----------------------------------
  // 戻り値：スロットID
  // ・0,1,2...：空きスロットのID
  // ・-1：空きスロットが無い
  //─────────────────
  int SLOT_GET_FREE() {
    //┬
    //◎┐先頭から走査
    for (int ID = 0; ID < SLOTs; ID++) {
    //│＼（全スロットを走査し終えた場合）
    //│ ▽中断：ループ処理を中断
    //│
    //○スロットを確認
    if (!TBL[ID].used) return ID;
    //│＼（未使用の場合）
    //│ ▼返却：当該スロットIDを返す
    } /* END-for */
    //│
    //▼返却：エラーCD(空きスロットがない)
    return -1;
    //┴
  } /* SLOT_GET_FREE() */

  //─────────────────
  // 動的アタッチ
  //----------------------------------
  // 戻り値 ：処理結果（論理値）
  // ・false：正常
  // ・true ：異常
  //─────────────────
  bool SLOT_ATTACH(){
//--------------------------
//【ブリッジ】単一スロット
//--------------------------
#if (MODE == MODE_BRIDGE)
    //【ブリッジ（クライアント）モードの場合】
    // MY_NET 自身がアクティブであれば 0番スロットに直接割り当てる
    if (!MY_NET.connected()) return false;

    if (!TBL[0].used) {
      SLOT_INI(TBL[0]);
      TBL[0].used = true;
      TBL[0].CONN = MY_NET; // クライアント接続をスロット0にセット
      TBL[0].CONN.setNoDelay(true);
    }
    return false;
//--------------------------
//【ブリッジ以外】動的スロット
//--------------------------
#else
    //┬
    //◎┐未管理のTCP接続をMMP管理対象へ登録する
    while (true) {
    //│
    //○新規のTCP接続を取得
    WiFiClient newConn = MY_NET->available(); // WiFiサーバ(ポインタ)
    if (!newConn) return false;
    //│＼（あらたな接続がない場合）
    //│ ▼返却：正常
    //│
    //●空きスロットを探す
    int ID = SLOT_GET_FREE();
    if (ID < 0) return true;
    //│＼（空きスロットがない）
    //│ ▼返却：異常
    //│
    //●スロットを初期化
    SLOT_INI(TBL[ID]);
    //│
    //○スロットに新規接続を登録
    TBL[ID].used = true   ; // 使用中
    TBL[ID].CONN = newConn; // TCP接続(実体)を登録
    TBL[ID].CONN.setNoDelay(true); // TCPパケット遅延制御
    //┴
    } //* END-while */
#endif
//--------------------------
  } /* SLOT_ATTACH() */

//############################
//# ブリッジは[trans()]で処理
//############################
#if (MODE != MODE_BRIDGE)
//========================================================
// レスポンス
//========================================================
  //━━━━━━━━━━━━━━━━━
  // クライアントにレスポンス
  //━━━━━━━━━━━━━━━━━
  void SEND_CONN(WiFiClient argConn) override {
    //┬
    //○クライアントにレスポンス
    if (argConn.connected()) argConn.print(ctx.resMSG);
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */
#endif
//############################

//========================================================
// データ受信
//========================================================
  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  void ON_RECIVE(){
    //┬
    //○接続管理スロットを動的アタッチ
    bool Result = SLOT_ATTACH();
    //│
    //◎┐スロットを走査
    for (int ID = 0; ID < SLOTs; ID++) {
      //│＼（最後のスロットに達した場合）
      //│ ▼完了：走査を終了
      //│
      //○┐スロットの状態を確認
        //│
        //○接続状況を確認
        if (!TBL[ID].CONN.connected()) {
        //│＼（切断の場合）
            //○スロットを初期化する
            //▽次へ：次のスロットを走査
            SLOT_INI(TBL[ID]);
            continue;
        } /* END-if */
        //│
        //○使用状況を確認
        if (!TBL[ID].used) continue;
        //│＼（未使用のスロットの場合）
        //│ ▽次へ：次のスロットを走査
        //┴
      //│
      //●ストリームを受信
      String retFrame = adpFnStream::GET_FRAME(TBL[ID].CONN);
      if (retFrame == "") continue;
      //│＼（フレームが未完成の場合）
      //│ ▽次へ：次のスロットを走査
      //│
      //○キューに登録（基底クラスの pushQueue を呼出し）
      pushQueue(TBL[ID].CONN, retFrame, ID);
      //┴
    } /* END-for */
    //┴
  } /* ON_RECIVE() */

  //━━━━━━━━━━━━━━━━━
  // スレッド処理の定義
  // ※この関数はスタティックにする
  //━━━━━━━━━━━━━━━━━
  TaskHandle_t TaskHandle = NULL; // タスク・ハンドル
  static void StreamQueue(void *pvParameters) {
    AdapterTCP* self = static_cast<AdapterTCP*>(pvParameters);
    for (;;) {
      if (self) self->ON_RECIVE();        // 疑似コールバック関数
      vTaskDelay(1 / portTICK_PERIOD_MS); // 短いウェイト
    }
  } /* StreamQueue() */


  //━━━━━━━━━━━━━━━━━
  //
  //━━━━━━━━━━━━━━━━━
  void RUN_TASK() {
    //○受信タスクをFreeRTOSの別スレッドとして起動（自動コア割当）
    xTaskCreate(
      StreamQueue           , // 実行するタスク関数
      String(ADP_ID).c_str(), // タスク名（デバッグ用）
      4096                  , // スタックサイズ（バイト単位）
      this                  , // パラメータ
      2                     , // 優先度
      &TaskHandle             // タスクハンドル
    );
  }

//========================================================
// 担務（公開機能）
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterTCP(MmpContext& argCtx) : AdapterQueueBase(argCtx) {
//--------------------------
// ブリッジは単一スロット
//--------------------------
#if (MODE == MODE_BRIDGE)
    //┬
    //●接続管理TBLを作成
    SLOTs = 1;
    TBL   = new T_SLOT[SLOTs];
    //│
    //○メッセージ表示
    Serial.println(" [OK] TCP");
    //┴
//--------------------------
// ブリッジ以外は複数スロット
//--------------------------
#else
    //┬
    //●接続管理TBLを作成
    SLOTs = 10;
    TBL   = new T_SLOT[SLOTs];
    //│
    //○サービス資源を生成
    MY_NET = new WiFiServer(MY_PORT);
    MY_NET->begin();
    //│
    //●受信タスクを別スレッドとして起動
    RUN_TASK();
    //│
    //○メッセージ表示
    Serial.printf(" [OK] TCP (PORT=[%d])\n", MY_PORT);
    //┴
#endif
//--------------------------
  } /* constractor AdapterTCP() */


//############################
//# 転送機能はブリッジのみ
//############################
#if (MODE == MODE_BRIDGE)
  //━━━━━━━━━━━━━━━━━
  // 転送受付
  //━━━━━━━━━━━━━━━━━
  void trans() {
    //┬
    //◇┐クライアントを起動
    if (!MY_NET.connected()) {
      //├┐（未接続の場合）
        //│
        //○TCPクライアントを起動
        String   ip   = ctx.bridge.Dat1;
        uint16_t port = (uint16_t)ctx.bridge.Dat2.toInt();
        MY_NET.setTimeout(2000);
        if (!MY_NET.connect(ip.c_str(), port)) {ctx.strFrame = "#CNT!"; return;}
        //│＼（接続に失敗した場合）
        //│ ○コンテクストにエラーCDをセット
        //│ ▼終了：早期リターン
        //│
        //○0番スロットをリセットして自身を登録準備
        SLOT_INI(TBL[0]);
        //│
        //●受信タスクが未起動なら起動（※二重起動防止）
        if (TaskHandle == NULL) RUN_TASK();
        //┴
      //└┐（その他）
        //┴
    } /* END-if */
    //│
    //○リクエストを転送
    MY_NET.print(ctx.strFrame);
    //┴
  };
#endif
//############################

}; /* class AdapterTCP */