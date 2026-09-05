// filename : adapter/UART.cpp
//========================================================
// 経路アダプタ：UART
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
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
class AdapterUART : public AdapterBase {
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
    const int ADP_ID = ADP_ID_UART;
          int SS_SLOTS = 2        ; // 固定スロット(USB(CDC)に限定)

  //━━━━━━━━━━━━━━━━━
  // 接続管理
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // 基本情報
    //─────────────────
    struct T_SS_SLOT{
      SS_SLOT_TYPE Base              ; // 基本メンバ
      Stream*      CONN  = nullptr   ; // アクセス資源(参照)
    };
    static T_SS_SLOT* ssTBL          ; // 事前予約


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
  void SEND_CONN(bool argMode, Stream* argConn){
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
// Ｃ．リクエスト・キュー管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
    struct myQueue {
      Stream* CONN = nullptr; // アクセス資源(シリアルのオブジェクトを参照)
      String  FRAME         ; // 受信バッファ
    };
    static std::queue<myQueue> QUEUE      ; // キューバッファ
    static std::mutex          QUEUE_MUTEX; // 別スレッドとの衝突回避用のロック

  //─────────────────
  // キューの取出
  //----------------------------------
  // 引数：
  // ・キュー受取用の変数
  //----------------------------------
  // 戻り値：キューの有無（論理値）
  // ・true ：あり
  // ・false：なし
  //─────────────────
  bool popQueue(myQueue &argData) {
    //┬
    //○別スレッドとの衝突回避用のロック
    std::lock_guard<std::mutex> lock(QUEUE_MUTEX);
    //│
    //○キューの容量を確認
    if (QUEUE.empty()) return false;
    //│＼（通信デバイスが起動していない場合）
    //│ ▼返却：なし
    //│
    //○先頭を抽出
    //○先頭を削除
    //▼返却：あり
    argData = QUEUE.front();
    QUEUE.pop();
     return true;
  } /* popQueue() */

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
      //○キューに登録
      std::lock_guard<std::mutex> lock(QUEUE_MUTEX); // 排他ロック
      QUEUE.push({ssTBL[ID].CONN, retFrame})       ; // キューを追加(通信資源、フレーム)
      //┴
    } /* END-for */
    //┴
  } /* ON_RECIVE() */

  //━━━━━━━━━━━━━━━━━
  // スレッド処理の定義
  //━━━━━━━━━━━━━━━━━
  static TaskHandle_t TaskHandle;         // タスク・ハンドル
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
  AdapterUART(MmpContext& argCtx) : AdapterBase(argCtx) {
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

  //━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━
  void handle() override {
    //┬
    //◎┐ルーティングを指示
    myQueue popDat;
    while (popQueue(popDat)) {
      //│＼（キューが空の場合）
      //│ ▼完了：ルーティングを終了
      //│
      //○フレームの状態を確認
      if (popDat.FRAME.startsWith("#")){SEND_CONN(true, popDat.CONN); continue;}
      //│＼（エラーが発生している場合）
      //│ ●エラーを強制レスポンス
      //│ ▽次へ：次のキューを走査
      //│
      //●コマンドを実行
      mode::RUN(ADP_ID, popDat.FRAME);
      //│
      //●実行結果をレスポンス
      SEND_CONN(false, popDat.CONN);
      //┴
    } /* END-while */
    //┴
  } /* handle() */

}; /* class AdapterUART */


//########################################################
//# スタティック資源の実体
//########################################################
//┬
//■サーバ／サービス
//│
//■送受信バッファ
AdapterUART::T_SS_SLOT* AdapterUART::ssTBL = nullptr;
//│
//■スレッド／コールバック
TaskHandle_t AdapterUART::TaskHandle = NULL;
//│
//■リクエスト
std::queue<AdapterUART::myQueue> AdapterUART::QUEUE;
std::mutex AdapterUART::QUEUE_MUTEX;
//┴
