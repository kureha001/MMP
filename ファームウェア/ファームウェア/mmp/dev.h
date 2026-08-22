// filename : dev.h
//========================================================
// 通信デバイス初期化（公開情報）
//--------------------------------------------------------
// 2026/08/21 : 新設
//========================================================
#pragma once//┬
//■┐インクルード
  //■Arduinoシステム
  #include <BLEDevice.h>
  #include <BLEServer.h>
  #include <BLEUtils.h>
  //│
  //■ＭＭＰシステム
//┴

//━━━━━━━━━━━━━━━━━
// デバイスの公開情報
//━━━━━━━━━━━━━━━━━
  //【シリアル】
  namespace devSerial{
    extern bool ENABLED ; // 有効性
    void start()        ; // デバイス開始の指示
  }

  //【ネットワーク】
  namespace devNetwork{
    extern bool ENABLED ; // 有効性
    void start()        ; // デバイス開始の指示
  }

  //【Bluetooth】
  namespace devBLE{
    extern bool ENABLED             ; // 有効性
    extern BLEServer*         MY_SRV; // BLEサーバー実体
    extern BLECharacteristic* BLE_RX; // 受信用キャラクタリスティック
    extern BLECharacteristic* BLE_TX; // 送信用キャラクタリスティック
    void start()                    ; // デバイス開始の指示
  }

//━━━━━━━━━━━━━━━━━
// 通信デバイスを初期化
//━━━━━━━━━━━━━━━━━
  void INIT_DEVICE();