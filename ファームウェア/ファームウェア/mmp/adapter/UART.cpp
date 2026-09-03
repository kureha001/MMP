// filename : adapter/UART.cpp
//========================================================
// 通信アダプタ：ＵＡＲＴ
//--------------------------------------------------------
// Ver 1.2.1 (2026/08/27) 
//========================================================
#pragma once
//┬
//■インクルード
//┴

//########################################################
//# クラス
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
    const String ADP_ID   = "UART"; // アダプタID
    const int    SS_SLOTS = 2     ; // 固定スロット(物理ポート毎に1個)

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
  void SEND_CONN(Stream* argConn){
    //┬
    //○メッセージをレスポンス
    if (argConn != nullptr) argConn->print(ctx.resMSG);
    //│
    //●ログ出力
    adpBase::P9_SHOW_LOG();
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
  //─────────────────
  // 別タスクとして機能
  //─────────────────
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
      String retFrame = adpStream::GET_FRAME(*(ssTBL[ID].CONN), ssTBL[ID].Base);
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

  //─────────────────
  // タスクのハンドルを保持する変数
  //─────────────────
  static TaskHandle_t TaskHandle;

  //─────────────────
  // FreeRTOSタスクのエントリポイント
  //─────────────────
  static void StreamQueue(void *pvParameters) {
    AdapterUART* self = static_cast<AdapterUART*>(pvParameters);
    for (;;) {
      if (self) self->ON_RECIVE();        // コールバック関数を登録
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
    //●接続管理TBLを作成
    ssTBL = new T_SS_SLOT[SS_SLOTS];
    ssTBL[0].Base.used = true     ; // 使用中
    ssTBL[0].CONN      = &Serial  ; // 参照先を登録
    ssTBL[1].Base.used = true     ; // 使用中
    ssTBL[1].CONN      = &Serial1 ; // 参照先を登録
    //│
    //○受信タスクをFreeRTOSの別スレッドとして起動（自動コア割当）
    xTaskCreate(
      StreamQueue,    // 実行するタスク関数
      ADP_ID.c_str(), // タスク名（デバッグ用）
      4096,           // スタックサイズ（バイト単位）
      this,           // パラメータ
      2,              // 優先度
      &TaskHandle     // タスクハンドル
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
      if (popDat.FRAME.startsWith("#")){SEND_CONN(popDat.CONN); continue;}
      //│＼（エラーが発生している場合）
      //│ ●エラーをレスポンス
      //│ ▽次へ：次のキューを走査
      //│
      //●コマンドを実行
      adpBase::RUN(ADP_ID, popDat.FRAME);
      //│
      //●実行結果をレスポンス
      SEND_CONN(popDat.CONN);
      //┴
    } /* END-while */
    //┴
  } /* handle() */

}; /* class AdapterUART */

// staticメンバの実体定義
AdapterUART::T_SS_SLOT* AdapterUART::ssTBL = nullptr;
std::queue<AdapterUART::myQueue> AdapterUART::QUEUE;
std::mutex AdapterUART::QUEUE_MUTEX;
TaskHandle_t AdapterUART::TaskHandle = NULL;