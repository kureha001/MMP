// filename : connection/_index_.h
//========================================================
// クライアント接続部門
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#ifndef CONN_H
#define CONN_H
#pragma once

//┬
//□┐情報
  //□経路ID
  inline constexpr int ADP_ID_UART = 0;
  inline constexpr int ADP_ID_TCP  = 1;
  inline constexpr int ADP_ID_WAPI = 2;
  inline constexpr int ADP_ID_WSOC = 3;
  inline constexpr int ADP_ID_BLE  = 4;
  inline constexpr int ADP_ID_ESPN = 5;
  inline constexpr int ADP_ID_IIC  = 6;
  //│
  //□ストリーム受信型の接続管理
  struct SS_SLOT_TYPE {      // 接続管理スロット
    bool    used   = false ; // スロット有効性
    String  rx     = ""    ; // 受信バッファ
    bool    isOver = false ; // 受信バッファ容量超過判定
  };
  inline constexpr int SS_RX_SIZE = 128; // 受信バッファ容量
//┴┴
//┬
//□┐クライアント接続部門
  //□┐統括マネージャ
    //□共通係
    #include "common/_index_.h"
    //│
    //□担当マネージャ：動作モード
    #include "mode/_index_.h"
    //│
    //□担当：経路アダプタ
    #include "member/_index_.h" 
//┴┴┴

#endif // CONN_H