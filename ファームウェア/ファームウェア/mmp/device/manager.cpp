// filename : device/manager.cpp
//========================================================
// 通信デバイス部門：統括マネージャ
//--------------------------------------------------------
// Ver 1.2.3 (2026/09/06) IICポートを追加 
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <BLEDevice.h>
  #include <BLEServer.h>
  #include <BLEUtils.h>
//┴┴
//┬
//□┐通信デバイス部門
  //□統括マネージャ
  #include "_index_.h"
//┴┴

//########################################################
//# 処理詳細
//########################################################
namespace DeviceManager{
  //━━━━━━━━━━━━━━━━━
  // 通信デバイス関数の抽象構造体
  //━━━━━━━━━━━━━━━━━
  struct T_DEVICE {
    const char* name       ; // デバイス名
          bool* pEnabled   ; // 有効フラグへのポインタ
          void  (*pStart)(); // 開始関数ポインタ
  }; /* struct T_DEVICE */

  //━━━━━━━━━━━━━━━━━
  // 通信デバイス群（抽象化・一括管理）
  //━━━━━━━━━━━━━━━━━
  static const T_DEVICE DEVICE[] = {

    #if (MODE!=MODE_MAIN)||ADP_UART //※メインモード以外は強制適用
      { "UART", &devUART::ENABLED, devUART::START },
    #endif

    #if (MODE==MODE_MAIN)||ADP_IIC //※メインモードは強制適用
      { "IIC", &devIIC::ENABLED, devIIC::START },
    #endif

    #if ADP_TCP||ADP_WAPI||ADP_WSOC||ADP_ESPN
      { "WiFi", &devWiFi::ENABLED, devWiFi::START },
    #endif

    #if ADP_BLE
      { "BLE" , &devBLE::ENABLED, devBLE::START },
    #endif
  };
  static const size_t DEVs = sizeof(DEVICE) / sizeof(DEVICE[0]);

//========================================================
// 公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 活動に必要な資源を初期化
  //━━━━━━━━━━━━━━━━━
  void INIT() {
    //┬
    //○USB(CDC)ポートを初期化
    Serial.begin(115200);          // USB(CDC)
    Serial.setDebugOutput(false);  // SDKデバッグ出力を抑止
    delay(2000);                   // 安定するまで待つ
    //│
    //○ログ表示を開始
    Serial.println("<<通信デバイスの初期化>>");
    Serial.println(" [Serial device]"  );
    Serial.println("  [OK] USB (CDC) -> 115200bps");
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