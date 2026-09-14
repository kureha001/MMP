// filename : mmpConfig.h
//========================================================
// 環境設定
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/14)
// ・ベース部と選択部を分離
// ・モード別にカテゴリ化
// ・ターボモードを追加
//========================================================
#ifndef CONFIG_H
#define CONFIG_H
#pragma once

//========================================================
// ベース（編集禁止）
//========================================================
  //─────────────────
  // ターゲット・ボード
  // [device/member/UART] PIN設定に仕様
  // [device/member/IIC ] PIN設定に仕様
  //─────────────────
  #define BOARD_ESP32_S3_TINY 10
  #define BOARD_M5STAMP_S3    11
  #define BOARD_PICO2W        20

  //─────────────────
  // 動作モード
  //─────────────────
  #define MODE_MAIN    0 // メインモード
  #define MODE_SUB     1 // サブモード
  #define MODE_BRIDGE  2 // ブリッジモード

  //─────────────────
  // 経路アダプタ
  //─────────────────
    #define ADP_TCP  false
    #define ADP_HTTP false
    #define ADP_WSOC false
    #define ADP_ESPN false
    #define ADP_BLE  false
    #define ADP_IIC  false

//========================================================
// 用途に応じて設定
//========================================================
  //─────────────────
  // ターゲット・ボード
  //─────────────────
  #define BOARD BOARD_ESP32_S3_TINY

  //─────────────────
  // 動作モード
  //─────────────────
  #define MODE MODE_MAIN

  //─────────────────
  // 高速モード
  //─────────────────
  #define TURBO true

  //─────────────────
  // モード別に経路アダプタ選択
  // ※項目追加禁止
  //─────────────────
  //□メインモード：IIC以外は選択可能
  #if   (MODE == MODE_MAIN)
    #define ADP_TCP  false
    #define ADP_HTTP false
    #define ADP_WSOC false
    #define ADP_ESPN false
    #define ADP_BLE  false

  //□サブモード：すべて選択可能
  #elif (MODE == MODE_SUB)
    #define ADP_TCP  true
    #define ADP_HTTP true
    #define ADP_WSOC true
    #define ADP_ESPN true
    #define ADP_BLE  true
    #define ADP_IIC  false

  //□ブリッジモード：高速無線のみ選択可能
  #elif (MODE == MODE_BRIDGE)
    #define ADP_TCP  true
    #define ADP_WSOC true
    #define ADP_ESPN true
    #define ADP_BLE  true

  #endif

  #endif // CONFIG_H