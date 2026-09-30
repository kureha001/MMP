// filename : mmpMake.h
//========================================================
// システム構築
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/27)
//========================================================
#pragma once

//========================================================
// ベース（編集禁止）
//========================================================
  //─────────────────
  // 経路アダプタ
  //─────────────────
    #define ADP_UART true  // ブリッジでは必須
    #define ADP_UDP  false
    #define ADP_TCP  false
    #define ADP_WSOC false
    #define ADP_HTTP false
    #define ADP_ESPN false
    #define ADP_BLE  false
    #define ADP_IIC  false // 現在ブリッジ未対応

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
  #define MODE MODE_BRIDGE

  //②UART高速モード
  // USB(CDC)の単一スロット＆パケット処理
  // [Dep_Connect/adapter/_index_.h]にて分岐
  //・メ イ ン：サブ連携が不可
  //・サ　　ブ：GPIOのUARTは使用不可(USB-CDC,メイン連携は可)
  //・ブリッジ：GPIOのUARTは使用不可(USB-CDCは可)
  #define TURBO true

  //③モード別プリセット
  //(1)メイン用
  #if   (MODE == MODE_MAIN)
    #define ADP_UART true 
    #define ADP_UDP  true
    #define ADP_TCP  true
    #define ADP_WSOC true
    #define ADP_HTTP true
    #define ADP_ESPN true
    #define ADP_BLE  true
    #define ADP_IIC  false // Wire1を使用|WireはPWMで利用
  //(2)サブ用
  #elif (MODE == MODE_SUB)
    #define ADP_UART true 
    #define ADP_UDP  true
    #define ADP_TCP  true
    #define ADP_WSOC true
    #define ADP_HTTP true
    #define ADP_ESPN true
    #define ADP_BLE  true
    #define ADP_IIC  false // Wire1を使用|Wireは空き
  //(3)ブリッジ用
  #elif (MODE == MODE_BRIDGE)
    #define ADP_UDP  true
    #define ADP_TCP  true
    #define ADP_WSOC true
    #define ADP_HTTP true
    #define ADP_ESPN true
    #define ADP_BLE  true
  #endif