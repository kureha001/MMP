// filename : mmpConfig.h
//========================================================
// 環境設定
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/06) 
//========================================================
#ifndef CONFIG_H
#define CONFIG_H
#pragma once

//─────────────────
// ターゲット・ボード
// [device/member/UART] PIN設定に仕様
// [device/member/IIC ] PIN設定に仕様
//─────────────────
#define BOARD_ESP32_S3_TINY 10
#define BOARD_M5STAMP_S3    11
#define BOARD_PICO2W        20
// 現在のターゲット ※上記から選択
#define BOARD BOARD_ESP32_S3_TINY

//─────────────────
// 動作モード
//─────────────────
#define MODE_MAIN   0 // メインモード
#define MODE_SUB    1 // サブモード
#define MODE_BRIDGE 2 // ブリッジモード
// 現在の動作モード ※上記から選択
#define MODE MODE_MAIN

//─────────────────
// 経路アダプタ選択
//----------------------------------
//・必要：コメントアウト「しない」
//・不要：コメントアウト「する」
//─────────────────
#define ADP_UART true // UART ※本体＋サブ、ブリッジでは自動適用
#define ADP_TCP  true // TCP RAW
#define ADP_WAPI true // WWB API
#define ADP_WSOC true // WEB Socket
#define ADP_ESPN true // ESP NOW
#define ADP_BLE  true // BLE
#define ADP_IIC  false // IIC ※メインモードでは使用禁止

#endif // CONFIG_H