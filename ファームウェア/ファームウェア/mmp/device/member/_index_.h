// filename : device/member/_index_.h
//========================================================
// 通信デバイス部門／担当：通信基盤
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#ifndef DEV_MEMBER_H
#define DEV_MEMBER_H
#pragma once

//========================================================
// 役割
//========================================================
  //━━━━━━━━━━━━━━━━━
  // UARTポート
  //━━━━━━━━━━━━━━━━━
  #include "UART.cpp"
  namespace devUART{
    extern bool ENABLED ; // 有効性
    void START()        ; // デバイス開始の指示
  }

  //━━━━━━━━━━━━━━━━━
  // WiFiサーバ
  //━━━━━━━━━━━━━━━━━
  #include "WiFi.cpp"
  namespace devWiFi{
    extern bool ENABLED ; // 有効性
    void START()        ; // デバイス開始の指示
  }

  //━━━━━━━━━━━━━━━━━
  // BLEサーバ
  //━━━━━━━━━━━━━━━━━
  class BLEServer                   ; // 前方宣言(ヘッダー非依存化)
  class BLECharacteristic           ; // 前方宣言(ヘッダー非依存化)
  #include "BLE.cpp"
  namespace devBLE{
    extern bool ENABLED             ; // 有効性
    void START()                    ; // デバイス開始の指示
    //※BLE固有の実体ポインタ（前方宣言型を利用）
    extern BLEServer*         MY_SRV; // BLEサーバー
    extern BLECharacteristic* BLE_RX; // 受信用キャラクタリスティック
    extern BLECharacteristic* BLE_TX; // 送信用キャラクタリスティック
  } /* namespace devBLE */

#endif // DEV_MEMBER_H