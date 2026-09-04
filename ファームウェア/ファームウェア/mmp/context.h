// filename : context.h
//========================================================
// ＭＭＰコンテクスト
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/23) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <Arduino.h>
  //│
  //■ＭＭＰシステム
  #include "conf.h"    // プリプロセッサ
  //┴
//┴

//========================================================
// コンテクスト
//========================================================
struct MmpContext {
  //┬
  //■システム情報
#if defined(MMP_TYPE_MAIN)
  const String sysName = "MMP本体";
  boolean      sysLog  = true  ; // ログレベル {false:なし | true:あり}
#else
  const String sysName = "MMPサブ";
  boolean      sysLog  = true  ; // ログレベル {false:なし | true:あり}
#endif
  //■システム
  const String sysVer  = "V12a!"; // バージョン
  //│
  //■レスポンス
  String       resMSG  = "" ; // レスポンスメッセージ
  //│
  //■リクエスト情報---
  String       adpID    = ""; // アダプタID
  String       strFrame = ""; // フレーム
  String       cmdPath  = ""; // コマンドパス
  String       authCD   = ""; // 認証コード
  //│
  //■ユーザメモリ管理のアドレス情報
  int          accID    = -1; // アクセスID(MMP全体の一意な番号)
  const int    accIDS   = 30; // アクセスIDの総数(ユーザ認証スロット数)
  //┴
};