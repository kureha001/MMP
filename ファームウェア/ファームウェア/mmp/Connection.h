// filename : adp.h
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
  #include "conf.h"    // 初期設定
  #include "context.h" // コンテクスト
  extern MmpContext ctx;
  //│
  //□組織（統括マネージャ）
  #include "connection/manager.cpp"
  namespace AdapterManager{
    void INIT()  ; // 初期化
    void HANDLE(); // ハンドルをキック
  }
  //┴
//┴