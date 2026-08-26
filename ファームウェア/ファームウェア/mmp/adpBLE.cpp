// filename : adpBLE.cpp
//========================================================
// 通信アダプタ：ＢＬＥ
//（コールバック・クライアント追跡なし）
//--------------------------------------------------------
// Ver 1.2.0 (2026/08/25) 
// ・全体のロジックをシンプル化
//========================================================
#pragma once
//┬
//■┐インクルード
  //■┐Arduinoシステム
    //■BLE関連
    #include <BLEDevice.h> // ユーザ受付資源を
    #include <BLEServer.h> // ユーザ受付資源
    #include <BLEUtils.h > // ユーザ受付資源
    #include <BLE2902.h  > // ユーザ受付資源
    //│
    //■キューイング関連
    #include <Arduino.h>
    #include <queue>
    #include <mutex>
    //┴
  //│
  //■ＭＭＰシステム
  #include "adp.h"  // 通信アダプタ共通へ公開
  #include "dev.h"  // 統合デバイス定義（devBLEの公開資源を含む）
  //┴
//┴

//########################################################
//# 専用名の前空間
//########################################################
namespace adpBLE {
//========================================================
// Ａ．基本情報
//========================================================
  //─────────────────
  // ステータス
  //─────────────────
  const int  ROUTE_ID  = ROUTE_ID_BLE; // ＢＬＥ
        bool ENABLED   = false       ; // 有効性：{有効：true|無効：false}
        bool CONNECTED = false       ; // 接続中

  //─────────────────
  // 使用するサービス
  //─────────────────
  // ※BLEはサービスポートを持たないため、
  //   dev.hで公開されたBLE固有の受付資源を使用
  // ・BLE_RX：受信用キャラクタリスティック
  // ・BLE_TX：送信用キャラクタリスティック

//========================================================
// Ｂ．レスポンス
//========================================================
  //─────────────────
  // スロットの受付資源に送信
  //─────────────────
  void SEND_CONN(uint8_t argClientID){
    //┬
    //○メッセージをレスポンス
    if (devBLE::BLE_TX != nullptr) {
      devBLE::BLE_TX->setValue(ctx.resMSG.c_str());
      devBLE::BLE_TX->notify();
    } /* END-if */
    //│
    //●ログ出力
    F_SHOW_LOG();
    //┴
  } /* SEND_CONN() */

//========================================================
// Ｃ．リクエスト管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
  const int WAIT_MS = 15        ; // 受信タイムラグ
  struct myQueue {
    uint8_t connNum; // クライアント番号
    String  connRX ; // 受信バッファ
  };
  static std::queue<myQueue> QUEUE      ; // キューバッファ
  static std::mutex          QUEUE_MUTEX; // 別スレッドとの衝突回避用のロック

  //─────────────────
  // キューの取り出し
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

  //─────────────────
  // リクエストの登録
  //----------------------------------
  // コールバック関数として機能
  //─────────────────
  //━━━━━━━━━━━━━━━━━
  // コールバック：サーバ用
  //━━━━━━━━━━━━━━━━━
  class Callback_Server : public BLEServerCallbacks {
    //─────────────────
    // 接続イベント：接続制限（同時1人）
    //─────────────────
    void onConnect(BLEServer* pServer) override {
      //┬
      //○接続状況を確認
      if (CONNECTED) return;
      //│＼（既に参加している場合）
      //│ ▼終了：これ以上は参加させない
      //│
      //○ステータスを変更（接続済）
      //○アドバタイジングを停止(新規の侵入を物理的に防ぐ)
      CONNECTED = true;
      if (devBLE::MY_SRV != nullptr) devBLE::MY_SRV->getAdvertising()->stop();
      //┴
    } /* onConnect() */

    //─────────────────
    // 切断イベント：接続制限を解除
    //─────────────────
    void onDisconnect(BLEServer* pServer) override {
      //┬
      //○アドバタイジングを再開
      //○ステータスを変更（未接続）
      if (devBLE::MY_SRV != nullptr) devBLE::MY_SRV->startAdvertising();
      CONNECTED = false;
      //┴
    } /* onDisconnect() */
  }; /* Callback_Server */

  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  class Callback_Client : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      //┬
      //○未取り込みデータを受信（getValue()参照後は消費されない）
      delay(WAIT_MS);
      String rxData = pCharacteristic->getValue(); // データ複製
      if (rxData.length() < 1) return;
      //│＼（空の場合）
      //│ ▼終了：早期リターン
      //│
      //○受信データをキューに追加
      std::lock_guard<std::mutex> lock(QUEUE_MUTEX);
      QUEUE.push({0,rxData});
      //┴
    }; /* onWrite() */
  }; /* Callback_Client */

  static Callback_Server ON_CONNECTION; // 接続・切断
  static Callback_Client ON_RECIVE    ; // データ受信

//========================================================
// Ｄ．公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START() {
    //┬
    //○サービスを開始
    devBLE::MY_SRV->setCallbacks(&ON_CONNECTION); // サーバ(接続/切断)
    devBLE::BLE_RX->setCallbacks(&ON_RECIVE    ); // サーバ(接続/切断)
    //│
    //○アダプタを有効化
    ENABLED = true; 
    Serial.println("　[OK] Bluetooth");
    //┴
  } /* START() */

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口（ポーリング入口）
  //━━━━━━━━━━━━━━━━━
  void HANDLE(){
    //┬
    //○起動チェック
    if (!ENABLED) return; // 初期化済み
    //│＼（このアダプタが無効の場合）
    //│ ▼終了：早期リターン
    //│
    //◎┐ルーティングを指示
    myQueue popDat;
    while (popQueue(popDat)) {
      //│＼（キューが空の場合）
      //│ ▼BREAK：ルーティングを終了
      //│
      //●キュー情報をワークにセット
      F0_SETUP(popDat.connRX);
      //│
#if defined(MMP_TYPE_MAIN) // --┨ＭＭＰ本体┠----┐
      //○リクエストをデータ項目に分解
      F3_SET_ACD_CPATH();
      //│
      //●認証処理を実施
      if (F4_CHECK_AUTH()){SEND_CONN(popDat.connNum); continue;}
      //│＼（処理継続が不可の場合）
      //│ ●エラーをレスポンス
      //│ ▽次へ：次のキューを走査
#endif // ----------------------------------------┘
      //│
      //●コマンド実行
      F5_RUN();
      //│
      //●実行結果をレスポンス
      SEND_CONN(popDat.connNum);
      //┴
    } /* END-while */
    //┴
  } /* HANDLE() */

} /* namespace adpBLE */