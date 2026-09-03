// filename : dev.cpp
//========================================================
// デバイス・マネージャ：デバイスを統括する
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <BLEDevice.h>
  #include <BLEServer.h>
  #include <BLEUtils.h>
  //│
  //■ＭＭＰシステム
  #include "dev.h"       // デバイスの初期化(devBLEを参照の為）
  //│
  //■ＭＭＰシステム(通信デバイス群）
  #include "device/UART.cpp"
  #include "device/WiFi.cpp"
  #include "device/BLE.cpp"
//┴

//########################################################
//# 前空間：通信デバイス・マネージャ
//########################################################
namespace DeviceManager{
  //━━━━━━━━━━━━━━━━━
  // 通信デバイス群（抽象化・一括管理）
  //━━━━━━━━━━━━━━━━━
  static const T_DEVICE DEVICE[] = {
    #if defined(ADP_UART)
      { "UART", &devUART::ENABLED, devUART::START },
    #endif
    #if defined(ADP_TCP)||defined(ADP_WAPI)||defined(ADP_WSOC)||defined(ADP_ESPN)
      { "WiFi", &devWiFi::ENABLED, devWiFi::START },
    #endif
    #if defined(ADP_BLE)
      { "BLE" , &devBLE::ENABLED, devBLE::START },
    #endif
  };
  static const size_t DEVs = sizeof(DEVICE) / sizeof(DEVICE[0]);

  //━━━━━━━━━━━━━━━━━
  // 初期化
  //━━━━━━━━━━━━━━━━━
  void INIT() {
    // --------------------------------------------------
    // 設定値の整合性チェック（ビルドガード）
    // --------------------------------------------------
    #if !defined(MMP_TYPE_MAIN) && !defined(ADP_UART)
        #error "【設定エラー】サブ構成の場合は ADP_UART の定義が必須です！"
    #endif
    #if defined(MMP_TYPE_MAIN) && !defined(ADP_UART)
        #warning "【確認】サブ機を繋げる場合は UART を有効にしてください。"
    #endif
    //┬
    //○USB(CDC)ポートを初期化
    Serial.begin(115200);          // USB(CDC)
    Serial.setDebugOutput(false);  // SDKデバッグ出力を抑止
    delay(2000);                   // 安定するまで待つ
    //│
    //○ログ表示を開始
    Serial.println("<<通信デバイスの初期化>>");
    Serial.println(" [Serial device]"  );
    Serial.println("  [OK] USB (CDC) -> 115,200bps");
    //│
    //◎┐通信デバイスを初期化
    for (size_t devID = 0; devID < DEVs; ++devID) {
      //│＼（すべての通信デバイスを走査した場合）
      //│ ▽完了：走査終了
      //│
      //○この通信デバイスを開始
      const auto& dev = DEVICE[devID];
      dev.pStart();
      //┴
    } /* END-for */
    //┴
  } /* INIT() */

} /* namespace DeviceManager */