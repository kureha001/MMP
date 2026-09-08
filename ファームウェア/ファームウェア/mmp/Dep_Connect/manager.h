// filename : Dep_Connect/_index_.h
//========================================================
// 接続部門
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <Arduino.h>
//┴┴

//========================================================
// 共有資源
//========================================================
//┬
//□┐情報
  //│
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

//========================================================
// 組織図
//========================================================
//┬
//□┐接続部門
  //□共通課
  #include "common/_index_.h"
  //│
  //□┐業務課：担当課長
  #include "adapter/_index_.h"
    //│
    //□ダイレクト課
    //□ブリッジ課
     #include "adapter/direct/_index_.h" 
    //#include "adapter/bridge/_index_.h" 
  //┴┴
//│
//□通信部門 ※WiFiの状態を参照
#include "../Dep_Network/manager.h"
//┴┴

//========================================================
// 部門長の役務
//========================================================
  #include "manager.cpp"
  namespace DepConnect{
    void INIT(); // 部下招集
    void WORK(); // 業務遂行（接続応答）
  }