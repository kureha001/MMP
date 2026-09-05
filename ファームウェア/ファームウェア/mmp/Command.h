// filename : Command.h
//========================================================
// コマンド実行部門
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#ifndef CMD_H // コンパイラ不具合対策
#define CMD_H // adapter/mode/member/main.cpp での指定が認識できない
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
  #include "command/manager.cpp"
  namespace CommandManager {
    void INIT()      ; // 初期化
    void RunCommand(); // コマンド実行
  }; /* namespace CommandManager */
  //┴
//┴

#endif // CMD_H