// filename : mmpNew.ino
const char* g_version = "V060!";  // コンテクストのメンバ
//========================================================
//  MMP Firmware : Code Name -- Yuki2 -- 
//--------------------------------------------------------
// ボード情報：Waveshare ESP32-S3-tiny用
//   - ESP32S3 Dev Module 
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/11) 初版
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
Perser  g_ROUTER(ctx);
Perser* g_perser = &g_ROUTER;

//━━━━━━━━━━━━━━━━━
// セットアップ
//━━━━━━━━━━━━━━━━━
void setup(){

  // クライアントのハンドルを作成
  g_CONNECTED_SERIAL = InitSerial();  // シリアルポート
  g_CONNECTED_NET    = InitNet();     // ネット

  // パーサーを初期化（DIGのみ登録：fnPerser.h参照）
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
}

//━━━━━━━━━━━━━━━━━
// ループ
//━━━━━━━━━━━━━━━━━
void loop(){

  // クライアント(シリアル)のハンドル
  if (g_CONNECTED_SERIAL) {
    srvSerial::handle();
  }

  // クライアント(ネット)のハンドル
  if (g_CONNECTED_NET) {
    srvHttp::handle();  // WebAPI
    srvTcp::handle();   // TCP
  }
}