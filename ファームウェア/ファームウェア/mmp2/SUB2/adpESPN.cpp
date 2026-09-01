// filename : adpESPN.cpp
//========================================================
// 通信アダプタ：ＥＳＰ－ＮＯＷ
//--------------------------------------------------------
// Ver 1.2.1 (2026/08/27) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■┐Arduinoシステム
  #include <WiFi.h>
  #include <esp_now.h>
  //│
  //■ＭＭＰシステム
  #include "adp.h"  // 通信アダプタ共通へ公開
  //┴
//┴

//########################################################
//# 専用名の前空間
//########################################################
namespace adpESPN {
//========================================================
// Ａ．アダプタの基本
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // ステータス
    //─────────────────
    const String ADP_ID = "ESPN"; // アダプタID

    //─────────────────
    // 使用するサービス
    //─────────────────
    // ・ESP-NOW

//========================================================
// Ｂ．レスポンス
//========================================================
void SEND_CONN(const uint8_t* argConn, String argMSG){
    //┬
    //○メッセージをレスポンス
    // 【対策1】返信相手がピアに未登録なら、ここで自動追加する
    if (!esp_now_is_peer_exist(argConn)) {
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, argConn, 6);
      peerInfo.channel = 0; // 現在のチャンネルを使用
      peerInfo.encrypt = false;
      esp_now_add_peer(&peerInfo);
    }

    // 【対策2】データ長から '+ 1' を外し、純粋な文字列の長さにする
    esp_now_send(
        argConn,                        // 送信先MACアドレス
        (const uint8_t*)argMSG.c_str(), // 送信データ
        argMSG.length()                 // 送信データ長（+1 を削除）
    );
    //┴
  } /* SEND_CONN() */

//========================================================
// Ｃ．リクエスト・キュー管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
  struct myQueue {
    uint8_t CONN[6]; // MACアドレス
    String  FRAME  ; // 受信バッファ
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
  void ON_RECIVE(
    const esp_now_recv_info_t *recv_info,
//    const esp_now_recv_info_t *recv_info,
    const uint8_t *payload,
    int length
  ) {
    //┬
    //○未取り込みデータを受信
//    if (recv_info == nullptr || payload == nullptr || length < 1) return;
    //│＼（空の場合）
    //│ ▼終了：早期リターン
    //│
    //○送信元MACアドレスを取得
    uint8_t mac[6];
    memcpy(mac, recv_info->src_addr, 6);
    //│
    //○受信データをキューに追加
    std::lock_guard<std::mutex> lock(QUEUE_MUTEX);
    myQueue pkt;
    memcpy(pkt.CONN, mac, 6);
    pkt.FRAME = String((const char*)payload, length);
    QUEUE.push(pkt);
    //│
    //▼終了：早期リターン
    return;
  } /* ON_RECIVE() */

//========================================================
// Ｅ．公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START() {
    //┬
    //○サービス資源を生成
    if (esp_now_init() != ESP_OK) {
    //│＼（通信デバイスが起動していない場合）
        //○起動ログを表示（異常終了）
        //▼終了：早期リターン
        Serial.println("　[NG ] ESP-NOW -> 初期化失敗");
        return;
    } /* END-if */
    esp_now_register_recv_cb(ON_RECIVE); // コールバック関数登録
    //│
    //○メッセージ表示
    Serial.println(String("　[OK] ESP-NOW   -> MAC ") + String(WiFi.macAddress()));
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

} /* namespace adpESPN */