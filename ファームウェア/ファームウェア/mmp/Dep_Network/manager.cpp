// filename : Dev_Network/manager.cpp
//========================================================
// 通信部門：統括マネージャ
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

//========================================================
// 必要な資源
//========================================================
//┬
//□┐通信部門
//#include "../Dep_Network.h"
  //│
  //□自部署
  #include "_index_.h"
//┴┴

//########################################################
//# 統括マネージャの役務（詳細）
//########################################################
namespace DepNetwork{
//========================================================
// 共有資源
//========================================================
  //━━━━━━━━━━━━━━━━━
  // １．部下を招集
  //━━━━━━━━━━━━━━━━━
  //┬
  //○部下の座席を用意
  struct T_RECORD {
    const char* name     ; // デバイス名
      bool* pEnabled   ; // 有効フラグへのポインタ
      void  (*pStart)(); // 開始関数ポインタ
  }; /* struct */
  //│
  //○┐部下（通信デバイス）を招集
  static const T_RECORD DB[] = {
    //│
    //○UART担当 ※メインモード以外は強制
    #if (MODE!=MODE_MAIN) || ADP_UART
    { "UART", &devUART::ENABLED, devUART::START },
    #endif
    //│
    //○IIC担当 ※メインモードは強制(PWMモジュールが使用)
    #if (MODE==MODE_MAIN) || ADP_IIC
    { "IIC", &devIIC::ENABLED, devIIC::START },
    #endif
    //│
    //○WiFi担当
    #if ADP_TCP || ADP_WAPI || ADP_WSOC || ADP_ESPN
    { "WiFi", &devWiFi::ENABLED, devWiFi::START },
     #endif
    //│
    //○BLE担当
    #if ADP_BLE
    { "BLE" , &devBLE::ENABLED, devBLE::START },
    #endif
    //┴
  //┴
  };

  static const size_t DBs = sizeof(DB) / sizeof(DB[0]);

//========================================================
// 公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  // ２．部下に業務遂行を指示
  //━━━━━━━━━━━━━━━━━
  void INIT() {
    //┬
    //○USB(CDC)ポートを初期化
    Serial.begin(115200);          // USB(CDC)
    Serial.setDebugOutput(false);  // SDKデバッグ出力を抑止
    delay(2000);                   // 安定するまで待つ
    //│
    //○始業のあいさつ（開始）
    Serial.println("<<通信デバイスの初期化>>");
    Serial.println(" [Serial device]"  );
    Serial.println("  [OK] USB (CDC) -> 115200bps");
    //│
    //◎┐部下（通信デバイス）に業務遂行を指示
    for (size_t devID = 0; devID < DBs; ++devID) {
      //│＼（全員に指示を終えた場合）
      //│ ▽完了:業務遂行の指示を終える
      //│
      //○この担当に指示
      const auto& dev = DB[devID];
      dev.pStart();
      //┴
    } /* END-for */
    //┴
  } /* INIT() */

} /* namespace DepNetwork */