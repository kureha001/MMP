// filename : Dep_Connect/adapter/base/UART.cpp
//========================================================
// 接続部門／業務課／担当(標準型)：UART 担当
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/19)
// ・使用するUARTをモード別に構築
// ・標準スロットを廃止
//========================================================

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
class AdapterUART : public AdapterQueueBase<Stream*> {
public:
  //━━━━━━━━━━━━━━━━━
  // 抽象基底クラスからコンテクストを継承
  //━━━━━━━━━━━━━━━━━
  using AdapterQueueBase::AdapterQueueBase;

private:
//========================================================
//§基本情報
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 一般情報
  //━━━━━━━━━━━━━━━━━
    const int ADP_ID = ADP_ID_UART;
    int getAID() const override {return ADP_ID;} // 基底クラスに連携

//========================================================
//§接続管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
  int SLOTs = 0; // コンストラクタで決定
  struct T_SLOT{
    bool    used = false  ; // 基本メンバ
    Stream* CONN = nullptr; // アクセス資源(参照)
  };
  T_SLOT* TBL             ; // 事前予約

//========================================================
//§返信処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // クライアントにレスポンス
  //━━━━━━━━━━━━━━━━━
  void SEND_CONN(Stream* argConn) override {
    //┬
    //○クライアントにレスポンス
    argConn->print(ctx.resMSG);
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */

//========================================================
//§受信処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  void ON_RECIVE(){
    //┬
    //◎┐スロットを走査
    for (int ID = 0; ID < SLOTs; ID++) {
      //│＼（最後のスロットに達した場合）
      //│ ▼完了：走査を終了
      //│
      //●ストリームを受信
      String retFrame = adpFnStream::GET_FRAME(*(TBL[ID].CONN));
      //│
      //●キューに登録（基底クラスの pushQueue を呼出し）
      if (retFrame != "") pushQueue(TBL[ID].CONN, retFrame, ID);
      //┴
    } /* END-for */
    //┴
  } /* ON_RECIVE() */

  //━━━━━━━━━━━━━━━━━
  // スレッド処理の定義
  //━━━━━━━━━━━━━━━━━
  static void StreamQueue(void *pvParameters) {
    AdapterUART* self = static_cast<AdapterUART*>(pvParameters);
    for (;;) {
      if (self) self->ON_RECIVE();        // 疑似コールバック関数
      vTaskDelay(1 / portTICK_PERIOD_MS); // 短いウェイト
    }
  } /* StreamQueue() */

  //━━━━━━━━━━━━━━━━━
  // 並列処理の開始
  //━━━━━━━━━━━━━━━━━
  TaskHandle_t TaskHandle = NULL; // タスク・ハンドル
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
  } /* RUN_TASK() */

//############################
//# 転送機能はブリッジのみ
//############################
#if (MODE == MODE_BRIDGE)
//========================================================
//§転送処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // ポーリングの前処理
  //━━━━━━━━━━━━━━━━━
  bool handle_Begin() override final {
    //┬
    //◇ブリッジマスタとして進行を制御
    switch (ctx.bridge.Stat) {
      case BSTAT::IDLE: return false; // 待機中：進行OK
      case BSTAT::REQ : return true ; // 依頼中：進行NG
      case BSTAT::BUSY: return true ; // 処理中：進行NG
      case BSTAT::DONE:               // 処理済：進行OK
        //●ブリッジ元にレスポンス
        //○進行状況を［待機中］にセット
        //▼終了：早期リターン（進行OK）
        SEND_CONN(TBL[ctx.bridge.slotID].CONN);
        ctx.bridge.Stat = BSTAT::IDLE;
        return false;
    } /* END-switch */
    return true; // 想定外：進行NG
  } /* handle_Begin() */
#endif
//############################

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterUART(MmpContext& argCtx) : AdapterQueueBase(argCtx) {
    //┬
    //●接続管理TBLを作成
//--------------------------
// サブ
//・ブリッジ：すべてクライアント用
//・メインとの接続ポートは登録しない
//--------------------------
#if (MODE == MODE_SUB)
    SLOTs = 2;
    TBL = new T_SLOT[SLOTs];
    TBL[0].CONN = &Serial ; TBL[0].used = true;
    TBL[1].CONN = &Serial2; TBL[1].used = true;
    String msg = " [OK] UART (USB CDC + Serial #2)";
//--------------------------
// サブ以外
//・メイン：サブとの接続ポートを登録
//・ブリッジ：すべてクライアント用
//--------------------------
#else
    SLOTs = 3;
    TBL = new T_SLOT[SLOTs];
    TBL[0].CONN = &Serial ; TBL[0].used = true;
    TBL[1].CONN = &Serial1; TBL[1].used = true; //※サブとの接続用
    TBL[2].CONN = &Serial2; TBL[2].used = true;
    String msg = " [OK] UART (USB CDC + Serial #1,2)";
#endif
//--------------------------
    //│
    //●受信タスクを起動
    RUN_TASK();
    //│
    //○メッセージ表示
    Log::prtln(msg);
    //┴
  } /* constractor AdapterUART() */

}; /* class AdapterUART */