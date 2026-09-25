// filename : Dep_Connect/adapter/BLE.cpp
//========================================================
// 接続部門／担当：BLE
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/24)
//========================================================

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス：非同期キュー型
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class AdapterBLE:
public AdapterQueueBase<uint8_t> // 接続識別子：uint8_t
{
private:
//========================================================
//§基本情報
//========================================================
  //─────────────────
  // 一般情報
  //─────────────────
  const int ADP_ID = ADP_ID_BLE;
  int getAID() const override {return ADP_ID;}

  //─────────────────
  // サービス関連情報
  //─────────────────
  static AdapterBLE* MY_TASK; // タスク識別(インスタンス)

//========================================================
//§返信処理
//========================================================
  //─────────────────
  // クライアントにレスポンス
  //─────────────────
  void SEND_CONN(uint8_t argConn) override final {
//--------------------------
//➡ブリッジ以外
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
#endif /* ➡ブリッジ以外 */
//--------------------------
  } /* SEND_CONN() */

//========================================================
//§受信処理
//========================================================
  //─────────────────
  // タスク関数：データ受信（Rx）
  //─────────────────
//--------------------------
//➡ブリッジ以外
#if (MODE != MODE_BRIDGE)
  class ServerCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    //┬
    //○インスタンスを確認
    if (!MY_TASK) return;
    //│＼（当該インスタンスではない場合）
    //│ ▼終了：早期リターンする
    //│
    //○未取り込みデータを受信
    String rxValue = pCharacteristic->getValue();
    if (rxValue.length() <= 0) return;
    //│＼（空の場合）
    //│ ▼終了：早期リターンする
    //│
    //●受信データをキューに追加
    MY_TASK->pushQueue(0, rxValue, 0);
  } /* onWrite() */
  }; /* class ServerCallbacks */
#endif /* ➡ブリッジ以外 */
//--------------------------

  //─────────────────
  // タスク関数：通知受信（Tx）
  //─────────────────
//##########################
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
  static void ON_RECIVE_NOTIFY(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* argDATA,
    size_t   argLEN,
    bool     argIsNotify
  ) {
  //┬
  //○┐【前処理】
    //○インスタンスを確認
    if (!MY_TASK) return;
    //│＼（当該インスタンスではない場合）
    //│ ▼終了：早期リターンする
    //│
    //○受信データを確認
    if (argDATA == nullptr || argLEN < 1) return;
    //│＼（空の場合）
    //│ ▼終了：早期リターンする
    //┴
  //│
  //○┐【主処理】
    //○┐キュー情報を取得
      //○フレームを取得
      String qFrame = String((const char*)argDATA, argLEN);
      //┴
    //│
    //●受信データをキューに追加
    MY_TASK->pushQueue(0, qFrame, 0);
    //┴
  //│
  //○┐【後処理】
  //┴┴
  } /* ON_RECIVE_NOTIFY() */
#endif /* ➡ブリッジ以外 */
//##########################

//========================================================
//§転送処理
//========================================================
//############################
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
  //━━━━━━━━━━━━━━━━━
  // 転送実施
  //━━━━━━━━━━━━━━━━━
  void TRANS() override final {
    //┬
    //○クライアント資源の状態を確認
    if (!devBLE::ENABLED || devBLE::MY_CLI == nullptr || !devBLE::MY_CLI->isConnected())
    {ctx.bridge.MSG = RCD::Trn1Err; return;}
    //│＼（状態が[未接続]の場合）
    //│ ○完了MSGにエラーCDをセット
    //│ ▼終了：早期リターンする
    //│
    //○通信口（RX）の状態を確認
    if (devBLE::BLE_CLI_RX == nullptr)
    {ctx.bridge.MSG = RCD::Trn2Err; return;}
    //│＼（状態が[未接続]の場合）
    //│ ○完了MSGにエラーCDをセット
    //│ ▼終了：早期リターンする
    //│
    //○退避したフレームでリクエスト(非同期でデータ受信)
    devBLE::BLE_CLI_RX->writeValue(ctx.bridge.Frame.c_str(), ctx.bridge.Frame.length());
    //┴
  } /* TRANS() */
#endif /* ➡ブリッジ */
//##########################

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // コンストラクタ：非同期キュー型
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  AdapterBLE(MmpContext& argCtx):
  AdapterBase<uint8_t>(argCtx),     // 接続識別子：uint8_t
  AdapterQueueBase<uint8_t>(argCtx) // 接続識別子：uint8_t
  {
//--------------------------
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
  //┬
  //○┐【前処理】
    //○接続状況を確認
    if (devBLE::BLE_CLI_TX == nullptr || !devBLE::BLE_CLI_TX->canNotify()) {
    //│＼（切断の場合）
        //○メッセージ表示
        //▼終了：早期リターンする
        Log::prtln(" [NG] BLE");
        return;
    } /* if */
    //┴
  //│
  //○┐【主処理】
    //○コールバック登録（通知受信）
    MY_TASK = this;
    devBLE::BLE_CLI_TX->registerForNotify(ON_RECIVE_NOTIFY);
    //┴
  //│
  //○┐後処理
    //○メッセージ表示
    Log::prtln(" [OK] BLE(Tx)");
  //┴┴
//➡ブリッジ以外
#else
  //┬
  //○┐【前処理】
    //○接続状況を確認
    if (devBLE::BLE_RX == nullptr) {
    //│＼（切断の場合）
        //○メッセージ表示
        //▼終了：早期リターンする
        Log::prtln(" [NG] BLE(Rx)");
        return;
    } /* if */
    //┴
  //│
  //○┐【主処理】
  //○コールバックを登録（データ受信）
    MY_TASK = this;
    devBLE::BLE_RX->setCallbacks(new ServerCallbacks());
  //│
  //○┐後処理
    //○メッセージ表示
    char msg[128];
    snprintf(msg, sizeof(msg), " [OK] BLE(Rx) / NAME.%s", devBLE::MY_NAME);
    Log::prtln(String(msg));
  //┴┴
#endif /* ➡ブリッジ｜➡ブリッジ以外 */
//--------------------------
  } /* constractor AdapterBLE() */

}; /* class AdapterBLE */

//========================================================
//§インスタンス管理
//========================================================
AdapterBLE* AdapterBLE::MY_TASK = nullptr;