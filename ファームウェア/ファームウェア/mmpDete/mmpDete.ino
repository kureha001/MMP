// filename : mmpDete.ino
const char* g_Version = "0401!";
//========================================================
//  MMP Firmware : Code Name -- Dete -- 
//-------------------------------------------------------- 
// ボード情報：Waveshare RP2040 Zero
// ボード情報：Waveshare RP2350 Zero
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.01 (2025/09/29)
//  - デジタル入出力モジュール：PORコマンド復活
//  - PWMモジュール：角度指定用コマンド新設
//  - PWMモジュール：ローテーションサー用コマンド新設
//  - シリアルポート開始を別ファイル(openPort.h)に分離
//  - モジュール固有部分をモジュールファイルに移管
//========================================================
#include <Arduino.h>
#include <Wire.h>

// 基本機能
#include "openPort.h" // シリアルポートを開く 
#include "perser.h"   // パーサーのフロント ※module.hを包括 

// 個別モジュール機能
#include "mINF.h"
#include "mANA.h"
#include "mDIG.h"
#include "mPWM.h"
#include "mI2C.h"
#include "mMP3.h"

Stream*     g_serial        = nullptr;
int         g_currentClient = 0; // 0=USB, 1=UART0

//━━━━━━━━━━━━━━━━━
// コンテクストのマッピング
//━━━━━━━━━━━━━━━━━
MmpContext g_ctx = {
  .currentClient    = &g_currentClient  , // カレントのストリームID: 0=USB, 1=UART0
  .serial           = &g_serial         , // ストリームのハンドル
  .pixels           = &g_pixels         , // コマンド別のRGB-LED発行用
  .versionStr       = g_Version           // ファームウェア・バージョン
};
Perser g_router(g_ctx);                   // コンストラクタで依存性注入

//━━━━━━━━━━━━━━━━━
// セットアップ
//━━━━━━━━━━━━━━━━━
void setup(){

  // シリアルポートを開く
  InitPort();

  // モジュール初期化処理
  InitAnalog(); // アナログ入力
  InitPWM();    // ＰＷＭ出力
  InitMP3();    // ＭＰ３

  // モジュール一覧を構築
  g_router.addModule(new ModuleInfo   (g_ctx, LED_PALETTE_INFO    ));
  g_router.addModule(new ModuleAnalog (g_ctx, LED_PALETTE_ANALOG  ));
  g_router.addModule(new ModuleDigital(g_ctx, LED_PALETTE_DIGITAL ));
  g_router.addModule(new ModulePwm    (g_ctx, LED_PALETTE_PWM     ));
  g_router.addModule(new ModuleI2C    (g_ctx, LED_PALETTE_I2C     ));
  g_router.addModule(new ModuleDF     (g_ctx, LED_PALETTE_MP3     ));

  // ストリーム(クライアント)一覧を構築(オブジェクト,名称,クライアントID)
  g_router.addSource(&Serial,  "USB",   0);
  g_router.addSource(&Serial1, "UART0", 1);
}

//━━━━━━━━━━━━━━━━━
// ループ
//━━━━━━━━━━━━━━━━━
void loop(){
  g_router.pollAll(); // パーサーのフロント
}