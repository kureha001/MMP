// filename : mmp.ino
//========================================================
//  MMP Firmware
//--------------------------------------------------------
// ボード情報：Waveshare ESP32-S3-tiny用
//   - ESP32S3 Dev Module
//   - USB CDC ON Boot: Enabled
//   - Flash Size: 4MB (32Mb)
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/14) α版
//========================================================
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#include "cli.h"        // クライアント：共通ユーティリティ
#include "cliSerial.h"  // クライアント：シリアルポート
#include "cliNet.h"     // クライアント：ネットワーク
#include "parser.h"     // コマンド パーサー

const char* ino_VERSION = "V10a!";  // コンテクストのメンバ

//━━━━━━━━━━━━━━━━━
// グローバル変数
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // アクティブ判定
  //─────────────────
  bool ino_READY_SERIAL = false;
  bool ino_READY_NET    = false;


//━━━━━━━━━━━━━━━━━
// グローバル資源(定義)
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // NwoPixel(個数=1) コンテクストのメンバ
  //─────────────────
  #define NEOPIXEL_PIN 38   // Waveshare ESP32-S3-Tiny: WS2812 DIN=GPIO38
  Adafruit_NeoPixel g_PIXEL(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

  //─────────────────
  // コンテクスト
  // - 型定義：mod.h
  //─────────────────
  MmpContext ctx  = {
    .pixels   = &g_PIXEL,   // コマンド別のRGB-LED発行用
    .version  = ino_VERSION // ファームウェア・バージョン
  }; /* ctx */

  //─────────────────
  // パーサー：parser.hで定義・実装
  //─────────────────
  Parser  ino_ROUTER(ctx)       ; // 本体(依存性注入)
  Parser* g_PARSER = &ino_ROUTER; // 外部公開ポインタ


//━━━━━━━━━━━━━━━━━
// セットアップ
//━━━━━━━━━━━━━━━━━
void setup(){

  // クライアントのハンドルを作成
  ino_READY_SERIAL = InitSerial();  // シリアルポート
  ino_READY_NET    = InitNet();     // ネット

  // パーサーを初期化
  ino_ROUTER.Init();

  // 機能モジュールの初期化
  InitAnalog(ctx);
  InitPWM();
  InitMP3();

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

  // クライアント(シリアル)のハンドル
  if (ino_READY_SERIAL) {
    srvSerial::handle();
  } /* if */

  // クライアント(ネット)のハンドル
  if (ino_READY_NET) {
    srvHttp::handle();  // WebAPI用
    srvTcp::handle();   // TCP Bridge用
  } /* if */
} /* loop() */