// filename : mmp.ino
//========================================================
//  MMP Firmware
//--------------------------------------------------------
// ボード情報：Waveshare ESP32-S3-tiny用
//  - ESP32S3 Dev Module
//  - USB CDC ON Boot: Enabled
//  - Flash Size: 4MB (32Mb)
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/14) α版 
//・[ＷＥＢページ：管理画面]サービスを追加
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <Wire.h>
  #include <Adafruit_NeoPixel.h>
  //│
  //■ＭＭＰシステム
  #include "mmpCtx.h"    // 全体仕様
  #include "parser.h"    // INO_PARSERを利用
  #include "ini.h"       // setup()
  #include "adp.h"       // loop()
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト実体化(型定：mmpCtx.h)
  //─────────────────
  const char* INO_VERSION = "V10a!";  // コンテクストのメンバ
  MmpContext ctx  = {.version = INO_VERSION};

  //─────────────────
  // NwoPixel ※パーサ実体化よりも先に記述
  //─────────────────
  #define NEOPIXEL_PIN 38 // Waveshare ESP32-S3-Tiny: WS2812 DIN=GPIO38
  Adafruit_NeoPixel INO_PIXEL(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800); // 1個

  //─────────────────
  // コマンドパーサ参照(定義：parser.h)
  //─────────────────
  Parser  ino_ROUTER(ctx)         ; // 本体(依存性注入)
  Parser* INO_PARSER = &ino_ROUTER; // 外部公開ポインタ

//━━━━━━━━━━━━━━━━━
// セットアップ
//━━━━━━━━━━━━━━━━━
void setup(){

  // 通信アダプタを初期化
  iniSerial::start(); // シリアル系
  iniNet::start();    // ネットワーク系  

  // パーサーを初期化
  ino_ROUTER.Init(); // 依存注入済みに対し初期化処理を実行

  // 機能モジュールの初期化
  //InitAnalog(ctx.accIDS); // アナログ入力
  //InitPWM(ctx.accIDS);    // PWM出力
  //InitMP3();              // MP3プレイヤー

  // 開始メッセージ出力
  Serial.println("---------------------------");
  Serial.print  ("Running... MMP Ver");
  Serial.println(String(ctx.version));
  Serial.println("---------------------------");
} /* setup() */

//━━━━━━━━━━━━━━━━━
// ループ
//━━━━━━━━━━━━━━━━━
void loop(){

  //○通信アダプタ
  adpSerial::handle(); // シリアル
  adpTcp   ::handle(); // TCPブリッジ
  adpHttp  ::handle(); // WebAPI

  //○ＷＥＢページ
  adpAdmin::handle(); // 管理画面

} /* loop() */