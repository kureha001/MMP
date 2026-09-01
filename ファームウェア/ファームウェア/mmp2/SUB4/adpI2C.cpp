// filename : adpI2C.cpp
//========================================================
// 通信アダプタ：Ｉ２Ｃ
//--------------------------------------------------------
// Ver 1.2.2 (2026/08/27) 
//========================================================
#pragma once
#include <Wire.h>
#include "adp.h"  // 通信アダプタ共通へ公開

//########################################################
//# 専用名の前空間
//########################################################
namespace adpI2C {
//========================================================
// Ａ．アダプタの基本
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // ステータス
    //─────────────────
    const String ADP_ID   = "I2C"; // アダプタID
    
  //━━━━━━━━━━━━━━━━━
  // 接続管理
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // 基本情報
    //─────────────────
    static const uint8_t I2C_ADDR_MIN = 0xA0; // スレーブのI2Cアドレス（先頭）
    static const uint8_t I2C_ADDR_MAX = 0xA4; // スレーブのI2Cアドレス（末尾）
    String SEND_MSG[I2C_ADDR_MAX - I2C_ADDR_MIN + 1]; // スレーブへのレスポンスバッファ


//========================================================
// Ｂ．レスポンス
//========================================================
  void SEND_CONN(uint8_t argConn, String argMSG){
    //┬
    //○メッセージを返送バッファにセット
      SEND_MSG[argConn - I2C_ADDR_MIN] = argMSG;
    //┴
  } /* SEND_CONN() */

//========================================================
// Ｃ．リクエスト・キュー管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
    struct myQueue {
      uint8_t CONN ; // I2C Slaveアドレス
      String  FRAME; // 受信バッファ
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
    //◎┐スレーブ（I2Cアドレス）を走査
    for (uint8_t ID = I2C_ADDR_MIN; ID <= I2C_ADDR_MAX; ID++) {
      //│＼（最後のアドレスに達した場合）
      //│ ▼完了：走査を終了
      //│
      //○前処理
      String retFrame = "";
      int    nowID    = ID - I2C_ADDR_MIN;
      String msg      = SEND_MSG[nowID] == "" ? "####!" : SEND_MSG[nowID];
      SEND_MSG[nowID] = "";
      //│
      //○レスポンスをスレーブへ返信
      Wire.beginTransmission(ID);
      Wire.write((const uint8_t*)msg.c_str(),msg.length());
      Wire.endTransmission(false);
      //│
      //○リクエストをスレーブから取得
      Wire.requestFrom(ID, SS_RX_SIZE); // 指定サイズ分取得する
      while (Wire.available()) retFrame += (char)Wire.read();
      //│
      //○末尾の余分をカット
      int idx = retFrame.indexOf('!');
      if (idx < 0) continue;
      retFrame = retFrame.substring(0, idx + 1);
      if (retFrame == "!") continue;
      //│
      //○キューに登録
      std::lock_guard<std::mutex> lock(QUEUE_MUTEX); // 排他ロック
      QUEUE.push({(uint8_t)ID, retFrame});           // キューを追加(通信資源、フレーム)
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
// Ｅ．公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START() {
    //┬
    //○サービスを開始
    //  ※PWMモジュールが先行して初期化済み
    //│
    //○受信タスクをFreeRTOSの別スレッドとして起動（自動コア割当）
    xTaskCreate(
      StreamQueue,    // 実行するタスク関数
      ADP_ID.c_str(), // タスク名（デバッグ用）
      4096,           // スタックサイズ（バイト単位）
      NULL,           // パラメータ
      2,              // 優先度
      &TaskHandle     // タスクハンドル
    );
    //│
    //○メッセージ表示
    Serial.print  (String("　[OK] I2C       -> "));
    Serial.print  (String(I2C_ADDR_MIN));
    Serial.print  (" ～ ");
    Serial.println(String(I2C_ADDR_MAX));
    //┴
  } /* START() */

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口（ポーリング入口）
  //━━━━━━━━━━━━━━━━━
  void HANDLE(){
    //┬
    //◎┐ルーティングを指示
    myQueue popDat;
    while (popQueue(popDat)) {
      //│＼（キューが空の場合）
      //│ ▼完了：ルーティングを終了
      //│
      //○フレームの状態を確認
      if (popDat.FRAME.startsWith("#")){
      //│＼（エラーが発生している場合）
          //●エラーをレスポンス
          //▽次へ：次のキューを走査
          SEND_CONN(popDat.CONN, popDat.FRAME);
          continue;
      }
      //│
      //○フレームをメイン機に転送
      Serial.print(popDat.FRAME);
      //│
      //○メイン機からのレスポンスを受信
      String strMSG = "";
      while (Serial.available()) strMSG += (char)Serial.read();
      //│
      //○レスポンスをクライアントへ返却
      SEND_CONN(popDat.CONN, strMSG);
      //┴
    } /* END-while */
    //┴
  } /* HANDLE() */

} /* namespace adpI2C */