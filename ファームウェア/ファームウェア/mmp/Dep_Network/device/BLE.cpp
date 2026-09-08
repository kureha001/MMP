// filename : Dev_Network/device/BLE.cpp
//========================================================
// 通信部門／デバイス課：BLE 担当
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <BLEDevice.h> // BLE全体の初期化・管理用,名前設定.Central（親機）/Peripheral（子機）としての基本機能の起動
//  #include <BLEUtils.h> // UUIDのフォーマット変換,BLEの通信データを文字列やバイト配列に相互変換
//  #include <BLEServer.h> // Server（ペリフェラル / 子機）機能の構築,接続・切断時のイベントハンドラ（コールバック）を設定
  #include <BLE2902.h> // Notification（通知）/ Indication 機能の有効化,キャラクタリスティックに追加し、「値更新の自動通知」を有効化
  //┴
//┴

//########################################################
//# 処理詳細
//########################################################
namespace devBLE {
//========================================================
// 共通資源
//========================================================
  //─────────────────
  // BLE通信で使用するUUID
  //─────────────────
  // サービスUUID：MMP用BLEサービスを識別
  // RX UUID     ：MMPからBLEデバイスへの受信口（WRITE）
  // TX UUID     ：BLEデバイスからMMPへの送信口（NOTIFY/READ）
  //─────────────────
  #define UUID_SERVICE "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
  #define UUID_RX      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
  #define UUID_TX      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

  //─────────────────
  // アダプター層へ公開するBLE資源
  //─────────────────
  // MY_SRV ：BLEサーバ本体への参照
  // BLE_RX ：受信用Characteristicへの参照
  // BLE_TX ：送信用Characteristicへの参照
  //
  // これらはBLE資源そのものを所有するのではなく、
  // devBLEが生成した資源をアダプター層から利用するための公開参照。
  //─────────────────
  BLEServer*         MY_SRV  = nullptr;
  BLECharacteristic* BLE_RX  = nullptr;
  BLECharacteristic* BLE_TX  = nullptr;
  const String       MY_NAME = "MMP-ESP32S3";


//========================================================
// 担務（公開機能）
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
  bool ENABLED = false; // 有効判定：有効：true、無効：false

  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START() {
    //┬
    //○開始メッセージを表示
    Serial.println(" [Bluetooth device]"  );
    //│
    //○BLEデバイスを初期化
    // Bluetoothスタックを起動し、
    // デバイス名を「MMP-ESP32S3」として設定する。
    BLEDevice::init(MY_NAME.c_str());
    //│
    //○BLEサーバを生成
    MY_SRV = BLEDevice::createServer();
    if (MY_SRV == nullptr) {
    //│＼（サーバ生成に失敗した場合）
        //○エラーメッセージを表示
        //○無効化
        //▼終了：早期リターン
        Serial.println("   [NG] サーバ生成に失敗");
        Serial.println("");
        ENABLED = false;
        return;
    } /* END-if */
    //│
    //○MMP用BLEサービスを生成
    BLEService *pService = MY_SRV->createService(UUID_SERVICE);
    if (pService == nullptr) {
    //│＼（サービス生成に失敗した場合）
        //○エラーメッセージを表示
        //○無効化
        //▼終了：早期リターン
        Serial.println("   [NG] サービス生成に失敗");
        Serial.println("");
        ENABLED = false;
        return;
    } /* END-if */
    //│
    //○受信用Characteristicを生成
    // MMP側からBLEへデータを書き込むための受信口を作成する。
    BLE_RX = pService->createCharacteristic(
      UUID_RX,
      BLECharacteristic::PROPERTY_WRITE
    );
    //│
    //○送信用Characteristicを生成
    // BLE側からMMP側へデータを通知・読み出しするための送信口を作成する。
    BLE_TX = pService->createCharacteristic(
      UUID_TX,
      BLECharacteristic::PROPERTY_NOTIFY |
      BLECharacteristic::PROPERTY_READ
    );
    //│
    //○┐アドバタイジングを開始
      //○送信用Characteristicに通知用Descriptorを追加
      // 通知（NOTIFY）を利用するためのBLE2902 Descriptorを登録する。
      BLE_TX->addDescriptor(new BLE2902());
      //│
      //○BLEサービスを開始
      // 作成したサービスとCharacteristicをBLEサーバ上で有効にする。
      pService->start();
      //│
      //○BLEアドバタイジング資源を取得
      // 周囲のBLEクライアントから発見・接続できる状態を作るため、
      // Advertising資源への参照を取得する。
      BLEAdvertising *BLE_ADV = BLEDevice::getAdvertising();
      if (BLE_ADV == nullptr) {
      //│＼（資源取得に失敗した場合）
          //○エラーメッセージを表示
          //○無効化
          //▼終了：早期リターン
          Serial.println("   [NG] ペアリング準備に失敗");
          Serial.println("");
          ENABLED = false;
          return;
      } /* END-if */
      //│
      //○AdvertisingにMMP用サービスを登録
      // クライアントがこのBLEサービスを発見できるようにする。
      BLE_ADV->addServiceUUID(UUID_SERVICE);
      //│
      //○スキャン応答を有効化
      // BLEスキャン時の追加情報を返せるようにする。
      BLE_ADV->setScanResponse(true);
      //│
      //○接続パラメータを設定
      // BLEクライアントとの接続条件に使用される推奨値を設定する。
      BLE_ADV->setMinPreferred(0x06);
      BLE_ADV->setMaxPreferred(0x12);
      //│
      //○アドバタイジングを開始
      // ここから外部のBLEクライアントがMMPを発見して接続できる状態になる。
      BLEDevice::startAdvertising();
      //┴
    //│
    //○終了メッセージを表示
    Serial.println(String("   [OK] device name : ") + MY_NAME.c_str()     );
    Serial.println(String("   [OK] service UUID: ") + String(UUID_SERVICE));
    Serial.println(String("   [OK] recive  UUID: ") + String(UUID_RX)     );
    Serial.println(String("   [OK] send    UUID: ") + String(UUID_TX)     );
    Serial.println("");
    //│
    //○有効性セット
    ENABLED = true;
    //┴
  } /* START() */

} /* namespace devBLE */