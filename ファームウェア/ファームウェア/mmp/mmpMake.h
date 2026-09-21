// filename : mmpMake.h
//========================================================
// システム構築
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/21)
//========================================================
#ifndef CONFIG_H
#define CONFIG_H
#pragma once

//========================================================
// ベース（編集禁止）
//========================================================
  //─────────────────
  // 経路アダプタ
  //─────────────────
    #define ADP_UART true
    #define ADP_TCP  false
    #define ADP_HTTP false
    #define ADP_WSOC false
    #define ADP_ESPN false
    #define ADP_BLE  false
    #define ADP_IIC  false //※メイン・ブリッジは不可

  //─────────────────
  // 動作モード
  //─────────────────
  #define MODE_MAIN    0 // メイン
  #define MODE_SUB     1 // サブ
  #define MODE_BRIDGE  2 // ブリッジ

//========================================================
// コンパイルオプション
//========================================================
  //①動作モード
  #define MODE MODE_SUB

  //②UART高速モード
  // USB(CDC）の単一スロット＆パケット処理
  // [Dep_Connect/adapter/_index_.h]にて分岐
  //・メ イ ン：サブ連携が不可
  //・サ　　ブ：GPIO UARTの使用が不可(USB-CDC,メイン連携は可)
  //・ブリッジ：GPIO UARTの使用が不可(USB-CDCは可)
  #define TURBO false

  //③モード別プリセット
  //(1)メイン用
  #if   (MODE == MODE_MAIN)
    #define ADP_UART true 
    #define ADP_TCP  true
    #define ADP_HTTP true
    #define ADP_WSOC true
    #define ADP_ESPN true
    #define ADP_BLE  true
  //(2)サブ用
  #elif (MODE == MODE_SUB)
    #define ADP_UART false 
    #define ADP_TCP  false
    #define ADP_HTTP false
    #define ADP_WSOC false
    #define ADP_ESPN true
    #define ADP_BLE  false
    #define ADP_IIC  false
  //(3)ブリッジ用
  #elif (MODE == MODE_BRIDGE)
    #define ADP_TCP  false
    #define ADP_HTTP false
    #define ADP_WSOC false
    #define ADP_ESPN true
    #define ADP_BLE  false
  #endif

#endif // CONFIG_H