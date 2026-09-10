// filename : mmpContext.h
//========================================================
// コンテクスト
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/06) 
//========================================================
#ifndef CONTEXT_H
#define CONTEXT_H
#pragma once

//========================================================
// コンテクスト
//========================================================
  struct T_BRIDGE {
    int    adpID = -1; // 転送先のアダプタID
    int    Stat  =  0; // 進行状況
    String Dat1    = ""; // 転送先の個別情報１
    String Dat2    = ""; // 転送先の個別情報２
    String Dat3    = ""; // 転送先の個別情報３
  };

  struct MmpContext {
  //┬
  //■システム情報
  const String sysVer  = "V13a!"  ; // バージョン
  bool         sysLog  = true     ; // ログ表示
  //│
  //■リクエスト情報
  int          adpID    = -1; // 経路アダプタID
  String       strFrame = ""; // フレーム(リクエスト内容)
  String       cmdPath  = ""; // コマンドパス
  String       authCD   = ""; // 認証コード
  //│
  //■転送情報（ブリッジモードで使用）
  T_BRIDGE     bridge;
  //│
  //■ユーザメモリ情報（特定の機能モジュールで使用）
  int          accID    = -1; // アクセスID(MMP全体で一意)
  const int    accIDS   = 30; // アクセスIDの総数(ユーザ認証スロット数)
  //│
  //■レスポンス情報（クライアントへの返却メッセージ）
  String       resMSG  = "" ;
  //┴
};

#endif // CONTEXT_H