// filename : Dep_Connect/adapter/base/TCP.cpp
//========================================================
// 接続部門／業務課／担当(標準型)：TCP RAW 担当
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/11)
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
#if (MODE == MODE_BRIDGE)
    WiFiClient  MY_NET           ; // WiFiクライアント(実体)
#else
    WiFiServer* MY_NET  = nullptr; // WiFiサーバ(ポインタ)
    int         MY_PORT = 8081   ; // ポート番号
#endif

//========================================================
// 接続情報管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
  int SS_SLOTS = 10              ; // 複数スロット(接続タイミングで登録)
  struct T_SS_SLOT{
    SS_SLOT_TYPE Base           ; // 基本メンバ
    WiFiClient   CONN           ; // アクセス資源(TCP接続の実体)
  };
  T_SS_SLOT*     ssTBL = nullptr; // 事前予約

  //─────────────────
  // 初期化
  //----------------------------------
  // 引数：(参照)接続管理スロット
  //─────────────────
  void SS_INI_SLOT(T_SS_SLOT& argSlot){
    adpFnStream::SS_INI_SLOT_BASE(argSlot.Base); // 基本メンバを初期化
    if (argSlot.CONN) argSlot.CONN.stop()      ; // アクセス資源を切断
  } /* SS_INI_SLOT() */
    
  //─────────────────
  // 空きSID取得
  //----------------------------------
  // 戻り値：スロットID
  // ・0,1,2...：空きスロットのID
  // ・-1：空きスロットが無い
  //─────────────────
  int SS_GET_FREE_ID() {
    //┬
    //◎┐先頭から走査
    for (int ID = 0; ID < SS_SLOTS; ID++) {
    //│＼（全スロットを走査し終えた場合）
    //│ ▽中断：ループ処理を中断
    //│
    //○スロットを確認
    if (!ssTBL[ID].Base.used) return ID;
    //│＼（未使用の場合）
    //│ ▼返却：当該スロットIDを返す
    } /* END-for */
    //│
    //▼返却：エラーCD(空きスロットがない)
    return -1;
    //┴
  } /* SS_GET_FREE_ID() */

  //─────────────────
  // 動的アタッチ
  //----------------------------------
  // 戻り値 ：処理結果（論理値）
  // ・false：正常
  // ・true ：異常
  //─────────────────
  bool SS_ATTACH(){
#if (MODE == MODE_BRIDGE)
    //【ブリッジ（クライアント）モードの場合】
    // MY_NET 自身がアクティブであれば 0番スロットに直接割り当てる
    if (!MY_NET.connected()) return false;

    if (!ssTBL[0].Base.used) {
      SS_INI_SLOT(ssTBL[0]);
      ssTBL[0].Base.used = true;
      ssTBL[0].CONN      = MY_NET; // クライアント接続をスロット0にセット
      ssTBL[0].CONN.setNoDelay(true);
    }
    return false;
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
    int ID = SS_GET_FREE_ID();
    if (ID < 0) return true;
    //│＼（空きスロットがない）
    //│ ▼返却：異常
    //│
    //●スロットを初期化
    SS_INI_SLOT(ssTBL[ID]);
    //│
    //○スロットに新規接続を登録
    ssTBL[ID].Base.used = true     ; // 使用中
    ssTBL[ID].CONN      = newConn  ; // TCP接続(実体)を登録
    ssTBL[ID].CONN.setNoDelay(true); // TCPパケット遅延制御
    //┴
    } //* END-while */
#endif
  } /* SS_ATTACH() */

//========================================================
// レスポンス
//========================================================
  //━━━━━━━━━━━━━━━━━
  // クライアントにレスポンス
  //━━━━━━━━━━━━━━━━━
  void SEND_CONN(WiFiClient argConn) override {
#if (MODE != MODE_BRIDGE)
    //┬
    //○クライアントにレスポンス
    if (argConn.connected()) argConn.print(ctx.resMSG);
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
#endif
  } /* SEND_CONN() */

//========================================================
// データ受信
//========================================================
  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  void ON_RECIVE(){
    //┬
    //○接続管理スロットを動的アタッチ
    bool Result = SS_ATTACH();
    //│
    //◎┐スロットを走査
    for (int ID = 0; ID < SS_SLOTS; ID++) {
      //│＼（最後のスロットに達した場合）
      //│ ▼完了：走査を終了
      //│
      //○┐スロットの状態を確認
        //│
        //○接続状況を確認
        if (!ssTBL[ID].CONN.connected()) {
        //│＼（切断の場合）
            //○スロットを初期化する
            //▽次へ：次のスロットを走査
            SS_INI_SLOT(ssTBL[ID]);
            continue;
        } /* END-if */
        //│
        //○使用状況を確認
        if (!ssTBL[ID].Base.used) continue;
        //│＼（未使用のスロットの場合）
        //│ ▽次へ：次のスロットを走査
        //┴
      //│
      //●ストリームを受信
      String retFrame = adpFnStream::GET_FRAME(ssTBL[ID].CONN, ssTBL[ID].Base);
      if (retFrame == "") continue;
      //│＼（フレームが未完成の場合）
      //│ ▽次へ：次のスロットを走査
      //│
      //○キューに登録（基底クラスの pushQueue を呼出し）
      pushQueue(ssTBL[ID].CONN, retFrame, ID);
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
#if (MODE == MODE_BRIDGE)
    //┬
    //●接続管理TBLを作成
    ssTBL = new T_SS_SLOT[SS_SLOTS];
    //│
    //○メッセージ表示
    Serial.println(" [OK] TCP");
    //┴
#else
    //┬
    //●接続管理TBLを作成
    ssTBL = new T_SS_SLOT[SS_SLOTS];
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
  } /* constractor AdapterTCP() */


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
        SS_INI_SLOT(ssTBL[0]);
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

}; /* class AdapterTCP */