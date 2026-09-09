// filename : Dep_Connect/adapter/base/_index_.h
//========================================================
// 接続部門／業務課／担当(標準型)：担当割一覧
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/10)
//========================================================
#pragma once

//========================================================
// 担務
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
  // TCP RAW
  //━━━━━━━━━━━━━━━━━
  #include "TCP.cpp"
  namespace adpTCP{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //━━━━━━━━━━━━━━━━━
  // WEB API
  //━━━━━━━━━━━━━━━━━
  #include "HTTP.cpp"
  namespace adpHTTP{     
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