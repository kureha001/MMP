// filename : Dep_Connect/common/_index_.h
//========================================================
// 接続部門／共通課：担当割一覧
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/14)
// ・[adpFnBase::SETUP_CTX()]を追加([AdapterQueueBase]の処理を移動)
// ・[adpFnAuth::]を非公開化
//========================================================
#ifndef CONN_COMMON_H
#define CONN_COMMON_H
#pragma once

//========================================================
// 担務
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 一般処理
  //━━━━━━━━━━━━━━━━━
  #include "normal.cpp"
  namespace adpFnBase{
    void FORMAT_URI(String &str);  // [adpFnStream]で利用
    void RUN(int argAdpID, String argFrame);
    void SHOW_LOG();
    void SETUP_CTX(int argAID, String argFrame);
  }

  //━━━━━━━━━━━━━━━━━
  // 専門処理：ユーザ認証
  //━━━━━━━━━━━━━━━━━
  static const String SP_CMD_START = "_START_!"; // [modeMain][adpWAPI]で利用
  #include "sp_auth.cpp" 
  namespace adpFnAuth{
    void INIT_TBL(); // [DepConnection]で利用
    bool CHECK()   ; // [adpFnBase]で利用
  }

  //━━━━━━━━━━━━━━━━━
  // 専門処理：ストリーム受信
  //━━━━━━━━━━━━━━━━━
  #include "sp_stream.cpp"
  namespace adpFnStream{
    String GET_FRAME(Stream& argConn);
  }

#endif // CONN_COMMON_H
