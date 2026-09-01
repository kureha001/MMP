// filename : adpWSOC.cpp
//========================================================
// 通信アダプタ：ＷＥＢ Ｓｏｃｋｅｔ
//--------------------------------------------------------
// Ver 1.2.1 (2026/08/27) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■追加ライブラリ：WebSockets by Markus Sattler
  #include <WebSocketsServer.h>
  //│
  //■ＭＭＰシステム
  #include "adp.h"
  //┴
//┴

//########################################################
//# 専用名前空間
//########################################################
namespace adpWSOC {
//========================================================
// Ａ．アダプタの基本
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // ステータス
    //─────────────────
    const String ADP_ID  = "WSOC"; // アダプタID

    //─────────────────
    // 使用するサービス
    //─────────────────
    static WebSocketsServer* ADP_SRV = nullptr; // WebSocketサーバ
    static int               SRV_PORT = 8082  ; // ポート番号

//========================================================
// Ｂ．レスポンス
//========================================================
  void SEND_CONN(uint8_t argConn, String argMSG){
    //┬
    //○メッセージをレスポンス
    if (ADP_SRV) ADP_SRV->sendTXT(argConn, argMSG.c_str());
    //┴
  } /* SEND_CONN() */

//========================================================
// Ｃ．リクエスト・キュー管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
  struct myQueue {
    uint8_t CONN ; // アクセス資源(クライアント番号)
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
  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  void ON_RECIVE(
    uint8_t   num    , // クライアント番号
    WStype_t  type   , // エベント種別
    uint8_t * payload, // 受信データ
    size_t    length   // 受信データ長
  ){
    //┬
    //○イベントの種類を確認
    if(type !=WStype_TEXT) return;
    //│＼（テキスト以外の場合）
    //│ ▼終了：早期リターン
    //│
    //○未取り込みデータを受信
    if (payload == nullptr || length < 1) return;
    //│＼（空の場合）
    //│ ▼終了：早期リターン
    //│
    //○受信データをキューに追加
    std::lock_guard<std::mutex> lock(QUEUE_MUTEX);
    QUEUE.push({num, String((char*)payload)});
    //┴
  } /* ON_RECIVE() */

//========================================================
// Ｅ．公開機能
//========================================================
  //─────────────────
  // 初期化処理
  //─────────────────
  void START() {
    //┬
    //○サービスを開始
    ADP_SRV = new WebSocketsServer(SRV_PORT); // サーバ生成
    ADP_SRV->onEvent(ON_RECIVE)             ; // コールバック関数登録
    ADP_SRV->begin()                        ; // サーバ起動
    //│
    //○メッセージ表示
    Serial.println(String("　[OK] WebSocket -> port ") + String(SRV_PORT));
    //┴
  } /* START() */

  //─────────────────
  // ハンドラ入口（ポーリング入口）
  //─────────────────
  void HANDLE(){
    //┬
    //●WebSocketサーバの処理を進める
    ADP_SRV->loop();
    //│
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

} /* namespace adpWSOC */