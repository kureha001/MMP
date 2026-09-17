// filename : Dev_Network/device/UART.cpp
//========================================================
// 通信部門／デバイス課：UART 担当
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/15)
// ・UARTポートの見直し 
//========================================================

//########################################################
//# 処理詳細
//########################################################
namespace devUART {
//========================================================
// 共有資源
//========================================================
//========================================================
// 担務（公開機能）
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
  #if BOARD == BOARD_ESP32_S3_SUPER_MINI
    const int PIN1_RX = 8;
    const int PIN1_TX = 9;
  #else
    const int PIN1_RX = 17;
    const int PIN1_TX = 18;
  #endif

  const int PIN2_RX = 11;
  const int PIN2_TX = 12;

  bool ENABLED = false; // 有効判定：有効：true、無効：false

  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START(){

    //○通常利用ポートを起動
    Serial.begin(SERIAL_BPS);
    String strUse = "USB(CDC)";

    //○サブモードでメインモード機と接続するポートを起動
#if (MODE == MODE_SUB)
    Serial2.begin(SERIAL_BPS, SERIAL_8N1, PIN2_RX, PIN2_TX);
    strUse += ",Serial2";
#endif

    //○有効性セット
    char msg[128];
    snprintf(msg, sizeof(msg), " [OK] UART (PORT=[%s] %sbps)", strUse.c_str(), String(SERIAL_BPS).c_str());
    Log::prtln(String(msg));

    ENABLED = true;
  } /* START() */
} /* namespace devUART */