// filename : adpTCP.cpp
//========================================================
// 通信アダプタ：ＴｃｐＲＡＷ
//（接続を維持するネットワーク通信）
//--------------------------------------------------------
// Ver 1.2.1 (2026/08/26) 
// ・全体のロジックをシンプル化
// ・ハンドルで切断スロットをパージする処理を追加
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <WiFi.h> // ユーザ受付資源
  #include <Arduino.h>
  #include <queue>
  #include <mutex>
  //│
  //■ＭＭＰシステム
  #include "adp.h"  // 通信アダプタ共通へ公開
  //┴
//┴

//########################################################
//# 専用名の前空間
//########################################################
namespace adpTCP {
//========================================================
// Ａ．アダプタの基本
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // ステータス
    //─────────────────
    const String ADP_ID  = "TCPR"  ; // アダプタID
    const int    SS_SLOTS  = 10    ; // 複数スロット(接続タイミングで登録)
          bool   ENABLED   = false ; // 有効性：{有効：true|無効：false}

    //─────────────────
    // 使用するサービス
    //─────────────────
    static WiFiServer* ADP_SRV = nullptr; // WiFiサーバ
    static int         SRV_PORT = 8081  ; // ポート番号

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
    static T_SS_SLOT* ssTBL = nullptr   ; // 事前予約

    //─────────────────
    // 初期化
    //----------------------------------
    // 引数：(参照)接続管理スロット
    //─────────────────
    void SS_INI_SLOT(T_SS_SLOT& argSlot){
      SS_INI_SLOT_BASE(argSlot.Base)       ; // 基本メンバを初期化
      if (argSlot.CONN) argSlot.CONN.stop(); // アクセス資源を切断
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
  void SEND_CONN(WiFiClient& argConn){
    //┬
    //○メッセージをレスポンス
    //●ログ出力
    if (argConn.connected()) argConn.print(ctx.resMSG);
    P9_SHOW_LOG();
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
  //─────────────────
  // 別タスクとして機能
  //─────────────────
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
      String retFrame = P2_STREAM(ssTBL[ID].CONN, ssTBL[ID].Base);
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
  static TaskHandle_t TaskHandle = NULL;

  //─────────────────
  // FreeRTOSタスクのエントリポイント
  //─────────────────
  void StreamQueue(void *pvParameters) {
    for (;;) {
      ON_RECIVE();                        // コールバック関数を登録
      vTaskDelay(1 / portTICK_PERIOD_MS); // 短いウェイト
    }
  } /* StreamQueue() */

//========================================================
// Ｇ．公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START() {
    //┬
    //○┐サービスを開始
      //│
      //●接続管理TBLを作成
      ssTBL = new T_SS_SLOT[SS_SLOTS];
      //│
      //○サービス資源を生成
      ADP_SRV = new WiFiServer(SRV_PORT);
      ADP_SRV->begin();
      //│
      //○受信タスクをFreeRTOSの別スレッドとして起動（コア0に割り当て）
      xTaskCreatePinnedToCore(
        StreamQueue,    // 実行するタスク関数
        "UART_ADAPTER", // タスク名（デバッグ用）
        4096,           // スタックサイズ（バイト単位）
        NULL,           // パラメータ
        2,              // 優先度
        &TaskHandle,    // タスクハンドル
        0               // 割り当てるコア番号 (0)
      );
      //┴
    //│
    //○アダプタを有効化
    ENABLED = true; 
    Serial.println(String("　[OK] TCP Raw   -> port ") + String(SRV_PORT));
    //┴
  } /* START() */

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口（ポーリング入口）
  //━━━━━━━━━━━━━━━━━
  void HANDLE(){
    //┬
    //○１．起動チェック
    if (!ENABLED) return; // 初期化済み
    if (!ADP_SRV) return; // サーバが起動済み
    //│＼（このアダプタが無効の場合）
    //│ ▼終了：早期リターン
    //│
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
      //●キュー情報をワークにセット
      P0_SETUP_CONTEXT(ADP_ID, popDat.FRAME);
      //│
#if defined(MMP_TYPE_MAIN) // --┨ＭＭＰ本体┠----┐
      //○リクエストをデータ項目に分解
      P1_SET_ACD_CPATH();
      //│
      //●認証処理を実施
      if (P2_CHECK_AUTH()){SEND_CONN(popDat.CONN); continue;}
      //│＼（処理継続が不可の場合）
      //│ ●エラーをレスポンス
      //│ ▽次へ：次のキューを走査
#endif // ----------------------------------------┘
      //│
      //●コマンド実行
      P3_RUN();
      //│
      //●実行結果をレスポンス
      SEND_CONN(popDat.CONN);
      //┴
    } /* END-while */
    //┴
  } /* HANDLE() */

} /* namespace adpTCP */