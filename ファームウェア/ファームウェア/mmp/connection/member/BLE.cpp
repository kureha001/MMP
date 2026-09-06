// filename : connection/member/BLE.cpp
//========================================================
// 経路アダプタ：BLE
//--------------------------------------------------------
// Ver 1.2.3 (2026/09/06) 
//========================================================
//┬
//■┐インクルード
  //■同僚
  #include "_index_.h"
  //│
  //■他部門連携
  #include "../../device.h" // デバイスの初期化(devBLEを参照の為）
  //│
  //■Arduinoシステム
  #include <BLEDevice.h> // ユーザ受付資源
  #include <BLEServer.h> // ユーザ受付資源
  #include <BLEUtils.h > // ユーザ受付資源
  #include <BLE2902.h  > // ユーザ受付資源
  //┴
//┴

//########################################################
//# クラス：経路アダプタ(BLE)
//########################################################
class AdapterBLE : public AdapterQueueBase<uint8_t> {
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
    // インスタンス管理用
    //（静的コールバックからのルーティング用）
    //─────────────────
    static AdapterBLE* MY_INSTANS;

    //─────────────────
    // ステータス
    //─────────────────
    const int  ADP_ID  = ADP_ID_BLE;
          bool IS_BUSY = false; // 接続状況｛true：接続あり｜false：接続なし｝

    //─────────────────
    // 使用するサービス
    //─────────────────
    // ※BLEはサービスポートを持たないため、
    //   dev.hで公開されたBLE固有の受付資源を使用
    // ・BLE_RX：受信用キャラクタリスティック
    // ・BLE_TX：送信用キャラクタリスティック
    static const int WAIT_MS = 15 ; // 受信タイムラグ

  //━━━━━━━━━━━━━━━━━
  // ID取得 (基底クラスの dispatch 処理用)
  //━━━━━━━━━━━━━━━━━
  int getAdpId() const override { return ADP_ID; }

//========================================================
// レスポンス
//========================================================
  //─────────────────
  // クライアントにレスポンス
  // ※AdapterQueueBaseをオーバーライド
  //─────────────────
  void SEND_CONN(uint8_t argConn) override {
    //┬
    //○メッセージをレスポンス
    if (devBLE::BLE_TX != nullptr) {
      devBLE::BLE_TX->setValue(ctx.resMSG.c_str());
      devBLE::BLE_TX->notify();
    } /* END-if */
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */

//========================================================
// データ受信
//========================================================
  //━━━━━━━━━━━━━━━━━
  // コールバック：サーバ用
  //━━━━━━━━━━━━━━━━━
  // ※既定のコールバック用のクラス関数をオーバーライドする
  class Callback_Server : public BLEServerCallbacks {
    //─────────────────
    // 接続イベント：接続制限（同時1人）
    //─────────────────
    void onConnect(BLEServer* pServer) override {
      //┬
      //○インスタンスを確認
      if (!MY_INSTANS) return;
      //│＼（通信デバイスが起動していない場合）
      //│ ▼終了：早期リターン
      //│
      //○接続状況を確認
      if (MY_INSTANS->IS_BUSY) return;
      //│＼（既に参加している場合）
      //│ ▼終了：これ以上は参加させない
      //│
      //○ステータスを変更（接続済）
      MY_INSTANS->IS_BUSY = true;
      //│
      //○アドバタイジングを停止(新規の侵入を物理的に防ぐ)
      if (devBLE::MY_SRV != nullptr) devBLE::MY_SRV->getAdvertising()->stop();
      //┴
    } /* onConnect() */

    //─────────────────
    // 切断イベント：接続制限を解除
    //─────────────────
    void onDisconnect(BLEServer* pServer) override {
      //┬
      //○インスタンスを確認
      if (!MY_INSTANS) return;
      //│＼（通信デバイスが起動していない場合）
      //│ ▼終了：早期リターン
      //│
      //○アドバタイジングを再開
      if (devBLE::MY_SRV != nullptr) devBLE::MY_SRV->startAdvertising();
      //│
      //○ステータスを変更（未接続）
      MY_INSTANS->IS_BUSY = false;
      //┴
    } /* onDisconnect() */
  }; /* Callback_Server */
  Callback_Server ON_CONNECTION;

  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  // ※既定のコールバック用のクラス関数をオーバーライドする
  class Callback_Client : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      //┬
      //○インスタンスを確認
      if (!MY_INSTANS) return;
      //│＼（通信デバイスが起動していない場合）
      //│ ▼終了：早期リターン
      //│
      //○未取り込みデータを受信（getValue()参照後は消費されない）
      delay(WAIT_MS);
      String rxData = pCharacteristic->getValue(); // データ複製
      if (rxData.length() < 1) return;
      //│＼（空の場合）
      //│ ▼終了：早期リターン
      //│
      //○受信データをキューに追加（基底クラスの pushQueue を呼出し）
      MY_INSTANS->pushQueue(0, rxData);
      //┴
    }; /* onWrite() */
  }; /* Callback_Client */
  Callback_Client ON_RECIVE;

//========================================================
// 公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterBLE(MmpContext& argCtx) : AdapterQueueBase(argCtx) {
    //┬
    //○インスタンスを登録
    MY_INSTANS = this;
    //│
    //○サービス資源を生成
    devBLE::MY_SRV->setCallbacks(&ON_CONNECTION); // サーバ(接続/切断)
    devBLE::BLE_RX->setCallbacks(&ON_RECIVE    ); // クライアント(受信)
    //│
    //○メッセージ表示
    Serial.println(" [OK] Bluetooth");
    //┴
  } /* constractor AdapterBLE() */
}; /* class AdapterBLE */

//━━━━━━━━━━━━━━━━━
//インスタンス管理用
//━━━━━━━━━━━━━━━━━
AdapterBLE* AdapterBLE::MY_INSTANS = nullptr;