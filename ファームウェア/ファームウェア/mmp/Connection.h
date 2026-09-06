// filename : Connection.h
//========================================================
// クライアント接続部門
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#ifndef CONN_H
#define CONN_H
#pragma once
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <Arduino.h>
//┴┴
//┬
//□┐情報
  //□環境設定
  #include "mmpConfig.h"
  //│
  //□コンテクスト
  #include "mmpContext.h"
  extern MmpContext ctx;
//┴┴

//========================================================
// 役割
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 統括マネージャ
  //━━━━━━━━━━━━━━━━━
  #include "connection/manager.cpp"
  namespace ConnectionManager{
    void INIT(); // 初期化
    void WORK(); // 通常活動
  }

#endif // CONN_H