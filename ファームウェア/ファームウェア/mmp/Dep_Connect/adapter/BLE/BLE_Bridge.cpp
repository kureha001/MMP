// filename : Dep_Connect/adapter/BLE/BLE_Bridge.cpp
//========================================================
// 接続部門／担当：BLE（ブリッジモード）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/30)
//========================================================

//========================================================
//§開始処理・終了処理
//========================================================
  //───────────────────────────
  // 開始処理：コンストラクタ
  //───────────────────────────
  String CONSTRACT() override final {
  //┬
  //○┐【前処理】
    //○接続状況を確認
    if (
      !devBLE::ENABLED               ||
       devBLE::BLE_CLI_TX == nullptr ||
      !devBLE::BLE_CLI_TX->canNotify()
    ) return String(" [NG] BLE");
    //│＼（切断の場合）
    //│ ▼終了：早期リターンする（起動ログMSG[NG]）
    //┴
  //│
  //○┐【主処理】
    //○コールバック関数を登録する（通知受信）
    MY_TASK = this;
    devBLE::BLE_CLI_TX->registerForNotify(ON_RECIVE);
    //┴
  //│
  //○┐後処理
    //▼返却：正常終了（起動ログMSG[OK]）
    return String(" [OK] BLE(Tx)");
  //┴
  } /* CONSTRACT() */

  //───────────────────────────
  // 終了処理：スレーブにリクエストを送信する
  //───────────────────────────
  void SEND_REQUEST() override final {
  //┬
  //○┐【前処理】
    //○クライアント・通信口(RX)の接続状態を確認
    if (
      !devBLE::ENABLED               ||
       devBLE::MY_CLI     == nullptr || !devBLE::MY_CLI->isConnected() ||
       devBLE::BLE_CLI_RX == nullptr
    )
    {ctx.trans.Result = RCD::Trn1Err; return;}
    //│＼（状態が[未接続]の場合）
    //│ ○処理結果にエラーCDをセット
    //│ ▼終了：早期リターンする
    //┴
  //│
  //○┐【主処理】
    //○この通信アダプタにリクエストを送信する
    devBLE::BLE_CLI_RX->writeValue(ctx.trans.Frame.c_str(), ctx.trans.Frame.length());
    //┴
  //│
  //○┐【後処理】
    //●コンテクスト・ログを出力する
    adpFnBase::LOG_CTX();
  //┴┴
  } /* SEND_REQUEST() */

//========================================================
//§受信処理
//========================================================
  //───────────────────────────
  // 並列処理を定義：Tx用
  //───────────────────────────
  static void ON_RECIVE(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* argDATA,
    size_t   argLEN,
    bool     argIsNotify
  ) {
  //┬
  //○┐【前処理】
    //○受信内容を確認する
    if (!MY_TASK || argDATA == nullptr || argLEN < 1) return;
    //│＼（[タスク違い][空データ]いずれかの場合）
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
  } /* ON_RECIVE() */