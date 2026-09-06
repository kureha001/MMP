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
// Ver 1.2.2 (2026/09/06) 
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
  #include "mmpConfig.h"
  const int MODE_MAIN   = 0        ; // メインモード
  const int MODE_SUB    = 1        ; // サブモード
  const int MODE_BRIDGE = 2        ; // ブリッジモード
  const int MODE_BOOT   = MODE_MAIN; // 起動時モード
  //│
  //□コンテクスト
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
    DeviceManager::INIT();
    ConnectionManager::INIT();
    CommandManager::INIT();
    //┴
  } /* initialize() */

  //─────────────────
  // オープニング
  //─────────────────
  void opening(){
    //┬
    //○開始メッセージを出力
    String strMode = "";
    if (MODE_BOOT == MODE_MAIN  ) strMode = "メイン"  ;
    if (MODE_BOOT == MODE_SUB   ) strMode = "サブ"    ;
    if (MODE_BOOT == MODE_BRIDGE) strMode = "ブリッジ";
    Serial.println("---------------------------");
    Serial.printf (" MMP Ver.%s\n"    , ctx.sysVer);
    Serial.printf (" 動作モード：%s\n", strMode   );
    Serial.println("---------------------------");
    //│
    //●ファンファーレを鳴らす
    if (MODE_BOOT == MODE_MAIN) {
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