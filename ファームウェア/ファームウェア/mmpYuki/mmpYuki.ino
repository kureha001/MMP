// filename : mmpYuki.ino
const char* g_version = "0500!";  // コンテクストのメンバ
//========================================================
//  MMP Firmware : Code Name -- Dete -- 
//-------------------------------------------------------- 
// ボード情報：Waveshare RP2040 Zero
// ボード情報：Waveshare RP2350 Zero
//-------------------------------------------------------- 
// 変更履歴: Ver0.5.00 (2025/10/13)
// Scketch： Ver0.4から変更なし
// port   ： Ver0.4から変更なし
// perser ： クライアントからのリクエスト条件をmoudule.hへ移管
// module ： クライアントからのリクエスト条件をperse.hより移管
//           コマンド応答ヘルパーを一新
// mINF   ： WEB-API書式対応、コマンド応答を一新
// mANA   ： WEB-API書式対応、コマンド応答を一新
// mDIG   ： WEB-API書式対応、コマンド応答を一新
// mPWM   ： WEB-API書式対応、コマンド応答を一新
// mI2C   ： WEB-API書式対応、コマンド応答を一新
//========================================================
#include <Arduino.h>
#include <Wire.h>

#include "port.h"   // シリアルポートを開く 
#include "perser.h" // パーサーのフロント

//━━━━━━━━━━━━━━━━━
// コンテクストのマッピング
//━━━━━━━━━━━━━━━━━
MmpContext g_ctx = {
  .clientID     = 0         , // カレントのクライアントID: 0=USB, 1=UART0
  .clientID_max = -1        , // クライアントIDの上限(0～)
  .serial       = &g_serial , // クライアントのストリーム
  .pixels       = &g_pixels , // コマンド別のRGB-LED発行用
  .version      = g_version   // ファームウェア・バージョン
};
Perser g_router(g_ctx);       // コンストラクタで依存性注入

//━━━━━━━━━━━━━━━━━
// セットアップ
//━━━━━━━━━━━━━━━━━
void setup(){

  // 基本機能
  InitPort();         // シリアルポート
  g_router.Init();    // パーサー ※コンストラクタ依存性注入済

  // モジュール機能
  InitAnalog(g_ctx);  // アナログ入力
  InitPWM();          // ＰＷＭ出力
  InitMP3();          // ＭＰ３
}

//━━━━━━━━━━━━━━━━━
// ループ
//━━━━━━━━━━━━━━━━━
void loop(){
  g_router.pollAll(); // パーサーのフロント
}