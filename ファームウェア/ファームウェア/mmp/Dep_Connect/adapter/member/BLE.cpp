// filename : Dep_Connect/adapter/member/BLE.cpp
//========================================================
// 接続部門／業務課／担当(標準型)：BLE 担当
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/11)
//========================================================
//┬
//□┐インクルード
  #include <BLEDevice.h>
  #include <BLE2902.h>
//┴┴

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
//§基本情報
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 一般情報
  //━━━━━━━━━━━━━━━━━
    const int ADP_ID = ADP_ID_BLE;
    int getAID() const override {return ADP_ID;} // 基底クラスに連携

  //━━━━━━━━━━━━━━━━━
  // サービス関連情報
  //━━━━━━━━━━━━━━━━━
    static AdapterBLE* MY_INSTANS; // 静的コールバックからのルーティング用

//========================================================
//§返信処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // クライアントにレスポンス
  //━━━━━━━━━━━━━━━━━
  void SEND_CONN(uint8_t argConn) override {
#if (MODE != MODE_BRIDGE)
    //┬
    //○クライアントにレスポンス
    if (devBLE::BLE_TX != nullptr && devBLE::ENABLED) {
      devBLE::BLE_TX->setValue(ctx.resMSG.c_str());
      devBLE::BLE_TX->notify(); // 接続クライアントへ通知（Notify）
    }
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
#endif
  } /* SEND_CONN() */

//========================================================
//§受信処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // コールバック：サーバー受信用
  //━━━━━━━━━━━━━━━━━
#if (MODE != MODE_BRIDGE)
  class ServerCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      //┬
      //○インスタンスを確認
      if (!MY_INSTANS) return;
      //│＼（通信デバイスが起動していない場合）
      //│ ▼終了：早期リターン
      //│
    //○未取り込みデータを受信
      String rxValue = pCharacteristic->getValue();
      if (rxValue.length() <= 0) return;
      //│＼（空の場合）
      //│ ▼終了：早期リターン
      //│
      //●受信データをキューに追加
      MY_INSTANS->pushQueue(0, rxValue, 0);
    } /* onWrite() */
  }; /* class ServerCallbacks */
#endif

#if (MODE == MODE_BRIDGE)
  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント受信用
  //━━━━━━━━━━━━━━━━━
  static void ON_RECIVE_NOTIFY(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t*  pData,
    size_t    length,
    bool      isNotify
  ) {
    //┬
    //○インスタンスを確認
    if (!MY_INSTANS) return;
    //│＼（通信デバイスが起動していない場合）
    //│ ▼終了：早期リターン
    //│
    //○未取り込みデータを受信
    if (pData == nullptr || length < 1) return;
    //│＼（空の場合）
    //│ ▼終了：早期リターン
    //│
    //●受信データをキューに追加
    MY_INSTANS->pushQueue(0, String((char*)pData, length), 0);
    //┴
  } /* ON_RECIVE_NOTIFY() */
#endif

//############################
//# 転送機能はブリッジのみ
//############################
#if (MODE == MODE_BRIDGE)
//========================================================
//§転送処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 転送実施
  //━━━━━━━━━━━━━━━━━
  void trans() override final {
    //┬
    //○初期化資源の接続健全性を確認
    if (!devBLE::ENABLED || devBLE::MY_CLI == nullptr || !devBLE::MY_CLI->isConnected())
      {ctx.strFrame = "#CNT!"; return;}
    //│
    //○確立済みの通信口（RX）へリクエストを書き込み（非同期送出）
    if (devBLE::BLE_CLI_RX != nullptr)
      devBLE::BLE_CLI_RX->writeValue(ctx.strFrame.c_str(), ctx.strFrame.length());
    //┴
  };
#endif
//############################

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterBLE(MmpContext& argCtx) : AdapterQueueBase(argCtx) {
#if (MODE == MODE_BRIDGE)
    //┬
    //○インスタンスを取得
    MY_INSTANS = this;
    //│
    //○devBLE::START() で作成済みの通知受信用キャラクタリスティックへコールバック登録
    if (devBLE::BLE_CLI_TX != nullptr && devBLE::BLE_CLI_TX->canNotify())
      devBLE::BLE_CLI_TX->registerForNotify(ON_RECIVE_NOTIFY);
    //│
    //○メッセージ表示
    Serial.println(" [OK] BLE");
    //┴
#else
    //┬
    //○インスタンスを取得
    MY_INSTANS = this;
    //│
    //○受信コールバックを登録
    if (devBLE::BLE_RX != nullptr) devBLE::BLE_RX->setCallbacks(new ServerCallbacks());
    //│
    //○メッセージ表示
    Serial.printf(" [OK] BLE (NAME=[%s])\n", devBLE::MY_NAME);
    //┴
#endif
  } /* constractor AdapterBLE() */

}; /* class AdapterBLE */

//━━━━━━━━━━━━━━━━━
// インスタンス管理用
//━━━━━━━━━━━━━━━━━
AdapterBLE* AdapterBLE::MY_INSTANS = nullptr;