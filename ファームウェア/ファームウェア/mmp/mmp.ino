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
  #include "mmpCtx.h"    // MMPコンテクスト
  #include "iniSerial.h" // シリアルポート資源の初期化
  #include "iniNet.h"    // ネットワーク資源の初期化
  #include "adp.h"       // 通信アダプタ共通
  #include "parser.h"    // コマンド パーサー
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ＭＭＰシステムバージョン
  //─────────────────
  const char* INO_VERSION = "V10a!";  // コンテクストのメンバ

  //─────────────────
  // NwoPixel ※初期化・パーサで利用
  //─────────────────
  #define NEOPIXEL_PIN 38 // Waveshare ESP32-S3-Tiny: WS2812 DIN=GPIO38
  Adafruit_NeoPixel INO_PIXEL(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800); // 1個

  //─────────────────
  // コンテクスト実体化(型定：mmpCtx.h)
  //─────────────────
  MmpContext ctx  = {
    .version  = INO_VERSION // ファームウェア・バージョン
  }; /* ctx */

  //─────────────────
  // コマンドパーサ参照(定義：parser.h)
  //─────────────────
  Parser  ino_ROUTER(ctx)         ; // 本体(依存性注入) ※コンストラクタ
  Parser* INO_PARSER = &ino_ROUTER; // 外部公開ポインタ


//━━━━━━━━━━━━━━━━━
// セットアップ
//━━━━━━━━━━━━━━━━━
void setup(){

  // 通信アダプタを初期化
  InitSerial(); // シリアル系
  InitNet()   ; // ネットワーク系

  // パーサーを初期化
  ino_ROUTER.Init(); // 依存注入済みに対し初期化処理を実行

  // 機能モジュールの初期化
  InitAnalog(ctx); // アナログ入力
  InitPWM();       // PWM出力
  InitMP3();       // MP3プレイヤー

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
  httpAdmin::handle(); // 管理画面

} /* loop() */