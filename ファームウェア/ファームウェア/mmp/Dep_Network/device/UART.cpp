// filename : Dev_Network/device/UART.cpp
//========================================================
// 通信部門／デバイス課：UART 担当
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/17)
// ・UARTはここですべて管理
// ・UARTポートの見直し 
//========================================================

//########################################################
//# 処理詳細
//########################################################
namespace devUART {
//========================================================
// 共有資源
//========================================================
   //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
  bool ENABLED = false; // 有効判定：有効：true、無効：false

  //━━━━━━━━━━━━━━━━━
  // ボーレート
  //━━━━━━━━━━━━━━━━━
  const int BPS       = 921600;
  const int BPS_LOG   = 921600;
  const int BPS_CROSS = 3000000;

  //━━━━━━━━━━━━━━━━━
  // ピンアサイン
  //━━━━━━━━━━━━━━━━━
  const int PIN0_RX = 43; // ログ用
  const int PIN0_TX = 44;
  const int PIN1_RX = (MODE == MODE_MAIN) ? 17 : 8; // main<=>sub or Client
  const int PIN1_TX = (MODE == MODE_MAIN) ? 18 : 9;
  const int PIN2_RX = (MODE == MODE_MAIN) ? 11 :10; // MP3 or Client
  const int PIN2_TX = (MODE == MODE_MAIN) ? 12 :11;

  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START(){
    char msg[100];
    String msgUSB = "   [--] USB CDC";
    String msg0   = "   [--] Serial #0";
    String msg1   = "   [--] Serial #1";
    String msg2   = "   [--] Serial #2";
    //│
    //○USB CDCを起動(クライアント用)
    Serial.begin(BPS);
    msgUSB = "   [OK] USB CDC <=> Client";
    //│
    //○Serial0を起動(ログ用)
    Serial0.begin(BPS_LOG, SERIAL_8N1, PIN0_RX, PIN0_TX);
    snprintf(msg, sizeof(msg), "   [OK] Serial #0 -> %d bps ==> log", BPS_LOG);
    msg0 = msg;
    //│
    //○Serial1を起動(メイン・サブ：クロス接続用／ブリッジ：クライアント用)
    int intBps = (MODE == MODE_BRIDGE) ? BPS : BPS_CROSS;
    Serial1.begin(intBps, SERIAL_8N1, PIN1_RX, PIN1_TX);
    snprintf(msg, sizeof(msg), "   [OK] Serial #1 -> %d bps / Rx:%d Tx:%d", intBps, PIN1_RX, PIN1_TX);
    msg1 = msg; msg1 += (MODE == MODE_BRIDGE) ? " <=> Client" : " / main <=> sub";
    //│
//------------------------------------
// MAINはMP3で使用するのでbeginしない
//------------------------------------
#if MODE != MODE_MAIN
    //○Serial2を起動(クライアント)
      Serial2.begin(BPS, SERIAL_8N1, PIN2_RX, PIN2_TX);
      snprintf(msg, sizeof(msg), "   [OK] Serial #2 -> %d bps / Rx:%d Tx:%d <=> Client", BPS, PIN2_RX, PIN2_TX);
      msg2 = msg;
#endif
//------------------------------------
    //│
    //○メッセージ表示
    delay(2000);
    Log::prtln("<<通信デバイスの初期化>>");
    Log::prtln(" [UART]"  );
    Log::prtln(msg0);
    Log::prtln(msg1);
    Log::prtln(msg2);
    //│
    //○有効性セット
    ENABLED = true;
  } /* START() */
} /* namespace devUART */