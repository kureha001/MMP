// filename : devBLE.cpp
//========================================================
// 資源初期化：Bluetooth系
//--------------------------------------------------------
// Bluetooth通信が利用可能な状態にする
//--------------------------------------------------------
// Ver 2.0.0 (2026/08/17) α版 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <BLEDevice.h>
  #include <BLEUtils.h>
  #include <BLEServer.h>
  #include <BLE2902.h>
  //│
  //■ＭＭＰシステム
  #include "dev.h"
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  //─────────────────


//########################################################
//# 専用名の前空間
//########################################################
namespace devBLE {
//========================================================
// 基本情報
//========================================================
  bool ENABLED = false; // 有効判定：有効：true、無効：false

  // UUIDの定義
  #define UUID_SERVICE "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
  #define UUID_RX      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
  #define UUID_TX      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

  // アダプター層へ公開する外部参照資源
  BLEServer*        MY_SRV = nullptr;
  BLECharacteristic* BLE_RX = nullptr;
  BLECharacteristic* BLE_TX = nullptr;


  //========================================================
  // 初期化処理（BLEサーバの起動のみに徹する）
  //----------------------------------
  // 戻り値：なし
  //━━━━━━━━━━━━━━━━━
  void start() {
    //┬
    //○開始表示
    Serial.println(" [Bluetooth device]"  );
    //│
    //○通信デバイスの起動
    BLEDevice::init("MMP-ESP32S3");

    MY_SRV = BLEDevice::createServer();
    if (MY_SRV == nullptr) {
      Serial.println("  Bluetoothの起動に失敗");
      Serial.println("");
      ENABLED = false;
      return;
    }

    BLEService *pService = MY_SRV->createService(UUID_SERVICE);
    if (pService == nullptr) {
      Serial.println("  Bluetoothの起動に失敗");
      Serial.println("");
      ENABLED = false;
      return;
    }

    BLE_RX = pService->createCharacteristic(
      UUID_RX,
      BLECharacteristic::PROPERTY_WRITE
    );

    BLE_TX = pService->createCharacteristic(
      UUID_TX,
      BLECharacteristic::PROPERTY_NOTIFY |
      BLECharacteristic::PROPERTY_READ
    );
    BLE_TX->addDescriptor(new BLE2902());

    pService->start();

    BLEAdvertising *BLE_ADV = BLEDevice::getAdvertising();
    if (BLE_ADV == nullptr) {
      Serial.println("  Bluetoothの起動に失敗");
      Serial.println("");
      ENABLED = false;
      return;
    }
    BLE_ADV->addServiceUUID(UUID_SERVICE);
    BLE_ADV->setScanResponse(true);
    BLE_ADV->setMinPreferred(0x06);
    BLE_ADV->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();

    ENABLED = true;
    Serial.println("  Bluetoothの起動に成功");
    Serial.println("");
  } /* start() */

} /* namespace devBLE */