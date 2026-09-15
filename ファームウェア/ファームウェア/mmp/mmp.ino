// filename : mmp.ino
//========================================================
//  MMP Firmware
//--------------------------------------------------------
// - ボード情報      : Waveshare ESP32-S3-tiny用
// - ボート          : ESP32S3 Dev Module
// - USB CDC ON Boot : Enabled
// - Flash Size      : 4MB (32Mb)
// - Patition Scheme : Huge APP(3MB No OTA/1MB SPIFFS)
//--------------------------------------------------------
// 追加ライブラリ：
// - WebSockets by Markus Sattler
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/15)
// ・UARTポートの見直し 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <Wire.h> // setup()
  //┴
//┴

//========================================================
// １．活動資源を用意する
//========================================================
//┬
//□┐情報
  //□環境設定
  //□コンテクスト（実体化）
  #include "mmpConfig.h"
  #include "mmpContext.h"
  MmpContext ctx;
//│┴
//│
//□┐組織
  //□通信部門
  //□コマンド部門
  //□接続部門
  #include "Dep_Network/manager.h"
  #include "Dep_Command/manager.h"
  #include "Dep_Connect/manager.h"
//┴┴

//━━━━━━━━━━━━━━━━━
// 始業開始のパーツ
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 2-1.全部門に始業を指示する
  //─────────────────
  void initialize(){
    //┬
    //●通信部門に始業指示
    //●コマンド部門に始業指示
    //●接続部門に始業指示
    DepNetwork::INIT();
    DepCommand::INIT();
    DepConnect::INIT();
    //┴
  } /* initialize() */

  //─────────────────
  // 2-2.始業を宣言する
  //─────────────────
  void opening(){
    //┬
    //○動作モード名を取得
    String strMode = "";
    if (MODE == MODE_MAIN  ) strMode = "MAIN"  ;
    if (MODE == MODE_SUB   ) strMode = "SUB"    ;
    if (MODE == MODE_BRIDGE) strMode = "BRIDGE";
    //│
    //○開始メッセージを出力
    Log::prtln("-----------------------------");
    char msg[128];
    snprintf(msg, sizeof(msg), " MMP %s [MODE: %s]", ctx.sysVer, strMode);
    Log::prtln(String(msg));
    Log::prtln("-----------------------------");
    //│
    //●ファンファーレを鳴らす
    if (MODE == MODE_MAIN) {
      ctx.cmdPath = "MP3/PLAY:1:1!";
      DepCommand::RunCommand();
    }
    //┴
  } /* opening() */

//========================================================
// ２．活動開始を準備する
//========================================================
void setup(){
  //┬
  //●2-1.資源を初期化
  //●2-2.オープニングを表示
  initialize();
  opening();
  //┴
} /* setup() */

//========================================================
// ３．業務を遂行し続ける
//========================================================
void loop(){
  //┬
  //●接続部門に通常活動を指示
  DepConnect::WORK();
  //┴
} /* loop() */