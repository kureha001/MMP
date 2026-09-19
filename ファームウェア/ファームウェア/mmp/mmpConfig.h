// filename : mmpConfig.h
//========================================================
// 環境設定
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/16)
// ・UARTポートの見直し 
// ・ベース部と選択部を分離
// ・モード別にカテゴリ化
// ・ターボモードを追加
//========================================================
#ifndef CONFIG_H
#define CONFIG_H
#pragma once

//========================================================
// ベース（編集禁止）
//========================================================
  //─────────────────
  // 経路アダプタ
  //─────────────────
    #define ADP_UART true
    #define ADP_TCP  false
    #define ADP_HTTP false
    #define ADP_WSOC false
    #define ADP_ESPN false
    #define ADP_BLE  false
    #define ADP_IIC  false //※メイン・ブリッジは使用不可

  //─────────────────
  // 動作モード
  //─────────────────
  #define MODE_MAIN    0 // メインモード
  #define MODE_SUB     1 // サブモード
  #define MODE_BRIDGE  2 // ブリッジモード

//========================================================
// コンパイルオプション
//========================================================
  #define MODE  MODE_BRIDGE

  // UART高速モード
  // USB(CDC）の単一スロット＆パケット処理
  // 強制制御：[Dep_Connect/adapter/base/_index_.h]にて実施
  //・メ イ ン：サブ連携できない
  //・サ　　ブ：GPIO UART使用不可
  //・ブリッジ：GPIO UART使用不可
  #define TURBO false

  //□メインモード用
  #if   (MODE == MODE_MAIN)
    #define ADP_UART true 
    #define ADP_TCP  false
    #define ADP_HTTP false
    #define ADP_WSOC false
    #define ADP_ESPN false
    #define ADP_BLE  false

  //□サブモード用
  #elif (MODE == MODE_SUB)
    #define ADP_UART false 
    #define ADP_TCP  false
    #define ADP_HTTP false
    #define ADP_WSOC false
    #define ADP_ESPN true
    #define ADP_BLE  false
    #define ADP_IIC  false

  //□ブリッジモード用
  #elif (MODE == MODE_BRIDGE)
    #define ADP_TCP  true
    #define ADP_HTTP true
    #define ADP_WSOC true
    #define ADP_ESPN true
    #define ADP_BLE  true

  #endif

#endif // CONFIG_H

//========================================================
// リターンCD
//========================================================
namespace RCD{

  //一般用
  const String OK     = "_OK_!"; // 正常終了
  const String NotMod = "#MOD!"; // モジュール名が不正
  const String NotCmd = "#CMD!"; // コマンド名が不正
  const String ChkErr = "#CHK!"; // 引数チェックで不正
  const String IniErr = "#INI!"; // データが未初期化
  const String DevErr = "#DEV!"; // 使用不可のデバイス
  const String FilErr = "#FIL!"; // ファイル操作が異常終了
  const String NoDErr = "#NOD!"; // データ項目名が不正  
  const String ValErr = "#VAL!"; // 数値変換エラー  
  const String TimOut = "#TIO!"; // 数値変換エラー  

  //ユーザ認証用
  const String AuthErr1 = "#SS1!"; // 認証CD発行に失敗
  const String AuthErr2 = "#SS2!"; // 認証NG

  //ブリッジモード用
  const String Trn1Err = "#TR1!"; // 
  const String Trn2Err = "#TR2!"; // 

  //HTTPの疑似CD
  const String OK_Auth = "_AUT!"; // OK:認証
  const String OK_VAL  = "_VAL!"; // OK:数値
  const String OK_STR  = "_STR!"; // OK:文字列
} /* namespace RCD */

//========================================================
// 制限
//========================================================
namespace LIMIT{
  const int TIMEOUT_READ    = 2000;
  const int TIMEOUT_CONNECT = 10000;
} /* namespace READ_LIMIT */

//========================================================
// ログ出力
//========================================================
namespace Log{
  bool ENABLE = false; // ログ出力有効性
  void prtln(String argMSG) {Serial0.println(argMSG);}
  void prt  (String argMSG) {Serial0.print  (argMSG);}
} /* namespace Log */