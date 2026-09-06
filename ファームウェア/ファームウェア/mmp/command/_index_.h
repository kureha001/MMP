// filename : command/_index_.h
//========================================================
// コマンド実行部門
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#ifndef CONN_COMMAND_H
#define CONN_COMMAND_H
#pragma once

//┬
//□┐情報
  //□制限事項
  #define REQUEST_LENGTH 96 // リクエスト全体のバッファ長
  #define DAT_COUNT      10 // コマンド＋引数の個数
  #define DAT_LENGTH     20 // トークン最大長（未定義時のフォールバック）
  //│
  //□機能モジュール情報
  struct T_MOD {
    const char* name; // 名前
    const char* desc; // 説明
  };
//│┴
//│
//□┐クライアント接続部門
  //□┐統括マネージャ
    //□┐担当
      //□機能モジュール
      #include "member/_index_.h"
//┴┴┴┴

#endif // CONN_COMMAND_H