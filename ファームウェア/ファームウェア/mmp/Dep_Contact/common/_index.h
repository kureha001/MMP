// filename : Dep_Contact/common/_index.h
//========================================================
// 接客部門／庶務課：資料・担当名簿
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/27)
//========================================================
#pragma once

//========================================================
//§提供情報
//========================================================
//┬
//□┐資料
  //□通信アダプタID
  namespace AID {
    inline constexpr int UART = 100;
    inline constexpr int UDP  = 200;  
    inline constexpr int TCP  = 201;
    inline constexpr int WSOC = 210;
    inline constexpr int HTTP = 211;
    inline constexpr int ESPN = 220;
    inline constexpr int BLE  = 300;
    inline constexpr int IIC  = 900;
  }
  //│
  //□ブリッジモードの進捗状況ID
  namespace BSTAT {
    inline constexpr int IDLE = 0; // 待機中
    inline constexpr int REQ  = 1; // 依頼中（マスタ→スレーブ）
    inline constexpr int BUSY = 2; // 処理中（スレーブ実行中）
    inline constexpr int DONE = 3; // 処理済（応答・完了）
  }
//┴┴

//========================================================
//§担当名簿
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 一般処理係
  //━━━━━━━━━━━━━━━━━
  #include "base.cpp"
  namespace adpFnBase{
    void SHOW_LOG();              // 通信アダプタの[SEND_CONN]で利用
    void FORMAT_URI(String &str); // [adpFnStream]で利用
    bool SysCmd(String argFrame); // [modeSub][modeBridge]で利用
    void SETUP_CTX(int argAID, String argFrame); // APIやアダプタのハンドルで利用
  }

  //━━━━━━━━━━━━━━━━━
  // ユーザ認証係
  //━━━━━━━━━━━━━━━━━
  static const String SP_CMD_START = "_START_!"; // [modeMain][AD_HTTP]で利用
  #include "sp_auth.cpp" 
  namespace adpFnAuth{
    void INIT_TBL(); // 初期化：[DepContact]で利用
    bool CHECK()   ; // 認証を実施・認証開始コマンド応答：[modeMain]で利用
  }

  //━━━━━━━━━━━━━━━━━
  // ストリーム受信係
  //━━━━━━━━━━━━━━━━━
  #include "sp_stream.cpp"
  namespace adpFnStream{
    String GET_FRAME(Stream& argConn); // UART,TCPアダプタの受信処理やハンドルで利用
  }