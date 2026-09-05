// filename : connection/member/_index_.h
//========================================================
// クライアント接続部門／メンバー：経路アダプタ
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once
//┬
//□┐保有資源
  //│
  //□┐作業標準
    //│
    //□抽象基底クラス
    #include "_api_.h"
    //┴
  //│
  //□┐メンバー(経路アダプタ)
    //│
    //□UART
    #include "UART.cpp"
    namespace adpUART{
      void START()  ; // サービス開始の指示
      void HANDLE() ; // ポーリングのハンドル
    }
    //│
    //□TCP RAW
    #include "TCP.cpp"
    namespace adpTCP{
      void START()  ; // サービス開始の指示
      void HANDLE() ; // ポーリングのハンドル
    }
    //│
    //□WEB API
    #include "WEB_API.cpp"
    namespace adpWAPI{     
      void START()  ; // サービス開始の指示
      void HANDLE() ; // ポーリングのハンドル
    }
    //│
    //□WEB Socket
    #include "WEB_Socket.cpp"
    namespace adpWSOC{     
      void START()  ; // サービス開始の指示
      void HANDLE() ; // ポーリングのハンドル
    }
    //│
    //□BLE
    #include "BLE.cpp"
    namespace adpBLE{
      void START()  ; // サービス開始の指示
      void HANDLE() ; // ポーリングのハンドル
    }
    //│
    //□ESP NOW
    #include "ESP_NOW.cpp"
    namespace adpESPN{
      void START()  ; // サービス開始の指示
      void HANDLE() ; // ポーリングのハンドル
    }
    //│
    //□IIC
    #include "IIC.cpp"
    namespace adpI2C{
      void START()  ; // サービス開始の指示
      void HANDLE() ; // ポーリングのハンドル
    }
    //┴
  //┴
//┴