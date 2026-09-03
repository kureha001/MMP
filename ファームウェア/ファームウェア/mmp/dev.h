// filename : dev.h
//========================================================
// デバイス・マネージャ：デバイスを統括する
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once
//┬
//■インクルード
  #include <Arduino.h>
  //│
  //■ＭＭＰシステム
  #include "conf.h"    // プリプロセッサ
  #include "context.h" // コンテクスト
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// 通信デバイス関数の抽象構造体
//━━━━━━━━━━━━━━━━━
struct T_DEVICE {
  const char* name       ; // デバイス名
        bool* pEnabled   ; // 有効フラグへのポインタ
        void  (*pStart)(); // 開始関数ポインタ
}; /* struct T_DEVICE */

//========================================================
// 通信デバイス・マネージャ
//========================================================
namespace DeviceManager{
  void INIT();
} /* namespace DeviceManager */

//========================================================
// メンバー（通信デバイス）
//========================================================
  //─────────────────
  // UARTポート
  //─────────────────
  namespace devUART{
    extern bool ENABLED ; // 有効性
    void START()        ; // デバイス開始の指示
  } /* namespace devUART */

  //─────────────────
  // WiFiサーバ
  //─────────────────
  namespace devWiFi{
    extern bool ENABLED ; // 有効性
    void START()        ; // デバイス開始の指示
  } /* namespace devWiFi */

  //─────────────────
  // BLEサーバ
  //─────────────────
  class BLEServer        ; // 前方宣言(ヘッダー非依存化)
  class BLECharacteristic; // 前方宣言(ヘッダー非依存化)
  namespace devBLE{
    extern bool ENABLED             ; // 有効性
    void START()                    ; // デバイス開始の指示
    //※BLE固有の実体ポインタ（前方宣言型を利用）
    extern BLEServer*         MY_SRV; // BLEサーバー
    extern BLECharacteristic* BLE_RX; // 受信用キャラクタリスティック
    extern BLECharacteristic* BLE_TX; // 送信用キャラクタリスティック
  } /* namespace devBLE */