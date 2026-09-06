// filename : connection/member/UART.cpp
//========================================================
// 経路アダプタ：UART
//--------------------------------------------------------
// Ver 1.2.3 (2026/09/06) 
//========================================================
//┬
//■┐インクルード
  //■同僚
  #include "_index_.h"
  //┴
//┴

//########################################################
//# クラス：経路アダプタ(UART)
//########################################################
class AdapterUART : public AdapterQueueBase<Stream*> {
public:
  //━━━━━━━━━━━━━━━━━
  // 抽象基底クラスからコンテクストを継承
  //━━━━━━━━━━━━━━━━━
  using AdapterQueueBase::AdapterQueueBase;

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
    const int ADP_ID = ADP_ID_UART;
          int SS_SLOTS = 2        ; // 固定スロット(USB(CDC)に限定)

  //━━━━━━━━━━━━━━━━━
  // ID取得 (基底クラスの dispatch 処理用)
  //━━━━━━━━━━━━━━━━━
  int getAdpId() const override { return ADP_ID; }

  //━━━━━━━━━━━━━━━━━
  // 接続管理
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // 基本情報
    //─────────────────
    struct T_SS_SLOT{
      SS_SLOT_TYPE Base           ; // 基本メンバ
      Stream*      CONN  = nullptr; // アクセス資源(参照)
    };
    T_SS_SLOT*     ssTBL          ; // 事前予約


//========================================================
// Ｂ．レスポンス
//========================================================
  //─────────────────
  // スロットの受付資源に送信
  //----------------------------------
  // 引数：
  // ・出力制限：強制出力(true)、通常出力(false)
  // ・接続資源：キューから取得した物
  //─────────────────
  void SEND_CONN(bool argMode, Stream* argConn) override {
    //┬
    //○動作モードを確認
    if (!argMode && ctx.sysMode != MODE_MAIN) return;
    //│＼（出力制限がなく、メインモード以外の場合）
    //│ ▼終了：早期リターン
    //│
    //○メッセージをレスポンス
    if (argConn != nullptr) argConn->print(ctx.resMSG);
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */

//========================================================
// Ｄ．データ受信
//========================================================
  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  void ON_RECIVE(){
    //┬
    //◎┐スロットを走査
    for (int ID = 0; ID < SS_SLOTS; ID++) {
      //│＼（最後のスロットに達した場合）
      //│ ▼完了：走査を終了
      //│
      //○スロットの状態を確認
      if (ssTBL[ID].CONN == nullptr) continue;
      //│＼（未使用の場合）
      //│ ▽次へ：次のスロットを走査
      //│
      //●ストリームを受信
      String retFrame = adpFnStream::GET_FRAME(*(ssTBL[ID].CONN), ssTBL[ID].Base);
      if (retFrame == "") continue;
      //│＼（フレームが未完成の場合）
      //│ ▽次へ：次のスロットを走査
      //│
      //○キューに登録（基底クラスの pushQueue を呼出し）
      pushQueue(ssTBL[ID].CONN, retFrame);
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
    AdapterUART* self = static_cast<AdapterUART*>(pvParameters);
    for (;;) {
      if (self) self->ON_RECIVE();        // 疑似コールバック関数
      vTaskDelay(1 / portTICK_PERIOD_MS); // 短いウェイト
    }
  } /* StreamQueue() */

//========================================================
// Ｅ．公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterUART(MmpContext& argCtx) : AdapterQueueBase(argCtx) {
    //┬
    //●┐接続管理TBLを作成
      //○領域を確保
      SS_SLOTS = (ctx.sysMode == MODE_MAIN) ? 2 :1;
      ssTBL    = new T_SS_SLOT[SS_SLOTS];
      //│
      //○USB(CDC)をセット
      ssTBL[0].Base.used = true   ; // 使用中
      ssTBL[0].CONN      = &Serial; // 参照先を登録
      //│
      //○動作モードを確認
      if (ctx.sysMode == MODE_MAIN) {
      //│＼（メインモードの場合）
          //○UART1以降をセット
          ssTBL[1].Base.used = true    ; // 使用中
          ssTBL[1].CONN      = &Serial1; // 参照先を登録
          //┴
      } /* END-if */
    //│
    //○受信タスクをFreeRTOSの別スレッドとして起動（自動コア割当）
    xTaskCreate(
      StreamQueue           , // 実行するタスク関数
      String(ADP_ID).c_str(), // タスク名（デバッグ用）
      4096                  , // スタックサイズ（バイト単位）
      this                  , // パラメータ
      2                     , // 優先度
      &TaskHandle             // タスクハンドル
    );
    //│
    //○メッセージ表示
    Serial.println(String(" [OK] USB/UART  -> #0,#1"));
    //┴
  } /* constractor AdapterUART() */
}; /* class AdapterUART */