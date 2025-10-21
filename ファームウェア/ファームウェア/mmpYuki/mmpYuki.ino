// filename : mmpYuki.ino
const char* g_version = "0502!";  // コンテクストのメンバ
//========================================================
//  MMP Firmware : Code Name -- Dete -- 
//-------------------------------------------------------- 
// ボード情報
// Waveshare RP2xx0 ZERO用
//   - Waveshare RP2040 Zero
//   - Waveshare RP2350 Zero
// Waveshare ESP32-S3-tiny用
//   - ESP32S3 Dev Module 
//-------------------------------------------------------- 
// Ver0.5.02 (2025/10/21)
//  port.h
//   - ESP32-S3-tiny対応：ボーレートGPIOを区別
//   - ESP32-S3-tiny対応：シリアル起動を区別
//  ｍPWM.h
//   - ESP32-S3-tiny対応：シリアル起動を区別
//   - INFO/CONNECTのバグを修正
//   - 値取得系コマンドのリトライを強化
//  ｍMP3.h
//   - ESP32-S3-tiny対応：シリアル起動を区別
//   - INFO/CONNECTのバグを修正
// Ver0.5.01 (2025/10/16)
//   module、m***
//     - 10進数統一
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