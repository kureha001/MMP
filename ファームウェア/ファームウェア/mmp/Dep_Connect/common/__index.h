// filename : Dep_Connect/common/__index.h
//========================================================
// 接続部門／共通課：担当割一覧
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/22)
//========================================================
#ifndef CONN_COMMON_H
#define CONN_COMMON_H
#pragma once

//========================================================
//§部門共有情報
//========================================================
//┬
//□┐情報
  //□経路ID
  inline constexpr int ADP_ID_UART = 100;
  inline constexpr int ADP_ID_UDP  = 200;  
  inline constexpr int ADP_ID_TCP  = 201;
  inline constexpr int ADP_ID_WSOC = 210;
  inline constexpr int ADP_ID_HTTP = 211;
  inline constexpr int ADP_ID_ESPN = 220;
  inline constexpr int ADP_ID_BLE  = 300;
  inline constexpr int ADP_ID_IIC  = 900;
  //│
  //□ブリッジの進捗状況
  namespace BSTAT {
    inline constexpr int IDLE = 0; // 待機中
    inline constexpr int REQ  = 1; // 依頼中（マスタ→スレーブ）
    inline constexpr int BUSY = 2; // 処理中（スレーブ実行中）
    inline constexpr int DONE = 3; // 処理済（応答・完了）
  }
//┴┴

//========================================================
//§担当
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
