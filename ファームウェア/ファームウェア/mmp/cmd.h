// filename : cmd.h
//========================================================
// コマンド・マネージャ：経路アダプタと機能モジュールを連携する
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <Arduino.h>
  #include <vector>
  //│
  //■ＭＭＰシステム
  #include "conf.h"
  #include "context.h"
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト
  // ※_API_.h より前に宣言が必要
  //─────────────────
  extern MmpContext ctx;

//┬
//■インクルード（モジュール基底）
  #include "module/_API_.h"
//┴

//━━━━━━━━━━━━━━━━━
// コマンド・マネージャ
//━━━━━━━━━━━━━━━━━
namespace CommandManager {

  //─────────────────
  // クライアントからのリクエスト条件
  //─────────────────
  #define REQUEST_LENGTH 96 // リクエスト全体のバッファ長
  #define DAT_COUNT      10 // コマンド＋引数の個数
  #define DAT_LENGTH     20 // トークン最大長（未定義時のフォールバック）

  //─────────────────
  // 機能モジュール構造体
  //─────────────────
  struct T_MOD {
      const char* name; // 名前
      const char* desc; // 説明
  };

  //─────────────────
  // 機能モジュール・コンテナ（実体は cmd.cpp に配置）
  //─────────────────
  extern std::vector<ModuleBase*> MODULE;

  //─────────────────
  // 公開関数宣言
  //─────────────────
  void CLEAN();
  void INIT();
  void SHOW_DESC(String argName);
  void RunCommand();

}; /* namespace CommandManager */