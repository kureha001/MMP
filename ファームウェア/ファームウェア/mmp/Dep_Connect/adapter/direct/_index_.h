// filename : Dep_Connect/adapter/direct/_index_.h
//========================================================
// 接続部門／業務課／ダイレクト：担当割一覧
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/06) 
//========================================================
#ifndef CONN_ADP_D_H
#define CONN_ADP_D_H
#pragma once

//========================================================
// 担務
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 共通部品
  //━━━━━━━━━━━━━━━━━
  #include "_run_main.cpp"
  #include "_run_sub.cpp"
  namespace modeMain {void RUN();} // メインモード
  namespace modeSub  {void RUN();} // サブモード

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

#endif