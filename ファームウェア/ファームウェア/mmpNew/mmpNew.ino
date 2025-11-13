// filename : mmpNew.ino
const char* g_version = "V060!";  // コンテクストのメンバ
//========================================================
//  MMP Firmware
//--------------------------------------------------------
// ボード情報：Waveshare ESP32-S3-tiny用
//   - ESP32S3 Dev Module 
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/11) 初版
//========================================================
#include <Wire.h>
#include "cli.h"        // クライアント：共通ユーティリティ
#include "cliSerial.h"  // クライアント：シリアルポート
#include "cliNet.h"     // クライアント：ネットワーク
#include "fnPerser.h"   // コマンド パーサー

//━━━━━━━━━━━━━━━━━
// グローバル変数
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // アクティブ判定
  //─────────────────
  bool g_READY_SERIAL = false;
  bool g_READY_NET    = false;


//━━━━━━━━━━━━━━━━━
// グローバル資源(定義)
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト
  //─────────────────
  Stream* g_reply = &Serial;
  MmpContext ctx  = {
    .vStream  = &g_reply  , // 現在の“返信出力先”Stream（二重ポインタ）
    .clientID = -1        , // クライアントID
    .pixels   = &g_pixels , // コマンド別のRGB-LED発行用（cliSerial.h のグローバル）
    .version  = g_version   // ファームウェア・バージョン
  }; /* ctx */

  //─────────────────
  // パーサー
  //─────────────────
  Perser  g_ROUTER(ctx)       ; // 本体(依存性注入)
  Perser* g_PERSER = &g_ROUTER; // 外部公開ポインタ


//━━━━━━━━━━━━━━━━━
// セットアップ
//━━━━━━━━━━━━━━━━━
void setup(){

  // クライアントのハンドルを作成
  g_READY_SERIAL = InitSerial();  // シリアルポート
  g_READY_NET    = InitNet();     // ネット

  // パーサーを初期化
  g_ROUTER.Init();

  // 機能モジュールの初期化
  InitAnalog(ctx);
  InitPWM();
  InitMP3();

  // 開始メッセージ出力
  Serial.println("---------------------------");
  Serial.print  ("Running... MMP Ver");
  Serial.println(String(g_version));
  Serial.println("---------------------------");
} /* setup() */

//━━━━━━━━━━━━━━━━━
// ループ
//━━━━━━━━━━━━━━━━━
void loop(){

  // クライアント(シリアル)のハンドル
  if (g_READY_SERIAL) {
    srvSerial::handle();
  } /* if */

  // クライアント(ネット)のハンドル
  if (g_READY_NET) {
    srvHttp::handle();  // WebAPI用
    srvTcp::handle();   // TCP Bridge用
  } /* if */
} /* loop() */