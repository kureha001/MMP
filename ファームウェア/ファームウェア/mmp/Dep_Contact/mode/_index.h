// filename : Dep_Contact/mode/__index.h
//========================================================
// 接客部門／窓口手順書（モード別処理）：目次
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/27)
//========================================================
#pragma once
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <functional>
//┴┴

//========================================================
//§担当名簿
//========================================================
    //─────────────────
    // メインモード係
    //─────────────────
    #include "main.cpp"
    namespace modeMain{void RUN();}

    //─────────────────
    // サブモード係
    //─────────────────
    #include "sub.cpp"
    namespace modeSub{void RUN();}

    //─────────────────
    // ブリッジモード係
    //─────────────────
    #include "bridge.cpp"
    namespace modeBridge{
      void RUN  (int argSID, String argFrame);
      void MOVE_BSTAT(int argBSTAT, String argMSG);
      bool MASER(Stream* argConn, std::function<void(Stream*)> argSendConn);
      bool SLAVE(int     argAID , std::function<void()       > argTrans   );
    }