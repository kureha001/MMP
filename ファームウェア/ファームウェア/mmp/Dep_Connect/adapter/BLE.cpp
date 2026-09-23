// filename : Dep_Connect/adapter/BLE.cpp
//========================================================
// 接続部門／担当：BLE(非同期キュー型)
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/23)
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
  //□担当：通信アダプタ
  #include "__index.h"
//┴┴

//########################################################
class AdapterBLE : public AdapterQueueBase<uint8_t> {
//########################################################
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
//§各種ヘルパ
//========================================================

//========================================================
//§接続管理
//========================================================

//========================================================
//§返信処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // クライアントにレスポンス
  //━━━━━━━━━━━━━━━━━
  void SEND_CONN(uint8_t argConn) override {
//############################
//# ブリッジは[trans()]で処理
//############################
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
//############################
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

//========================================================
//§ハンドル前処理
//========================================================

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
    //○クライアント資源の状態を確認
    if (!devBLE::ENABLED || devBLE::MY_CLI == nullptr || !devBLE::MY_CLI->isConnected())
    {ctx.bridge.MSG = RCD::Trn1Err; return;}
    //│＼（状態が[未接続]の場合）
    //│ ○完了MSGにエラーCDをセット
    //│ ▼終了：早期リターン
    //│
    //○通信口（RX）の状態を確認
    if (devBLE::BLE_CLI_RX == nullptr)
    {ctx.bridge.MSG = RCD::Trn2Err; return;}
    //│＼（状態が[未接続]の場合）
    //│ ○完了MSGにエラーCDをセット
    //│ ▼終了：早期リターン
    //│
    //○退避したフレームでリクエスト(非同期でデータ受信)
    devBLE::BLE_CLI_RX->writeValue(ctx.bridge.Frame.c_str(), ctx.bridge.Frame.length());
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
  AdapterBLE(MmpContext& argCtx) : AdapterBase(argCtx), AdapterQueueBase<uint8_t>(argCtx) {
    //┬
    //○┐【前処理】
      //○インスタンスを登録
      MY_INSTANS = this;
      //┴
    //│
#if (MODE == MODE_BRIDGE)
    //○┐【主処理】
      //○devBLE::START() で作成済みの通知受信用キャラクタリスティックへコールバック登録
      if (devBLE::BLE_CLI_TX != nullptr && devBLE::BLE_CLI_TX->canNotify())
        devBLE::BLE_CLI_TX->registerForNotify(ON_RECIVE_NOTIFY);
      //┴
    //│
    //○┐後処理
      //○メッセージ表示
      Log::prtln(" [OK] BLE");
      //┴
    //┴
#else
    //○┐【主処理】
      //○受信コールバックを登録
      if (devBLE::BLE_RX != nullptr) devBLE::BLE_RX->setCallbacks(new ServerCallbacks());
      //┴
    //│
    //○┐後処理
      //○メッセージ表示
      char msg[128];
      snprintf(msg, sizeof(msg), " [OK] BLE / NAME.%s", devBLE::MY_NAME);
      Log::prtln(String(msg));
      //┴
    //┴
#endif
  } /* constractor AdapterBLE() */

}; /* class AdapterBLE */

//━━━━━━━━━━━━━━━━━
// インスタンス管理用
//━━━━━━━━━━━━━━━━━
AdapterBLE* AdapterBLE::MY_INSTANS = nullptr;