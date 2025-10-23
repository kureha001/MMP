// filename : mmpNew.ino
const char* g_version = "V600!";  // コンテクストのメンバ
//========================================================
//  MMP Firmware : Code Name -- Yuki2 -- 
//--------------------------------------------------------
// ボード情報：Waveshare ESP32-S3-tiny用
//   - ESP32S3 Dev Module 
//--------------------------------------------------------
// Ver0.6.00 (2025/xx/xx)
//========================================================
#include <Arduino.h>
#include <Wire.h>

#include "cliSerial.h"  // クライアント：シリアルポート 
#include "cliNet.h"     // クライアント：ネットワーク
#include "fnPerser.h"   // パーサー

bool g_CONNECTED_SERIAL = false;
bool g_CONNECTED_NET    = false;

//━━━━━━━━━━━━━━━━━
// コンテクスト関連のグローバル定義（fnPerser.h の extern と整合）
//━━━━━━━━━━━━━━━━━

// MmpContext が参照する「現在の出力先ストリーム」ポインタ
//  二重ポインタの指し先として NULL 禁止（初期はUSB Serial）
Stream* g_reply = &Serial;

// fnPerser.h は extern MmpContext ctx; extern Perser* g_perser; を要求
MmpContext ctx = {
  .clientID     = 0         , // 現在のクライアントID: 0=USB, 1=UART0, 2=TCP, 3=HTTP...
  .clientID_max = -1        , // 既知の最大クライアントID
  .reply        = &g_reply  , // 現在の“返信出力先”Stream（二重ポインタ）
  .pixels       = &g_pixels , // コマンド別のRGB-LED発行用（cliSerial.h のグローバル）
  .version      = g_version   // ファームウェア・バージョン
};

// パーサー本体（依存性注入）と、その外部公開ポインタ
Perser  g_router(ctx);
Perser* g_perser = &g_router;

//━━━━━━━━━━━━━━━━━
// セットアップ
//━━━━━━━━━━━━━━━━━
void setup(){

  // クライアントのハンドルを作成
  g_CONNECTED_SERIAL = InitSerial();  // シリアルポート
  g_CONNECTED_NET    = InitNet();     // ネット

  // パーサーを初期化（DIGのみ登録：fnPerser.h参照）
  g_router.Init();

  // まだ未同梱のモジュール初期化はビルドエラー回避のためコメントアウト
  InitAnalog(ctx);
  InitPWM();
  InitMP3();

  Serial.println(String("==================================="));
  Serial.println(String("MMP Firmware version : ") + String(g_version));
  Serial.println(String("==================================="));
}

//━━━━━━━━━━━━━━━━━
// ループ
//━━━━━━━━━━━━━━━━━
void loop(){

  // クライアント(シリアル)のハンドル
  if (g_CONNECTED_SERIAL) {
    g_router.pollAll();
    delay(1);
  }

  // クライアント(ネット)のハンドル
  if (g_CONNECTED_NET) {
    srvHttp::handle(); // WebAPI
    delay(1);
    srvTcp::handle();  // TCP
    delay(1);
  }
}