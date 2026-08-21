// filename : adpBLE.cpp
//========================================================
// 通信アダプタ：ＢＬＥブリッジ
//--------------------------------------------------------
// 2026/08/21 : 新設
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <BLEDevice.h> // ユーザ受付資源を
  #include <BLEServer.h> // ユーザ受付資源
  #include <BLEUtils.h > // ユーザ受付資源
  #include <BLE2902.h  > // ユーザ受付資源
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
  // dev.hで公開している以下を使用
  // ・MY_SRV：BLEサーバー実体
  // ・BLE_RX：受信用キャラクタリスティック
  // ・BLE_TX：送信用キャラクタリスティック

  //─────────────────
  // 接続スロット
  //─────────────────
  struct T_SS_SLOT : SS_SLOT_TYPE{ // ※共通テンプレートから派生
    String             conn_rx  = ""     ; // 受信バッファ（文字列ワーク）
    BLECharacteristic* conn_tx  = nullptr; // 送信資源（参照）
  };
  static T_SS_SLOT*    ssTBL    = nullptr; // 事前予約

//========================================================
// Ｂ．接続管理
//========================================================
  //─────────────────
  // 初期化
  //─────────────────
  void SS_INI_SLOT(T_SS_SLOT& argSlot){
    SS_INI_SLOT_BASE(argSlot) ; // 共通メンバを初期化
    argSlot.conn_rx = ""      ; // 受信バッファをクリア
    argSlot.conn_tx = nullptr ; // 送信資源の参照解除
  } /* SS_INI_SLOT */

  //─────────────────
  // 空きSID取得
  //----------------------------------
  // ➡【該当処理なし】※固定スロット
  //─────────────────
  int SS_GET_FREE_ID(){}

  //─────────────────
  // 登録
  //─────────────────
  void SS_ATTACH_SLOT(){
    //┬
    //●スロットを初期化
    SS_INI_SLOT(ssTBL[0]);
    //│
    //○スロットに新規接続を登録
    ssTBL[0].used    = true          ; // 使用中
    ssTBL[0].conn_tx = devBLE::BLE_TX; // 参照先を登録
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
    if (ctx.sysLog >= 0) LOG_PRINT(argMSG);
    //│
    //○メッセージをレスポンス
    if (argSS.conn_tx != nullptr) {
      argSS.conn_tx->setValue(argMSG.c_str());
      argSS.conn_tx->notify();
    } /* END-if */
    //┴
  } /* SEND_CONN() */

//========================================================
// Ｄ．プロセス部品
//========================================================
  //─────────────────
  // １．接続状態を確認
  //----------------------------------
  // 戻り値：接続状態（論理値）
  // ・false：接続状態が「良好」
  // ・true ：接続状態が「不良」
  //----------------------------------
  //【詳細】不正の場合：スロットを保持
  //─────────────────
  bool P1_CONNECT(T_SS_SLOT& argSS){
    //┬
    //○接続状態を確認
    if (!argSS.used) {
    //│ ＼（[未使用]の場合）
        //●スロットを初期化
        //▼RETURN：接続状態が「不良」
        SS_INI_SLOT(argSS);
        return true;
    } // END-if */
    //│
    //▼RETURN：接続状態が「良好」
    return false;
    //┴
  } /* P1_CONNECT() */

  //─────────────────
  // ２．フレームを取得
  //----------------------------------
  // 戻り値：フレーム作成状況（論理値）
  // ・true ：フレームが「未完成」
  // ・false：フレームが「完成」
  //----------------------------------
  //【データ受信方式】
  // ・取得単位：パケット
  // ・取得対象：サーバ(参照) pCharacteristic->getValue()
  //─────────────────
  bool P2_MAKE_FRAME(T_SS_SLOT&  argSS){
    //┬
    //○受信データからフレームを作成
    ctx.strFrame = argSS.conn_rx;
    //│
    //●フレームをURI形式に変換
    P2_FORMAT_URI(ctx.strFrame);
    //│
    //▼RETURN:フレームの作成状況
    return (ctx.strFrame == "" ? true : false);
    //┴
  } /* P2_MAKE_FRAME() */

  //─────────────────
  // ３．基本情報を取得
  //----------------------------------
  //【詳細】
  // フレーム書式    ：{コマンドパス}!
  //─────────────────
  void P3_MAKE_INFO(){
    //┬
    //〇フレームの内容をもとに認証CD・コマンドパスにセット
    P3_SET_ACD_CPATH();
    //┴
  } /* P3_MAKE_INFO() */

  //─────────────────
  // ４．認証を実施
  //----------------------------------
  // 戻り値：認証結果（論理値）
  // ・false： 処理続行の判定が「許可」
  // ・true ： 処理続行の判定が「不許可」
  //─────────────────
  bool P4_AUTH(T_SS_SLOT&  argSS){
    //┬
    //○認証処理をおこなう
    String strRes = P4_CHECK_AUTH();
    if (strRes != "") {SEND_CONN(argSS, strRes); return true;}
    //│ ＼（レスポンスメッセージがある場合）
        //▼RETURN：処理続行の判定が「不許可」
    //│
    //▼RETURN：処理続行の判定が「不許可」
    return false;
    //┴
  } /* P4_AUTH() */

//========================================================
// Ｅ．ルーティング処理（プロセス）
//--------------------------------------------------------
// BLEサーバが別プロセスで自動的に実行
//========================================================
  void routeMMP(T_SS_SLOT& argSS){
    //┬
    //○１．接続状態を確認
    if (P1_CONNECT(argSS)) return;
    //│ ＼（接続状態が「不良」の場合）
        //▼RETURN：早期リターン
    //│
    //●２．フレームを取得
    if (P2_MAKE_FRAME(argSS)) return;
    //│ ＼（フレームが「未完成」の場合）
        //▼RETURN：早期リターン
    //│
#if defined(MMP_TYPE_MAIN) // --┨ＭＭＰ本体┠----┐
    //○３．基本情報を取得
    P3_MAKE_INFO();
    //│
    //○４．ユーザ認証を実施
    if (P4_AUTH(argSS)) return;
    //│ ＼（処理続行の判定が「不許可」の場合）
        //▼RETURN：早期リターン
#endif // ----------------------------------------┘
    //│
    //●５．MMPコマンドを実行
    String resMMP = P5_RUN();
    //│
    //●６．実行結果をレスポンス
    SEND_CONN(argSS, resMMP);
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
      //│ ＼（既に参加している場合）
          //▼RETURN：これ以上は参加させない
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
      ssTBL[0].conn_rx = pCharacteristic->getValue();
      if (ssTBL[0].conn_rx.length() < 1) return;
      //│ ＼（空の場合）
          //▼RETURN：早期リターン
      //│
      //○１．起動チェック
      if (!ENABLED) return;
      //│ ＼（無効の場合）
          //▼RETURN：早期リターン
      //│
      //○２．新規接続のスロットを登録
      // ➡【該当処理なし】※start()で登録済み
      //│
      //○┐３．ルーティング処理
        //●コンテクストをセットアップ
        //●MMPコマンドへルーティング
        SETUP_CTX(ROUTE_ID, 0);
        routeMMP(ssTBL[0]);
        //┴
      //┴
    }; /* onWrite() */
  }; /* Callback_Client */

  static Callback_Server ON_CONNECTION; // 接続・切断
  static Callback_Client ON_RECIVE    ; // データ受信

  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //----------------------------------
  // ロジックで明示的に呼び出す ※handle()参照
  //━━━━━━━━━━━━━━━━━
  void start() {
    //┬
    //○１．前準備の完了状態を確認
    if (!devBLE::ENABLED) devBLE::start();
    if (!devBLE::ENABLED || !devBLE::MY_SRV) {
    //│ ＼（通信デバイスが起動していない場合）
        //○エラーメッセージを表示
        //○無効化
        //▼異常終了
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
      Serial.println(" BLE Bridge : OK");
      ENABLED = true;
    //┴
  } /* start() */

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口（ポーリング入口）
  // ➡【該当処理なし】※イベント駆動
  //━━━━━━━━━━━━━━━━━

} /* namespace adpBLE */