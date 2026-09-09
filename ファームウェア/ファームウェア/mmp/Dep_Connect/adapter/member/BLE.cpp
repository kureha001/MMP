// filename : Dep_Connect/adapter/base/BLE.cpp
//========================================================
// 接続部門／業務課／担当(標準型)：BLE 担当
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/10) 
//========================================================

//========================================================
// 組織図
//========================================================
//┬
//□┐接続部門
  //□┐業務課
    //□担当
    #include "_index_.h"
//┴┴┴

//########################################################
//# 処理詳細
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
  // 一般情報
  //━━━━━━━━━━━━━━━━━
    const int ADP_ID = ADP_ID_BLE;
    int getAID() const override {return ADP_ID;} // 基底クラスに連携
    static AdapterBLE* MY_INSTANS; // 静的コールバックからのルーティング用

  //━━━━━━━━━━━━━━━━━
  // サービス関連情報
  //━━━━━━━━━━━━━━━━━
    bool IS_BUSY = false; // 接続状況｛true：接続あり｜false：接続なし｝
    static const int WAIT_MS = 15 ; // 受信タイムラグ

//========================================================
// ヘルパ関数
//========================================================
#if (MODE == MODE_BRIDGE)
  //─────────────────
  // MACアドレス文字列を "XX:XX:XX:XX:XX:XX" 形式へ補正
  //─────────────────
  static String formatMacAddress(const String& rawMac) {
    // 既にコロンが含まれている場合はそのまま返却
    if (rawMac.indexOf(':') != -1) return rawMac;
    
    // 12桁連続ヘキサの場合（例: "50787D185150"）
    if (rawMac.length() == 12) {
      String formatted = "";
      for (int i = 0; i < 12; i += 2) {
        if (i > 0) formatted += ":";
        formatted += rawMac.substring(i, i + 2);
      }
      return formatted;
    }
    return rawMac;
  } /* formatMacAddress() */
#endif

//========================================================
// レスポンス
//========================================================
  //━━━━━━━━━━━━━━━━━
  // クライアントにレスポンス
  //━━━━━━━━━━━━━━━━━
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
#if (MODE != MODE_BRIDGE)
  //━━━━━━━━━━━━━━━━━
  // コールバック：サーバ用
  //━━━━━━━━━━━━━━━━━
  class Callback_Server : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
      if (!MY_INSTANS) return;
      if (MY_INSTANS->IS_BUSY) return;
      MY_INSTANS->IS_BUSY = true;
      if (devBLE::MY_SRV != nullptr) devBLE::MY_SRV->getAdvertising()->stop();
    } /* onConnect() */

    void onDisconnect(BLEServer* pServer) override {
      if (!MY_INSTANS) return;
      if (devBLE::MY_SRV != nullptr) devBLE::MY_SRV->startAdvertising();
      MY_INSTANS->IS_BUSY = false;
    } /* onDisconnect() */
  }; /* Callback_Server */
  Callback_Server ON_CONNECTION;

  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  class Callback_Client : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      if (!MY_INSTANS) return;
      delay(WAIT_MS);
      String rxData = pCharacteristic->getValue();
      if (rxData.length() < 1) return;
      MY_INSTANS->pushQueue(0, rxData);
    }; /* onWrite() */
  }; /* Callback_Client */
  Callback_Client ON_RECIVE;
#endif

//========================================================
// 担務（公開機能）
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterBLE(MmpContext& argCtx) : AdapterQueueBase(argCtx) {
    //┬
    //○インフラを確認
    if (!devBLE::ENABLED) return;
    //│
    //○インスタンスを登録
    MY_INSTANS = this;
    //│
#if (MODE == MODE_BRIDGE)
    //○メッセージ表示（ブリッジモード）
    Serial.println(" [OK] BLE Clieant");
#else
    //○サービス資源を生成（サーバーモード）
    devBLE::MY_SRV->setCallbacks(&ON_CONNECTION); // サーバ(接続/切断)
    devBLE::BLE_RX->setCallbacks(&ON_RECIVE    ); // クライアント(受信)
    Serial.println(" [OK] BLE Server");
#endif
    //┴
  } /* constractor AdapterBLE() */


#if (MODE == MODE_BRIDGE)
  //━━━━━━━━━━━━━━━━━
  // 転送受付（確立済みコネクションへの連続コマンド発行）
  //━━━━━━━━━━━━━━━━━
  void trans() override {
    //┬
    //○接続インフラの健全性を確認
    if (!devBLE::ENABLED || devBLE::MY_CLI == nullptr || !devBLE::MY_CLI->isConnected()) {
      ctx.resMSG = "#CNT!";
      return;
    }

    //○リクエストを遠隔RXポートへ書き込み（Write）
    devBLE::BLE_CLI_RX->writeValue(ctx.strFrame.c_str(), ctx.strFrame.length());

    //○レスポンスを遠隔TXポートから取得（Read）
    if (devBLE::BLE_CLI_TX != nullptr && devBLE::BLE_CLI_TX->canRead()) {
      delay(WAIT_MS);
      String strRes = devBLE::BLE_CLI_TX->readValue();
      ctx.resMSG   = strRes;
      ctx.strFrame = strRes;
    } else {
      ctx.resMSG = "!!!!!"; // レスポンスなし正常終了
    }
    //┴
  };
#endif

}; /* class AdapterBLE */

//━━━━━━━━━━━━━━━━━
//インスタンス管理用
//━━━━━━━━━━━━━━━━━
AdapterBLE* AdapterBLE::MY_INSTANS = nullptr;