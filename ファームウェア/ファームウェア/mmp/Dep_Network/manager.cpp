// filename : Dev_Network/manager.cpp
//========================================================
// 通信部門：部門長
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/15)
// ・UARTポートの見直し 
// ・UARTを強制使用に変更
//========================================================

//========================================================
// 組織図
//========================================================
//┬
//□┐通信部門
  //□デバイス課
  #include "device/_index_.h"
//┴┴

//########################################################
//# 部門長の役務（詳細）
//########################################################
namespace DepNetwork{
//========================================================
// 共有資源
//========================================================
   //━━━━━━━━━━━━━━━━━
  // 部下を招集
  //━━━━━━━━━━━━━━━━━
  //┬
  //□座席を用意
  struct T_RECORD {
    const char* name     ; // デバイス名
      bool* pEnabled   ; // 有効フラグへのポインタ
      void  (*pStart)(); // 開始関数ポインタ
  }; /* struct */
  //┴
  //┬
  //□┐部下（通信デバイス）を招集
  static const T_RECORD DB[] = {
    //│
    //□UART担当 ※メインモード以外は強制
    { "UART", &devUART::ENABLED, devUART::START },
    //│
    //□IIC担当 ※メインモードは(PWMモジュールが使用)
    #if (MODE==MODE_MAIN) || ADP_IIC
    { "IIC", &devIIC::ENABLED, devIIC::START },
    #endif
    //│
    //□WiFi担当
    #if ADP_TCP || ADP_WAPI || ADP_WSOC || ADP_ESPN
    { "WiFi", &devWiFi::ENABLED, devWiFi::START },
     #endif
    //│
    //□BLE担当
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
  //（１）本部の始業指示に応じる
  // → 部下に業務遂行を指示
  //━━━━━━━━━━━━━━━━━
  void INIT() {
    //┬
    //○ログ出力用ポートを初期化
    Serial1.begin(SERIAL_BPS, SERIAL_8N1, devUART::PIN1_RX, devUART::PIN1_TX);
    Serial1.setDebugOutput(false);
    delay(500);                    // 安定するまで待つ
    //│
    //○始業のあいさつ（開始）
    Log::prtln("<<通信デバイスの初期化>>");
    Log::prtln(" [UART device]"  );
    Log::prtln("  [OK] USB (CDC) -> 115200bps");
    //│
    //◎┐デバイス課の担当に業務遂行を指示
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