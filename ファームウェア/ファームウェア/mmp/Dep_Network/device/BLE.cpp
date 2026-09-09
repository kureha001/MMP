// filename : Dev_Network/device/BLE.cpp
//========================================================
// 通信部門／デバイス課：BLE 担当
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/10) 
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  // BLE全体の初期化・管理用
  // 名前設定.Central（親機）/Peripheral（子機）としての基本機能の起動
  #include <BLEDevice.h>
  // Notification（通知）/ Indication 機能の有効化
  // キャラクタリスティックに追加し、「値更新の自動通知」を有効化
  #include <BLE2902.h>
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
  //─────────────────
  #define UUID_SERVICE "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
  #define UUID_RX      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
  #define UUID_TX      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

  //─────────────────
  // アダプター層へ公開するBLE資源
  //─────────────────
  BLEServer*               MY_SRV     = nullptr; // 通常モード（ペリフェラル）用
  BLECharacteristic*       BLE_RX     = nullptr; // 通常モード受信用
  BLECharacteristic*       BLE_TX     = nullptr; // 通常モード送信用

  BLEClient*               MY_CLI     = nullptr; // ブリッジモード（セントラル）用
  BLERemoteCharacteristic* BLE_CLI_RX = nullptr; // ブリッジモード遠隔受信用
  BLERemoteCharacteristic* BLE_CLI_TX = nullptr; // ブリッジモード遠隔送信用

  const String             MY_NAME    = "MMP-ESP32S3";

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
#if (MODE == MODE_BRIDGE)
    //━━━━━━━━━━━━━━━━━
    // ブリッジモード：起動時に相手（ペリフェラル）へ初期接続を完了させる
    //━━━━━━━━━━━━━━━━━
    //○スキャナを取得して対向機器を探索
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    BLEScanResults* foundDevices = pBLEScan->start(3, false);

    BLEAdvertisedDevice* targetDevice = nullptr;
    if (foundDevices != nullptr) {
      for (int i = 0; i < foundDevices->getCount(); i++) {
        BLEAdvertisedDevice device = foundDevices->getDevice(i);
        if (device.getName() == MY_NAME.c_str()) {
          targetDevice = new BLEAdvertisedDevice(device);
          break;
        }
      }
    }

    if (targetDevice == nullptr) {
      Serial.println("   [NG] ターゲットデバイスが見つかりません");
      Serial.println("");
      ENABLED = false;
      return;
    }

    //○BLEクライアント（セントラル）を生成して接続
    MY_CLI = BLEDevice::createClient();
    if (MY_CLI == nullptr || !MY_CLI->connect(targetDevice)) {
      Serial.println("   [NG] ペリフェラルへの接続に失敗");
      Serial.println("");
      delete targetDevice;
      ENABLED = false;
      return;
    }
    delete targetDevice;
    pBLEScan->clearResults();

    //○サービスおよびキャラクタリスティックのリソースを取得・保持
    BLERemoteService* pService = MY_CLI->getService(BLEUUID(UUID_SERVICE));
    if (pService == nullptr) {
      Serial.println("   [NG] サービスが見つかりません");
      Serial.println("");
      ENABLED = false;
      return;
    }

    BLE_CLI_RX = pService->getCharacteristic(BLEUUID(UUID_RX));
    BLE_CLI_TX = pService->getCharacteristic(BLEUUID(UUID_TX));

    if (BLE_CLI_RX == nullptr || !BLE_CLI_RX->canWrite()) {
      Serial.println("   [NG] キャラクタリスティック(RX)準備に失敗");
      Serial.println("");
      ENABLED = false;
      return;
    }

    //○終了メッセージを表示
    Serial.println(String("   [OK] bridge target : ") + MY_NAME.c_str()     );
    Serial.println(String("   [OK] service UUID  : ") + String(UUID_SERVICE));
    Serial.println("");
    ENABLED = true;

#else
    //━━━━━━━━━━━━━━━━━
    // 通常モード：サーバー（ペリフェラル）として起動
    //━━━━━━━━━━━━━━━━━
    //○BLEサーバを生成
    MY_SRV = BLEDevice::createServer();
    if (MY_SRV == nullptr) {
    //│＼（サーバ生成に失敗した場合）
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
#endif
    //┴
  } /* START() */

} /* namespace devBLE */