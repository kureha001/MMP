// filename : Dep_Connect/adapter/BLE/BLE_MainSub.cpp
//========================================================
// 接続部門／担当：BLE（メインモード・サブモード）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/28)
//========================================================

//========================================================
//§開始処理・終了処理
//========================================================
  //───────────────────────────
  // 開始処理：コンストラクタ
  //───────────────────────────
  String CONSTRACT() {
  //┬
  //○┐【前処理】
    //○接続状況を確認
    if (
      !devBLE::ENABLED ||
      devBLE::BLE_RX == nullptr
    ) return String(" [NG] BLE(Rx)");
    //│＼（切断の場合）
    //│ ▼終了：早期リターンする
    //┴
  //│
  //○┐【主処理】
    //○コールバックを登録（データ受信）
    MY_TASK = this;
    devBLE::BLE_RX->setCallbacks(new ServerCallbacks());
    //┴
  //│
  //○┐後処理
    //▼：返却：起動ログ表示のMSG
    char msg[100];
    snprintf(msg, sizeof(msg), " [OK] BLE(Rx) / NAME.%s", devBLE::MY_NAME);
    return String(msg);
  //┴
  } /* CONSTRACT() */

  //───────────────────────────
  // 終了処理：接続元にMSGをレスポンスする
  //───────────────────────────
  void SEND_CONN(uint8_t argConn) override final {
  //┬
  //○┐【前処理】
    //○サーバ・通信口(TX)の接続状態を確認
    if (!devBLE::ENABLED || devBLE::BLE_TX == nullptr) return
    //│＼（状態が[未接続]の場合）
    //│ ▼終了：早期リターンする
    //┴
  //│
  //○┐【主処理】
    //○接続元にレスポンスMSGを送信する
    devBLE::BLE_TX->setValue(ctx.base.Msg.c_str());
    devBLE::BLE_TX->notify(); // 接続クライアントへ通知（Notify）
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */

//========================================================
//§受信処理
//========================================================
  //───────────────────────────
  // 並列処理を定義：Rx用
  //───────────────────────────
  class ServerCallbacks :
  public BLECharacteristicCallbacks {
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