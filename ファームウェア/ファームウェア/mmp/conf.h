// filename : conf.h
//========================================================
// 各種設定
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once

//─────────────────
// ターゲット・ボード(UART設定で必要)
//─────────────────
#define BOARD_ESP32_S3_TINY
//#define BOARD_M5STAMP_S3
//#define BOARD_PICO2W

//─────────────────
// MMPタイプ
//─────────────────
const int MODE_MAIN   = 0; // メインモード
const int MODE_SUB    = 1; // サブモード
const int MODE_BRIDGE = 2; // ブリッジモード
const int MODE_BOOT   = MODE_MAIN; // 起動時モード

//─────────────────
// 経路アダプタ選択
//----------------------------------
//・必要：コメントアウト「しない」
//・不要：コメントアウト「する」
//─────────────────
#define ADP_UART // UART  ※本体＋サブの構成、ブリッジで必須
#define ADP_TCP  // TCP RAW
#define ADP_WAPI // WWB API
#define ADP_WSOC // WEB Socket
#define ADP_ESPN // ESP NOW
#define ADP_BLE  // BLE
//#define ADP_I2C  // i2c ※他IICデバイスが使えなくなる(本体で使用禁止)