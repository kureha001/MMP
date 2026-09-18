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
  const int BPS     = 921600;
  const int BPS_LOG = 921600;
  const int BPS_SUB = 5000000;

  //━━━━━━━━━━━━━━━━━
  // ピンアサイン
  //━━━━━━━━━━━━━━━━━
  const int PIN1_RX = (MODE == MODE_MAIN) ? 17 : 8;
  const int PIN1_TX = (MODE == MODE_MAIN) ? 18 : 9;
  const int PIN2_RX = 11;
  const int PIN2_TX = 12;

  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START(){
    char msg[100];
    String msg0 = "   [--] USB CDC -> none";
    String msg1 = "   [--] Serial1 -> none";
    String msg2 = "   [--] Serial2 -> none";
    String msg3 = "   [--] Serial3 -> none";
    String msg4 = "   [--] Serial4 -> none";
    //│
    //○Serial0(USB CDC)を起動
    Serial.begin();
    msg0 = "   [OK] USB CDC";
    //│
    //○Serial1を起動(メインはサブ接続、その他はログ出力)
    int intBps = (MODE == MODE_MAIN) ? BPS_SUB : BPS_LOG;
    Serial1.begin(intBps, SERIAL_8N1, PIN1_RX, PIN1_TX);
    Serial1.setDebugOutput(false);
    snprintf(msg, sizeof(msg), "   [OK] Serial1 -> %d bps / Rx[%d] Tx[%d]", intBps, PIN1_RX, PIN1_TX);
    msg1 = msg; msg1 += (MODE == MODE_MAIN) ? " <=> sub-mode" : " ==> log";
    //│
    //○Serial2を起動(サブがメインに接続)
    if (MODE == MODE_SUB){
      Serial2.begin(BPS_SUB, SERIAL_8N1, PIN2_RX, PIN2_TX);
      snprintf(msg, sizeof(msg), "   [OK] Serial2 -> %d bps / Rx[%d] Tx[%d] <=> main-mode", BPS_SUB, PIN2_RX, PIN2_TX);
      msg2 = msg;
    }
    //│
    //○メッセージ表示
    delay(2000);
    Log::prtln("<<通信デバイスの初期化>>");
    Log::prtln(" [UART]"  );
    Log::prtln(msg0);
    Log::prtln(msg1);
    Log::prtln(msg2);
    Log::prtln(msg3);
    Log::prtln(msg4);
    //│
    //○有効性セット
    ENABLED = true;
  } /* START() */
} /* namespace devUART */