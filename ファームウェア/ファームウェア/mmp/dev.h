// filename : dev.h
//========================================================
// デバイス・マネージャ：デバイスを統括する
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/03) 
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
  #include "conf.h"
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
// デバイス・マネージャ
//━━━━━━━━━━━━━━━━━
namespace DeviceManager{
  void INIT_DEVICE();
}