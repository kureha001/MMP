// filename : connection/member/_index_.h
//========================================================
// クライアント接続部門／担当：経路アダプタ
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/06) 
//========================================================
#pragma once
//┬
//□┐情報
  //□作業標準（抽象基底クラス）
  #include "_api0_.h" // 基本
  #include "_api1_.h" // 上記にキューイング処理を派生追加
//┴┴

//========================================================
// 役割
//========================================================
  //━━━━━━━━━━━━━━━━━
  // UART
  //━━━━━━━━━━━━━━━━━
  #include "UART.cpp"
  namespace adpUART{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //━━━━━━━━━━━━━━━━━
  //TCP RAW
  //━━━━━━━━━━━━━━━━━
  #include "TCP.cpp"
  namespace adpTCP{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //━━━━━━━━━━━━━━━━━
  // WEB API
  //━━━━━━━━━━━━━━━━━
  #include "WEB_API.cpp"
  namespace adpWAPI{     
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //━━━━━━━━━━━━━━━━━
  // WEB Socket
  //━━━━━━━━━━━━━━━━━
  #include "WEB_Socket.cpp"
  namespace adpWSOC{     
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //━━━━━━━━━━━━━━━━━
  // BLE
  //━━━━━━━━━━━━━━━━━
  #include "BLE.cpp"
  namespace adpBLE{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //━━━━━━━━━━━━━━━━━
  // ESP NOW
  //━━━━━━━━━━━━━━━━━
  #include "ESP_NOW.cpp"
  namespace adpESPN{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //━━━━━━━━━━━━━━━━━
  //IIC
  //━━━━━━━━━━━━━━━━━
  #include "IIC.cpp"
  namespace adpI2C{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }
//┴┴┴┴