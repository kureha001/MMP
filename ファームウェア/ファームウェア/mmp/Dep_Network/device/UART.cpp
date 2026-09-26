// filename : Dev_Network/device/UART.cpp
//========================================================
// 通信部門／デバイス課：UART
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/19)
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
    #include <SoftwareSerial.h>
//┴┴

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
  bool ENABLED = false; // 有効判定[有効:true／無効:false]

  //━━━━━━━━━━━━━━━━━
  // ボーレート
  //━━━━━━━━━━━━━━━━━
  const int BPS       =  921600; // クライアント接続用
  const int BPS_LOG   =  921600; // ログ出力用
  const int BPS_CROSS = 3000000; // メイン・サブ連携

  //━━━━━━━━━━━━━━━━━
  // ピンアサイン
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    //【Serial0】ログ専用
    //─────────────────
    const int P0_RX = 43;
    const int P0_TX = 44;

    //─────────────────
    //【Serial1】
    // ・メ イ ン： サブとの連携用
    // ・サ　　ブ： メインとの連携用
    // ・ブリッヂ： クライアント用
    //─────────────────
    const int P1_RX = (MODE == MODE_MAIN) ? 17 : 8;
    const int P1_TX = (MODE == MODE_MAIN) ? 18 : 9;

    //─────────────────
    //【Serial2】クライアント専用
    //─────────────────
    const int P2_RX = (MODE == MODE_MAIN) ? 13 :10;
    const int P2_TX = (MODE == MODE_MAIN) ? 14 :11;

//====================================
#if MODE == MODE_MAIN
//・メ イ ン：機能モジュールで利用
//------------------------------------
    //─────────────────
    // SoftwareSerial：MP3プレイヤ専用
    //─────────────────
    const int MP3_BPS = 9600    ; // 通信速度
    const int MP3_MAX = 2       ; // 拡張可能な個数
    const int MP3_CNT = 1       ; // 実装個数
    SoftwareSerial* MP3[MP3_MAX]; // コンテナ
      //─────────────────
      // デバイス１
      //─────────────────
      const int MP3_RX1 = 11;
      const int MP3_TX1 = 12;
      //─────────────────
      // デバイス２
      //─────────────────
      const int MP3_RX2 = -1;
      const int MP3_TX2 = -1;
//------------------------------------
#endif
//====================================

  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START(){
    char msg[100];
    String msgUSB = "   [--] USB CDC"  ;
    String msg0   = "   [--] Serial #0";
    String msg1   = "   [--] Serial #1";
    String msg2   = "   [--] Serial #2";
    String msgMP3 = "";
    //│
//====================================
#if ADP_UART || MODE == MODE_BRIDGE
//・メ イ ン：任意
//・サ　　ブ：任意
//・ブリッジ：任意
//------------------------------------
    //○【USB CDC】を起動
    Serial.begin(BPS);
    msgUSB = "   [OK] USB CDC <=> Client";
//------------------------------------
#endif
//====================================
    //│
    //○【Serial0】を起動 ※全モードで強制
    Serial0.begin(BPS_LOG, SERIAL_8N1, P0_RX, P0_TX);
    snprintf(msg, sizeof(msg), "   [OK] Serial #0 -> %d bps ==> log", BPS_LOG);
    msg0 = msg;
    //│
//====================================
#if (ADP_UART || MODE != MODE_MAIN) && TURBO == false
//※ターボでは無効
//・メ イ ン：任意（無しはサブ連携不可）
//・サ　　ブ：強制（メイン連携の為）
//・ブリッジ：強制（GPIOクライアント用）
//------------------------------------
    //○【Serial1】を起動
    int intBps = (MODE == MODE_BRIDGE) ? BPS : BPS_CROSS; // 通信速度：用途別に変更
    Serial1.begin(intBps, SERIAL_8N1, P1_RX, P1_TX);
    snprintf(msg, sizeof(msg), "   [OK] Serial #1 -> %d bps / Rx:%d Tx:%d", intBps, P1_RX, P1_TX);
    msg1 = msg; msg1 += (MODE == MODE_BRIDGE) ? " <=> Client" : " / main <=> sub";
//------------------------------------
#endif
//====================================
    //│
//====================================
#if (ADP_UART || MODE == MODE_BRIDGE) && TURBO == false
//※ターボでは無効
//・メ イ ン：任意
//・サ　　ブ：任意
//・ブリッジ：強制（GPIOクライアント用）
//------------------------------------
    //○【Serial2】を起動
    Serial2.begin(BPS, SERIAL_8N1, P2_RX, P2_TX);
    snprintf(msg, sizeof(msg), "   [OK] Serial #2 -> %d bps / Rx:%d Tx:%d <=> Client", BPS, P2_RX, P2_TX);
    msg2 = msg;
//------------------------------------
#endif
//====================================
    //│
//====================================
#if MODE == MODE_MAIN
//------------------------------------
    //○【MP3用】を起動
    MP3[0] = new SoftwareSerial(0); 
    MP3[0]->begin(MP3_BPS, SWSERIAL_8N1, MP3_RX1, MP3_TX1, false, 256);
    snprintf(msg, sizeof(msg),"   [OK] for MP3 #0 -> %d bps / Rx:%d Tx:%d", MP3_BPS, MP3_RX1, MP3_TX1);
    msgMP3 = msg;
//------------------------------------
#endif
//====================================
    //│
    //○メッセージ表示
    delay(1000);
    Log::prtln("<<通信デバイスの初期化>>");
    Log::prtln(" [UART]"  );
    Log::prtln(msgUSB);
    Log::prtln(msg0);
    Log::prtln(msg1);
    Log::prtln(msg2);
    if (msgMP3 != "") Log::prtln(msgMP3);
    Log::prtln("");
    //│
    //○有効性セット
    ENABLED = true;
  } /* START() */
} /* namespace devUART */