// filename : connection/_index_.h
//========================================================
// クライアント接続部門
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once
//┬
//□┐クライアント接続部門（保有資源）
  //│
  //□┐作業標準
    //│
    //□経路ID（RID : Route ID）
    inline constexpr int ADP_ID_UART = 0;
    inline constexpr int ADP_ID_TCP  = 1;
    inline constexpr int ADP_ID_WAPI = 2;
    inline constexpr int ADP_ID_WSOC = 3;
    inline constexpr int ADP_ID_BLE  = 4;
    inline constexpr int ADP_ID_ESPN = 5;
    inline constexpr int ADP_ID_IIC  = 6;
    //│
    //□┐専門処理：ストリーム受信
      //│
      //□基本情報
      static const int SS_RX_SIZE = 128 ; // 受信バッファ容量
      //│
      //□接続情報（スロット構造体）
      struct SS_SLOT_TYPE {
        bool    used   = false ; // スロット有効性
        String  rx     = ""    ; // 受信バッファ
        bool    isOver = false ; // 容量超過フラグ
      };
      //┴
  //│
  //□┐組織
    //│
    //□スタッフ部門
    #include "common/_index_.h" // 共通担当　　　：基本処理部品
    #include "mode/_index_.h"   // 担当マネージャ：動作モード
    //│
    //□メンバー(経路アダプタ)
    #include "member/_index_.h"
    //┴
  //┴
//┴