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
// - WebSockets        by Markus Sattler
// - EspSoftwareSerial by Peter Lerup, Dirk Kaar
//--------------------------------------------------------
// Ver 1.4.0 (2026/10/01)
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
  //□システム構築
  #include "mmpMake.h"
  //│
  //□全体共通
  #include "mmp.h"
  //│
  //■コンテクスト（実体）
  MmpContext ctx;
//│┴
//│
//□┐組織
  //□通信設備部門
  #include "Dep_Network/manager.h"
  //│
  //□製造部門
  #include "Dep_Product/manager.h"
  //│
  //□接客部門
  #include "Dep_Contact/manager.h"
//┴┴

//━━━━━━━━━━━━━━━━━
// 始業開始のパーツ
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 2-1.全部門に始業を指示する
  //─────────────────
  void initialize(){
    //┬
    //●通信設備部門に始業を指示する
    //●製造部門に始業を指示する
    //●接客部門に始業を指示する
    DepNetwork::INIT();
    DepProduct::INIT();
    DepContact::INIT();
    //┴
  } /* initialize() */

  //─────────────────
  // 2-2.始業を宣言する
  //─────────────────
  void opening(){
    //┬
    //○動作モード名を求める
    String strMode = "";
    if (MODE == MODE_MAIN  ) strMode = "MAIN"  ;
    if (MODE == MODE_SUB   ) strMode = "SUB"    ;
    if (MODE == MODE_BRIDGE) strMode = "BRIDGE";
    //│
    //○開始メッセージを出力する
    Log::prtln("-----------------------------");
    char msg[128];
    snprintf(msg, sizeof(msg), " MMP %s [MODE: %s]", ctx.sysVer, strMode);
    Log::prtln(String(msg));
    snprintf(msg, sizeof(msg), " Log Output:[%s]", (Log::ENABLE ? "ON" : "OFF"));
    Log::prtln(String(msg));
    Log::prtln("-----------------------------");
    //│
    //●製造部門がファンファーレを鳴らす
    if (MODE == MODE_MAIN) {
      ctx.base.CmdPath = "MP3/PLAY:1:1!";
      DepProduct::HANDLE();
    }
    //┴
  } /* opening() */

//========================================================
// ２．活動開始を準備する
//========================================================
void setup(){
  //┬
  //●資源を初期化する
  //●オープニングを表示する
  initialize();
  opening();
  //┴
} /* setup() */

//========================================================
// ３．業務を遂行し続ける
//========================================================
void loop(){
  //┬
  //●接客部門に通常業務の遂行を指示する
  DepContact::HANDLE();
  //┴
} /* loop() */