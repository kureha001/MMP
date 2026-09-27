// filename : mmp.h
//========================================================
// 全体共通
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/27)
//========================================================
#ifndef MMP_H
#define MMP_H
#pragma once

//========================================================
//§コンテクスト
//========================================================
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // 構造体定義
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  //┬
  //■┐構造体定義
    //■基本情報
    struct T_BASE {
      int    AID   = -1; // 経路アダプタID
      String Frame = ""; // フレーム(リクエスト内容)
      String Cmd   = ""; // コマンドパス
      String Msg   = ""; // レスポンスMSG
    };
    //│
    //■転送情報
    struct T_TRANS {
      int    Stat  =  0; // 進行状況
      int    AID   = -1; // 通信アダプタID
      int    SID   =  0; // 接続元のスロットID
      String Dat1  = ""; // 個別情報１
      String Dat2  = ""; // 個別情報２
      String Dat3  = ""; // 個別情報３
      String Frame = ""; // フレーム(リクエスト内容)
      String Msg   = ""; // レスポンスMSG
    };
    //│
    //■ユーザ認証情報
    struct T_ACCESS {
      String    CD  = ""; // 認証コード
      int       ID  = -1; // アクセスID(MMP全体で一意)
      const int IDS = 30; // アクセスIDの総数(ユーザ認証スロット数)
    };
  //┴┴

  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // 構造体定義（本体）
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  //┬
  //■┐コンテクスト（本体）
  struct MmpContext {
    //■システム情報
    const String sysVer  = "V132!"  ; // バージョン
    //│
    //■基本情報
    T_BASE       base;
    //│
    //■転送情報
    T_TRANS      trans;
    //│
    //■ユーザ認証情報
    T_ACCESS     access;
  //┴┴
  };
#endif // MMP_H

//========================================================
//§リターンCD
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
  const String Trn0Err = "#TR0!"; // ブリッジ対象外 
  const String Trn1Err = "#TR1!"; // 
  const String Trn2Err = "#TR2!"; // 

  //HTTPの疑似CD
  const String OK_Auth = "_AUT!"; // OK:認証
  const String OK_VAL  = "_VAL!"; // OK:数値
  const String OK_STR  = "_STR!"; // OK:文字列
} /* namespace RCD */

//========================================================
//§制限
//========================================================
namespace LIMIT{
  const int TIME_READ    = 2000;
  const int TIME_CONNECT = 10000;
  const int READ_LEN     = 80;
} /* namespace READ_LIMIT */

//========================================================
//§ログ出力
//========================================================
namespace Log{
  bool ENABLE   = false; // ログ出力有効性
  void prtln(String argMSG) {Serial0.println (argMSG);}
  void prt  (String argMSG) {Serial0.print   (argMSG);}
  void Outln(String argMSG) {if(ENABLE) prtln(argMSG);}
  void Out  (String argMSG) {if(ENABLE) prt  (argMSG);}
} /* namespace Log */