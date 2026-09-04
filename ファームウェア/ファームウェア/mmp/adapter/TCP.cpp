// filename : adapter/TCP.cpp
//========================================================
// 経路アダプタ：TCP RAW
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <WiFi.h> // ユーザ受付資源
  #include <queue>
  #include <mutex>
  //┴
//┴

//########################################################
//# クラス：経路アダプタ(TCP RAW)
//########################################################
class AdapterTCP : public AdapterBase {
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
    const int ADP_ID = ADP_ID_TCP;
    const int    SS_SLOTS = 10    ; // 複数スロット(接続タイミングで登録)
          bool   ENABLED  = false ; // 有効性：{有効：true|無効：false}

    //─────────────────
    // 使用するサービス
    //─────────────────
    static WiFiServer* ADP_SRV    ; // WiFiサーバ
    static int         SRV_PORT   ; // ポート番号

  //━━━━━━━━━━━━━━━━━
  // 接続管理
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // 基本情報
    //─────────────────
    struct T_SS_SLOT{
      SS_SLOT_TYPE    Base              ; // 基本メンバ
      WiFiClient      CONN              ; // アクセス資源(TCP接続の実体)
    };
    static T_SS_SLOT* ssTBL             ; // 事前予約

    //─────────────────
    // 初期化
    //----------------------------------
    // 引数：(参照)接続管理スロット
    //─────────────────
    void SS_INI_SLOT(T_SS_SLOT& argSlot){
      adpStream::SS_INI_SLOT_BASE(argSlot.Base); // 基本メンバを初期化
      if (argSlot.CONN) argSlot.CONN.stop()    ; // アクセス資源を切断
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
      //▼返却：エラーコード(空きスロットがない)
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
      //┬
      //◎┐未管理のTCP接続をMMP管理対象へ登録する
      while (true) {
      //│
      //○新規のTCP接続を取得
      WiFiClient newConn = ADP_SRV->available();
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
      ssTBL[ID].Base.used = true      ; // 使用中
      ssTBL[ID].CONN      = newConn   ; // TCP接続(実体)を登録
      ssTBL[ID].CONN.setNoDelay(true) ; // TCPパケット遅延制御
      //┴
      } //* END-while */
    } /* SS_ATTACH() */

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
  void SEND_CONN(bool argMode, WiFiClient& argConn){
    //┬
    //○動作モードを確認
    if (!argMode && ctx.sysMode != MODE_MAIN) return;
    //│＼（出力制限がなく、メインモード以外の場合）
    //│ ▼終了：早期リターン
    //│
    //○メッセージをレスポンス
    if (argConn.connected()) argConn.print(ctx.resMSG);
    //│
    //●ログ出力
    adpBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */

//========================================================
// Ｃ．リクエスト・キュー管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
    struct myQueue {
      WiFiClient CONN ; // アクセス資源(TCP接続の実体)
      String     FRAME; // 受信バッファ
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
      String retFrame = adpStream::GET_FRAME(ssTBL[ID].CONN, ssTBL[ID].Base);
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
    AdapterTCP* self = static_cast<AdapterTCP*>(pvParameters);
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
  AdapterTCP(MmpContext& argCtx) : AdapterBase(argCtx) {
    //┬
    //●接続管理TBLを作成
    ssTBL = new T_SS_SLOT[SS_SLOTS];
    //│
    //○サービス資源を生成
    ADP_SRV = new WiFiServer(SRV_PORT);
    ADP_SRV->begin();
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
    Serial.println(String(" [OK] TCP Raw   -> port ") + String(SRV_PORT));
    //┴
  } /* constractor AdapterTCP() */

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
      adpBase::RUN(ADP_ID, popDat.FRAME);
      //│
      //●実行結果をレスポンス
      SEND_CONN(false, popDat.CONN);
      //┴
    } /* END-while */
    //┴
  } /* handle() */

}; /* class AdapterTCP */


//########################################################
//# スタティック資源の実体
//########################################################
//┬
//■サーバ／サービス
WiFiServer* AdapterTCP::ADP_SRV  = nullptr; // サーバ
int         AdapterTCP::SRV_PORT = 8081   ; // サービス・ポート
//│
//■送受信バッファ
AdapterTCP::T_SS_SLOT* AdapterTCP::ssTBL = nullptr;
//│
//■スレッド／コールバック
TaskHandle_t AdapterTCP::TaskHandle = NULL;
//│
//■リクエスト
std::queue<AdapterTCP::myQueue> AdapterTCP::QUEUE;
std::mutex AdapterTCP::QUEUE_MUTEX;
//┴