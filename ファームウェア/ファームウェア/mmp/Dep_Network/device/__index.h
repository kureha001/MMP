// filename : Dev_Network/device/__index.h
//========================================================
// 通信部門／デバイス課：担当名簿
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/18)
//========================================================
#pragma once

//========================================================
// 担当名簿
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
  // IICポート
  //━━━━━━━━━━━━━━━━━
  #include "IIC.cpp"
  namespace devIIC{
    extern bool ENABLED ; // 有効性
    void START()        ; // デバイス開始の指示
    bool UPDATE(int sda, int scl); // ピンアサイン変更
  }

  //━━━━━━━━━━━━━━━━━
  // WiFiサーバ
  //━━━━━━━━━━━━━━━━━
  #include "WiFi.cpp"
  namespace devWiFi{
    extern bool ENABLED ; // 有効性
    bool isConnect(bool argLog); // 有効性確認
    void START()             ; // デバイス開始の指示
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
    bool UPDATE(const char* newName); // デバイス名変更 ※ブリッジは再起動
    //※BLE固有の実体ポインタ（前方宣言型を利用）
    extern BLEServer*         MY_SRV; // BLEサーバー
    extern BLECharacteristic* BLE_RX; // 受信用キャラクタリスティック
    extern BLECharacteristic* BLE_TX; // 送信用キャラクタリスティック
  } /* namespace devBLE */