// filename : Dev_Network/manager.cpp
//========================================================
// 通信設備部門：部門長
//--------------------------------------------------------
// Ver 1.4.0 (2026/10/01)
//========================================================
//┬
//□┐通信設備部門
  //□担当：通信デバイス
  #include "device/_index.h"
//┴┴

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
    const char* name        ; // デバイス名
    bool*       pEnabled    ; // 有効フラグへのポインタ
    void        (*pHandle)(); // ハンドル関数ポインタ
  }; /* struct */
  //┴
  //┬
  //□┐部下（通信デバイス）を招集
  static const T_RECORD DB[] = {
    //│
    //□UART担当 ※メインモード以外は強制
    { "UART", &devUART::ENABLED, devUART::HANDLE },
    //│
    //□IIC担当 ※メインモードは(PWMモジュールが使用)
    #ifdef USE_IIC
    { "IIC", &devIIC::ENABLED, devIIC::HANDLE },
    #endif
    //│
    //□WiFi担当
    #ifdef USE_WiFi
    { "WiFi", &devWiFi::ENABLED, devWiFi::HANDLE },
     #endif
    //│
    //□Bluetooth担当
    #ifdef USE_BLUETOOTH
    { "BLE", &devBLE::ENABLED, devBLE::HANDLE },
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
    //◎┐デバイス課の担当に業務遂行を指示
    for (size_t devID = 0; devID < DBs; ++devID) {
      //│＼（全員に指示を終えた場合）
      //│ ▽完了:業務遂行の指示を終える
      //│
      //○この担当に指示
      const auto& dev = DB[devID];
      dev.pHandle();
      //┴
    } //～for
    //┴
  } /* INIT() */

} /* namespace DepNetwork */