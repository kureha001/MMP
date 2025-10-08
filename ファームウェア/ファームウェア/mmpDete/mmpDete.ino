// filename : mmpDete.ino
const char* g_version = "0401!";  // コンテクストのメンバ
//========================================================
//  MMP Firmware : Code Name -- Dete -- 
//-------------------------------------------------------- 
// ボード情報：Waveshare RP2040 Zero
// ボード情報：Waveshare RP2350 Zero
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.01 (2025/10/08)
//  - 固有部分を各ファイルに移管
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
  .serial       = nullptr   , // クライアントのストリーム
  .pixels       = &g_pixels , // コマンド別のRGB-LED発行用
  .version      = g_version   // ファームウェア・バージョン
};

Perser g_router(g_ctx);                   // コンストラクタで依存性注入

//━━━━━━━━━━━━━━━━━
// セットアップ
//━━━━━━━━━━━━━━━━━
void setup(){

  // 基本機能
  InitPort();       // シリアルポート
  g_router.Init();  // パーサー

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