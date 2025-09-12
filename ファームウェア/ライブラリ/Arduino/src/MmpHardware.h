#pragma once
#include <Arduino.h>

// 稼働実績
// Arduino UnoR4　：　標準設定ＯＫ
// RP2040/2350    ：　標準設定ＯＫ

// ===== UARTの選択 =====
#ifndef MMP_UART
  #define MMP_UART Serial1
#endif

#ifndef MMP_UART_TX_PIN
  #define MMP_UART_TX_PIN 0   // GPIO0 → TX
#endif

#ifndef MMP_UART_RX_PIN
  #define MMP_UART_RX_PIN 1   // GPIO1 → RX
#endif

// ★ （任意）setRX/setTX があるコアでのみ使いたい場合は ON にする
// #define MMP_UART_USE_SETRXTX 1

// ★ （任意）pico-sdk の gpio_set_function でピン多重化を行いたい場合は ON
//    ※ コアが pico-sdk を同梱している前提（非 Mbed）
/*
#define MMP_UART_USE_PICO_GPIOFUNC 1
#if defined(MMP_UART_USE_PICO_GPIOFUNC)
extern "C" {
  #include "hardware/gpio.h"
}
#endif
*/

// ===== ボーレート候補（MMPプリセット順）=====
static const uint32_t MMP_BAUD_CANDIDATES[] = {
  115200,  // BAUD_00（物理スイッチ実装あり）
  9600,    // BAUD_10（物理スイッチ実装あり）
  230400,  // BAUD_01
  921600,  // BAUD_11
  300      // 古いマイコン
};
static const size_t MMP_BAUD_CANDIDATES_LEN =
  sizeof(MMP_BAUD_CANDIDATES) / sizeof(MMP_BAUD_CANDIDATES[0]);
