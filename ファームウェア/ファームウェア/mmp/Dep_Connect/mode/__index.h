// filename : Dep_Connect/mode/__index.h
//========================================================
// 接続部門／処理手順：担当名簿
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/23)
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
      bool MASER(Stream* argConn, std::function<void(Stream*)> argSendConn);
      bool SLAVE(int     argAID , std::function<void()       > argTrans   );
    }