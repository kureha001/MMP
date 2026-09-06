// filename : connection/member/TCP.cpp
//========================================================
// 経路アダプタ：TCP RAW
//--------------------------------------------------------
// Ver 1.2.3 (2026/09/06) 
//========================================================
//┬
//■┐インクルード
  //■同僚
  #include "_index_.h"
  //│
  //■Arduinoシステム
  #include <WiFi.h> // ユーザ受付資源
  #include <queue>
  #include <mutex>
  //┴
//┴

//########################################################
//# クラス：経路アダプタ(TCP RAW)
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
  // 基本情報
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // ステータス
    //─────────────────
    const int  ADP_ID = ADP_ID_TCP;
    const int  SS_SLOTS = 10    ; // 複数スロット(接続タイミングで登録)
          bool ENABLED  = false ; // 有効性：{有効：true|無効：false}

    //─────────────────
    // 使用するサービス
    //─────────────────
    WiFiServer* ADP_SRV  = nullptr; // WiFiサーバ
    int         SRV_PORT = 8081   ; // ポート番号

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
// レスポンス
//========================================================
  //─────────────────
  // クライアントにレスポンス
  // ※AdapterQueueBaseをオーバーライド
  //─────────────────
  void SEND_CONN( WiFiClient argConn) override {
    //┬
    //○メッセージをレスポンス
    if (argConn.connected()) argConn.print(ctx.resMSG);
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
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
    AdapterTCP* self = static_cast<AdapterTCP*>(pvParameters);
    for (;;) {
      if (self) self->ON_RECIVE();        // 疑似コールバック関数
      vTaskDelay(1 / portTICK_PERIOD_MS); // 短いウェイト
    }
  } /* StreamQueue() */

//========================================================
// 公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterTCP(MmpContext& argCtx) : AdapterQueueBase(argCtx) {
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
}; /* class AdapterTCP */