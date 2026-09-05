// filename : connection/common/_index_.h
//========================================================
// クライアント接続部門／共通担当：基本処理
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once

//━━━━━━━━━━━━━━━━━
// 一般処理：ユーザ認証
//━━━━━━━━━━━━━━━━━
#include "normal.cpp"
namespace adpFnBase{
  void FORMAT_URI(String &str);  // [adpFnStream]で利用
  void RUN(int argAdpID, String argFrame);
  void SHOW_LOG();
}

//━━━━━━━━━━━━━━━━━
// 専門処理：ユーザ認証
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 特殊コマンド名
  //─────────────────
  static const String SP_CMD_START = "_START_!"; // [modeMain][adpWAPI]で利用

  //─────────────────
  // 実行部品
  //─────────────────
  #include "sp_auth.cpp" 
  namespace adpFnAuth{
    void INIT_TBL(); // [ConnectionManager]で利用
    bool CHECK()   ; // [adpFnBase]で利用
  }

  //─────────────────
  // 実行部品
  //─────────────────
  #include "sp_stream.cpp"
  namespace adpFnStream{
    void   SS_INI_SLOT_BASE(SS_SLOT_TYPE& argSlot);
    String GET_FRAME(Stream& argConn, SS_SLOT_TYPE argBASES);
  }
