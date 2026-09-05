// filename : adapter/_index_.h
//========================================================
// クライアント接続部門：部内組織
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once

//━━━━━━━━━━━━━━━━━
// 部門共通資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 経路ID（RID : Route ID）
  //─────────────────
  inline constexpr int ADP_ID_UART = 0;
  inline constexpr int ADP_ID_TCP  = 1;
  inline constexpr int ADP_ID_WAPI = 2;
  inline constexpr int ADP_ID_WSOC = 3;
  inline constexpr int ADP_ID_BLE  = 4;
  inline constexpr int ADP_ID_ESPN = 5;
  inline constexpr int ADP_ID_IIC  = 6;

  //━━━━━━━━━━━━━━━━━
  // 専門処理：ストリーム受信
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // 基本情報
    //─────────────────
    static const int SS_RX_SIZE = 128 ; // 受信バッファ容量

    //─────────────────
    // 構造体
    //─────────────────
    struct SS_SLOT_TYPE {
      bool    used   = false ; // スロット有効性
      String  rx     = ""    ; // 受信バッファ
      bool    isOver = false ; // 容量超過フラグ
    };

  //■組織
  #include "adapter/common/_index_.h" // 共通担当　　　：基本処理部品
  #include "adapter/mode/_index_.h"   // 担当マネージャ：動作モード

  //■部下
  #include "adapter/member/_index_.h" // メンバー　　　：経路アダプタ
  //┴
//┴