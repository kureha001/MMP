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
// Ver 1.2.2 (2026/09/03) 
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <Wire.h> // setup()
  //│
  //■ＭＭＰシステム(マネージャ群)
  #include "dev.h"  // 通信デバイス
  #include "adp.h"  // 経路アダプタ
  #include "cmd.h"  // コマンド
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// セットアップ部品
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 資源の初期化
  //─────────────────
  void initialize(){
    //┬
    //●デバイス・マネージャに初期化を依頼
    DeviceManager::INIT();
    //│
    //●アダプタ・マネージャに初期化を依頼
    AdapterManager::INIT();
    //│
  #if defined(MMP_TYPE_MAIN)
    //●コマンド・マネージャに初期化を依頼
    CommandManager::INIT();
    //┴
  #endif
  } /* initialize() */

  //─────────────────
  // オープニング
  //─────────────────
  void opening(){
    //┬
  #if defined(MMP_TYPE_MAIN)
    //●ファンファーレ
    ctx.cmdPath = "MP3/TRACK/PLAY_ROOT:1:1!";
    CommandManager::RunCommand();
  #endif
    //│
    //○開始メッセージ出力
    Serial.println("---------------------------");
    Serial.print  (String(ctx.sysName));
    Serial.println(String(" Ver.") + String(ctx.sysVer ));
    Serial.println("---------------------------");
    //┴
  } /* opening() */

//========================================================
// セットアップ
//========================================================
void setup(){
  //┬
  //○資源を初期化
  initialize();
  //│
  //●オープニングを表示
  opening();
  //┴
} /* setup() */

//========================================================
// ポーリング
//========================================================
void loop(){
  //┬
  //●アダプタ・マネージャにハンドルのキックを依頼
  AdapterManager::HANDLE();
  //┴
} /* loop() */