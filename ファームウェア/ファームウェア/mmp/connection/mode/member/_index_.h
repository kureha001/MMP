// filename : connection/mode/member/_index_.h
//========================================================
// クライアント接続部門／動作モード／担当
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#ifndef MODE_MEMBER_H
#define MODE_MEMBER_H
#pragma once
//┬
//□┐クライアント接続部門
  //□┐統括マネージャ
    //□┐担当マネージャ
      //□担当
      #include "main.cpp"   // メインモード
      #include "sub.cpp"    // サブモード
      #include "bridge.cpp" // ブリッジモード
//┴┴┴┴

//========================================================
// 役割
//========================================================
  namespace modeMain  {void RUN();} // メインモード
  namespace modeSub   {void RUN();} // サブモード
  namespace modeBridge{void RUN();} // ブリッジモード

#endif // MODE_MEMBER_H
