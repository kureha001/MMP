// filename : dev.h
//========================================================
// デバイスの初期化（公開情報）
//--------------------------------------------------------
//【目的】
// ・通信デバイスの起動する資源を公開する
// ・通信デバイスのインタフェイスを公開する
//--------------------------------------------------------
//【公開資源】
//・通信デバイスのインタフェイス：
//  - 有効性
//  - 初期化の関数
//  - BLEのサービス(サーバ・受信・送信)
//・通信デバイスの起動を指示する関数
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/23) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <BLEDevice.h>
  #include <BLEServer.h>
  #include <BLEUtils.h>
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

  //【WiFiサーバ】
  namespace devNetwork{
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

//━━━━━━━━━━━━━━━━━
// 通信デバイスを初期化
//━━━━━━━━━━━━━━━━━
  void INIT_DEVICE();