// filename : adpBLE.cpp
//========================================================
// 通信アダプタ：ＢＬＥ
//（イベント・コールバック型の通信）
//--------------------------------------------------------
//【目的】
// リクエストに従い、ＭＭＰコマンドを実行して、
// 結果をレスポンスする。
//--------------------------------------------------------
//【処理機能】
//・キュー内の「全リクエスト」を順次処理する
//・リクエストを取得する
//  - コールバック時にキューへ格納
//  - ポーリング時にキューからリクエストを取得
//  - キューがない場合は何も行われない
//・リクエストを基に共通情報(コンテクスト)を纏める
//・必要に応じてユーザ認証を実施する
//・MMPコマンドを実行する
//・MMPコマンドの実行結果をレスポンする
//・処理中にエラーなどがあれば適宜レスポンスする
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/23) 
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
  // 公開情報
  //─────────────────
  const int  ROUTE_ID = ROUTE_ID_BLE   ; // ＢＬＥ
  const int  SS_SLOTS = 1              ; // 固定スロット(1個を使いまわし)
        bool ENABLED  = false          ; // 有効性：{有効：true|無効：false}

  //─────────────────
  // 使用するサービス
  //─────────────────
  // ※BLEはサービスポートを持たないため、
  //   dev.hで公開されたBLE固有の受付資源を使用
  // ・MY_SRV：BLEサーバー実体
  // ・BLE_RX：受信用キャラクタリスティック
  // ・BLE_TX：送信用キャラクタリスティック

  //─────────────────
  // 接続スロット
  //─────────────────
  struct T_SS_SLOT : SS_SLOT_TYPE{
    String             connRX  = ""     ; // 受信資源（文字列ワーク）
    BLECharacteristic* connTX  = nullptr; // 送信資源（参照）
  };
  static T_SS_SLOT*    ssTBL   = nullptr; // 事前予約

  //─────────────────
  // キューイング
  //─────────────────
  const int WAIT_MS = 15       ; // BLEの受信タイムラグ
  std::queue<String> BLE_QUEUE ; // キューバッファ
  std::mutex BLE_QUEUE_MUTEX   ; // 別スレッドとの衝突回避用のロック

//========================================================
// Ｂ．接続管理
//========================================================
  //─────────────────
  // 初期化
  //----------------------------------
  // 引数：(参照)接続管理スロット
  //─────────────────
  void SS_INI_SLOT(T_SS_SLOT& argSlot){
    SS_INI_SLOT_BASE(argSlot); // 共通メンバを初期化
    argSlot.connRX = ""      ; // 受信バッファをクリア
    argSlot.connTX = nullptr ; // 送信資源の参照解除
  } /* SS_INI_SLOT */

  //─────────────────
  // 空きSID取得
  //----------------------------------
  // 戻り値：スロットID
  // ・0,1,2...：空きスロットのID
  // ・-1：空きスロットが無い
  //─────────────────
  int SS_GET_FREE_ID(){return -1;}
  // ➡【該当処理なし】※固定スロット

  //─────────────────
  // 登録
  //─────────────────
  void SS_ATTACH_SLOT(){
    //┬
    //●スロットを初期化
    SS_INI_SLOT(ssTBL[0]);
    //│
    //○スロットに新規接続を登録
    ssTBL[0].used   = true          ; // 使用中
    ssTBL[0].connTX = devBLE::BLE_TX; // 参照先を登録
    //┴
  } /* SS_ATTACH_SLOT() */

//========================================================
// Ｃ．レスポンス
//========================================================
  //─────────────────
  // スロットの受付資源に送信
  //─────────────────
  void SEND_CONN(
    T_SS_SLOT&    argSS, // 送信先
    const String& argMSG // 送信メッセージ
  ){
    //┬
    //●ログ出力
    if (ctx.sysLog >= 0) F_SHOW_LOG(argMSG);
    //│
    //○メッセージをレスポンス
    if (argSS.connTX != nullptr) {
      argSS.connTX->setValue(argMSG.c_str());
      argSS.connTX->notify();
    } /* END-if */
    //┴
  } /* SEND_CONN() */

//========================================================
// Ｄ．プロセス部品
//========================================================
  //─────────────────
  // １．接続状態を確認
  //----------------------------------
  // 引数：(参照)接続管理スロット
  //----------------------------------
  // 戻り値：接続状態（論理値）
  // ・false：良好
  // ・true ：不良
  //----------------------------------
  //【詳細】不正の場合：スロットを保持
  //─────────────────
  bool P1_CONNECT(T_SS_SLOT& argSS){
    //┬
    //○接続状態を確認
    if (!argSS.used) {
    //│＼（[未使用]の場合）
        //●スロットを初期化
        //▼返却：不良
        SS_INI_SLOT(argSS);
        return true;
    } // END-if */
    //│
    //▼返却：良好
    return false;
  } /* P1_CONNECT() */

  //─────────────────
  // ２．フレームを取得
  //----------------------------------
  // 引数：(参照)接続管理スロット
  //----------------------------------
  // 戻り値：フレーム作成状況（論理値）
  // ・true ：未完成
  // ・false：完成
  //----------------------------------
  //【データ受信方式】
  // ・取得単位  ：パケット
  // ・データ受信：サーバ(参照) pCharacteristic->getValue()
  //─────────────────
  bool P2_MAKE_FRAME(T_SS_SLOT& argSS){
    //┬
    //○受信データからフレームを作成
    ctx.strFrame = argSS.connRX;
    //│
    //●フレームをURI形式に変換
    F2_FORMAT_URI(ctx.strFrame);
    //│
    //▼返却：フレームの作成状況
    return (ctx.strFrame == "" ? true : false);
  } /* P2_MAKE_FRAME() */

  //─────────────────
  // ３．基本情報を取得
  //─────────────────
  void P3_MAKE_INFO(){
    //┬
    //〇フレームの内容をもとに認証CD・コマンドパスにセット
    F3_SET_ACD_CPATH();
    //┴
  } /* P3_MAKE_INFO() */

  //─────────────────
  // ４．認証を実施
  //----------------------------------
  // 戻り値：認証結果（論理値）
  // ・false： 処理継続が可能
  // ・true ： 処理継続が不可
  //─────────────────
  bool P4_AUTH(T_SS_SLOT& argSS){
    //┬
    //●認証処理を実施
    String strRes = F4_CHECK_AUTH();
    if (strRes != ""){SEND_CONN(argSS, strRes); return true;}
    //│＼（メッセージがある場合）
        //●エラーをレスポンス
        //▼返却：処理継続が不可
    //│
    //▼返却：処理継続が可能
    return false;
  } /* P4_AUTH() */

  //─────────────────
  // ５．MMPコマンドを実行
  //----------------------------------
  // 引数：(参照)接続管理スロット
  //─────────────────
  void P5_RUN_COMMAND(T_SS_SLOT& argSS){
    //┬
    //●MMPコマンドを実行
    String resMMP = F5_RUN();
    //│
    //●実行結果をレスポンス
    SEND_CONN(argSS, resMMP);
    //┴
  } /* P5_RUN_COMMAND() */

//========================================================
// Ｅ．ルーティング処理（プロセス）
//--------------------------------------------------------
// BLEサーバが別プロセスで自動的に実行
//========================================================
  void routeMMP(T_SS_SLOT& argSS){
    //┬
    //○１．接続状態を確認
    if (P1_CONNECT(argSS)) return;
    //│＼（不良の場合）
    //│ ▼終了：早期リターン
    //│
    //●２．フレームを取得
    if (P2_MAKE_FRAME(argSS)) return;
    //│＼（未完成の場合）
    //│ ▼終了：早期リターン
    //│
#if defined(MMP_TYPE_MAIN) // --┨ＭＭＰ本体┠----┐
    //○３．基本情報を取得
    P3_MAKE_INFO();
    //│
    //○４．ユーザ認証を実施
    if (P4_AUTH(argSS)) return;
    //│＼（処理継続が不可の場合）
    //│ ▼終了：早期リターン
#endif // ----------------------------------------┘
    //│
    //●５．MMPコマンドを実行
    P5_RUN_COMMAND(argSS);
    //┴
  } /* routeMMP() */


//========================================================
// Ｆ．初期化・ポーリング用ハンドル
//========================================================
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // BLEイベントコールバック：サーバ用
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  class Callback_Server : public BLEServerCallbacks {
    //─────────────────
    // 接続イベント
    //─────────────────
    void onConnect(BLEServer* pServer) override {
      //┬
      //○接続状況を確認
      if (ssTBL[0].used) return;
      //│＼（既に参加している場合）
      //│ ▼終了：これ以上は参加させない
      //│
      //○アドバタイジングを停止(新規の侵入を物理的に防ぐ)
      if (devBLE::MY_SRV != nullptr) devBLE::MY_SRV->getAdvertising()->stop();
      //│
      //●スロットを割り当て
      SS_ATTACH_SLOT();
      //┴
    } /* onConnect() */

    //─────────────────
    // 切断イベント
    //─────────────────
    void onDisconnect(BLEServer* pServer) override {
      //┬
      //●スロットを初期化
      if (ssTBL != nullptr) SS_INI_SLOT(ssTBL[0]);
      //│
      //○アドバタイジングを再開
      if (devBLE::MY_SRV != nullptr) devBLE::MY_SRV->startAdvertising();
      //┴
    } /* onDisconnect() */
  }; /* Callback_Server */

  //━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // BLEイベントコールバック：クライアント用
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  class Callback_Client : public BLECharacteristicCallbacks {
    //─────────────────
    // 受信イベント
    //─────────────────
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
      std::lock_guard<std::mutex> lock(BLE_QUEUE_MUTEX);
      BLE_QUEUE.push(rxData);
      //┴
    }; /* onWrite() */
  }; /* Callback_Client */

  static Callback_Server ON_CONNECTION; // 接続・切断
  static Callback_Client ON_RECIVE    ; // データ受信

  //─────────────────
  // キューを1つ抽出
  //----------------------------------
  // 引数：
  // ・キュー受取用の変数
  //----------------------------------
  // 戻り値：キューの有無（論理値）
  // ・true ：あり
  // ・false：なし
  //─────────────────
  bool popQueue(String &rxData) {
    //┬
    //○別スレッドとの衝突回避用のロック
    std::lock_guard<std::mutex> lock(BLE_QUEUE_MUTEX);
    //│
    //○キューの容量を確認
    if (BLE_QUEUE.empty()) return false;
    //│＼（通信デバイスが起動していない場合）
    //│ ▼返却：なし
    //│
    //○先頭を抽出
    rxData = BLE_QUEUE.front();
    //│
    //○先頭を削除
    BLE_QUEUE.pop();
    //│
    //▼返却：あり
     return true;
  } /* popQueue() */

  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START() {
    //┬
    //○１．前準備の完了状態を確認
    if (!devBLE::ENABLED || !devBLE::MY_SRV) {
    //│＼（通信デバイスが起動していない場合）
        //○エラーメッセージを表示
        //○無効化
        //▼終了：早期リターン
        Serial.println(" BLE Bridge : Bluetoothサーバが起動していません ");
        ENABLED = false;
        return;
    } /* END-if */
    //│
    //○２．対象の通信経路を宣言
    ctx.routeID = ROUTE_ID; // コンテクストにルートIDをセット
    //│
    //○３．サーバ資源生成
    // ➡【該当処理なし】
    //│
    //○４．接続情報TBLを作成
    ssTBL = new T_SS_SLOT[SS_SLOTS];
    //│
    //○５．ルーティング登録
    // ➡【該当処理なし】※Webサーバが対象
    //│
    //○６．サーバ開始
    devBLE::MY_SRV->setCallbacks(&ON_CONNECTION); // サーバ(接続/切断)
    devBLE::BLE_RX->setCallbacks(&ON_RECIVE    ); // サーバ(接続/切断)
    //│
    //○┐７．成功終了
      //○成功メッセージ
      //○有効化
      Serial.println("　[OK] Bluetooth");
      ENABLED = true;
    //┴
  } /* START() */

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口（ポーリング入口）
  //━━━━━━━━━━━━━━━━━
  void HANDLE(){
    //┬
    //○１．起動チェック
    if (!ENABLED) return; // 初期化済み
    //│＼（このアダプタが無効の場合）
    //│ ▼終了：早期リターン
    //│
    //○２．新規接続のスロットを登録
    // ➡【該当処理なし】※固定スロット
    //│
    //◎┐３．ルーティング処理
    String rxData;
    while (popQueue(rxData)) {
      //│＼（キューがある場合）
      //│ ▼BREAK：ルーティングを終了
      //│
      //●対象スロットをセット
      F0_SETUP(ROUTE_ID, 0);
      //│
      //○キューデータを受信バッファにセット
      ssTBL[0].connRX = rxData;
      //│
      //●MMPコマンドへルーティング
      routeMMP(ssTBL[0]);
      //┴
    } /* END-while */
    //┴
  } /* HANDLE() */

} /* namespace adpBLE */