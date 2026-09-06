// filename : adapter/_member_.h
//========================================================
// コマンド部門／メンバー：機能モジュール
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#ifndef COMMAND_MEMBER_H
#define COMMAND_MEMBER_H
#pragma once

//┬
//□┐情報
  //□作業標準（抽象基底クラス）
  #include "_api_.h"
//│┴
//│
//□┐クライアント接続部門
  //□┐統括マネージャ
    //□担当：機能モジュール
    #include "system.h"  // システム管理
    #include "analog.h"  // アナログ入力
    #include "digital.h" // デジタル入出力
    #include "pwm.h"     // PWM出力
    #include "IIC.h"     // IIC通信
    #include "mp3.h"     // MP3プレイヤー
//┴┴┴

#endif // COMMAND_MEMBER_H