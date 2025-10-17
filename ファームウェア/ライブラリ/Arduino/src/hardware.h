// hardware.h
#pragma once
#include <Arduino.h>

// ------------------------------------------------------------
// ボード依存の薄い定義（MmpClient.* が参照）
// できるだけ最小に保ち、各プロジェクトで必要なら上書き可
// ------------------------------------------------------------

// ========== UART ハードの選択 =================================
// Pico (Philhower core / Earle core) で GPIO UART を使うなら Serial1 が一般的。
// USB CDC を使う場合は Serial。既存の MmpClient.cpp は MMP_UART を参照します。
#ifndef MMP_UART
  #if defined(ARDUINO_ARCH_RP2040)
    // Earle core では Serial1 がハードUART (GPIO4=TX, GPIO5=RX が多い)
    #define MMP_UART Serial1
  #elif defined(ESP_PLATFORM)
    // ESP32 系の既定
    #define MMP_UART Serial1
  #else
    // その他環境の既定
    #define MMP_UART Serial1
  #endif
#endif


// ========== ボーレート候補（UART 自動接続で使用）=============
#ifndef MMP_BAUD_CANDIDATES_DEFINED
  static const uint32_t MMP_BAUD_CANDIDATES[] = {
    921600,57600,38400,19200,9600,4800,2400,300
  };
  static const size_t   MMP_BAUD_CANDIDATES_LEN =
      sizeof(MMP_BAUD_CANDIDATES)/sizeof(MMP_BAUD_CANDIDATES[0]);
  #define MMP_BAUD_CANDIDATES_DEFINED 1
#endif


// ========== TCP 機能の有効化（WiFi クライアントの有無で自動）==
#if !defined(MMP_ENABLE_TCP)
  #if __has_include(<WiFi.h>)
    #define MMP_ENABLE_TCP 1
  #else
    #define MMP_ENABLE_TCP 0
  #endif
#endif