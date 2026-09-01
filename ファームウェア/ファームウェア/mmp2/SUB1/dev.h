// filename : dev.h
//========================================================
// デバイスの初期化（公開情報）
//--------------------------------------------------------
// Ver 2.0.a (2026/09/01) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <BLEDevice.h>
  #include <BLEServer.h>
  #include <BLEUtils.h>
  //│
  //■ＭＭＰシステム
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// 通信デバイスの公開情報
//━━━━━━━━━━━━━━━━━
  //【UART】
  namespace devUART{
    extern bool ENABLED ; // 有効性
    void START()        ; // デバイス開始の指示
  }

  //【BLE】
  namespace devBLE{
    extern bool ENABLED             ; // 有効性
    extern BLEServer*         MY_SRV; // BLEサーバー実体
    extern BLECharacteristic* BLE_RX; // 受信用キャラクタリスティック
    extern BLECharacteristic* BLE_TX; // 送信用キャラクタリスティック
    void START()                    ; // デバイス開始の指示
  }

/*
  //【ｎRF58240】
  namespace devNRF{
    extern bool ENABLED ; // 有効性
    void START()        ; // デバイス開始の指示
  }
*/  

//━━━━━━━━━━━━━━━━━
// 通信デバイスを初期化
//━━━━━━━━━━━━━━━━━
  void INIT_DEVICE();