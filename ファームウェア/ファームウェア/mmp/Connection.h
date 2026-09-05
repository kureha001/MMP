// filename : Connection.h
//========================================================
// クライアント接続部門
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <Arduino.h>
  //┴
//┴
//┬
//□┐クライアント接続部門（保有資源）
  //│
  //□全体資源
  #include "mmpConfig.h"  // 初期設定
  #include "mmpContext.h" // コンテクスト
  extern MmpContext ctx;
  //│
  //□組織（統括マネージャ）
  #include "connection/manager.cpp"
  namespace ConnectionManager{
    void INIT()  ; // 初期化
    void HANDLE(); // ハンドルをキック
  }
  //┴
//┴