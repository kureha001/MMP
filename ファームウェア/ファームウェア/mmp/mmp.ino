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
// Ver 1.2.3 (2026/09/07) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <Wire.h> // setup()
  //┴
//┴
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
  //□部門
  #include "Device.h"     // 通信デバイス
  #include "Connection.h" // クライアント接続
  #include "Command.h"    // コマンド実行
//┴┴

//━━━━━━━━━━━━━━━━━
// セットアップ部品
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 活動に必要な資源を初期化
  //─────────────────
  void initialize(){
    //┬
    //●通信デバイス部門に初期化を依頼
    //●接続クライアント部門に初期化を依頼
    //●コマンド実行部門に初期化を依頼
    DeviceManager    ::INIT();
    ConnectionManager::INIT();
    CommandManager   ::INIT();
    //┴
  } /* initialize() */

  //─────────────────
  // オープニング
  //─────────────────
  void opening(){
    //┬
    //○動作モード名を取得
    String strMode = "";
    if (MODE == MODE_MAIN  ) strMode = "Main"  ;
    if (MODE == MODE_SUB   ) strMode = "Sub"    ;
    if (MODE == MODE_BRIDGE) strMode = "Bridge";
    //│
    //○開始メッセージを出力
    Serial.println("------------------------");
    Serial.printf (" MMP [%s Mode]  %s\n", strMode, ctx.sysVer);
    Serial.println("------------------------");
    //│
    //●ファンファーレを鳴らす
    if (MODE == MODE_MAIN) {
      ctx.cmdPath = "MP3/TRACK/PLAY_ROOT:1:1!";
      CommandManager::RunCommand();
    }
    //┴
  } /* opening() */

//========================================================
// セットアップ
//========================================================
void setup(){
  //┬
  //●資源を初期化
  //●オープニングを表示
  initialize();
  opening();
  //┴
} /* setup() */

//========================================================
// ポーリング
//========================================================
void loop(){
  //┬
  //●クライアント接続部門に通常活動を指示
  ConnectionManager::WORK();
  //┴
} /* loop() */