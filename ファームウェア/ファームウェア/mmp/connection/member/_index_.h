// filename : connection/member/_index_.h
//========================================================
// クライアント接続部門／メンバー：経路アダプタ
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■在籍一覧
  #include "_api_.h"        // <<抽象基底クラス>>
  #include "UART.cpp"       // UART
  #include "TCP.cpp"        // TCP RAW
  #include "WEB_API.cpp"    // WEB API
  #include "WEB_Socket.cpp" // WEB Socket
  #include "ESP_NOW.cpp"    // ESP-NOW
  #include "BLE.cpp"        // BLE
  #include "IIC.cpp"        // IIC
  //┴
//┴

//========================================================
// 同僚：経路アダプタ
//========================================================
  //─────────────────
  // UART
  //─────────────────
  namespace adpUART{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //─────────────────
  // TCP RAW
  //─────────────────
  namespace adpTCP{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //─────────────────
  // WEB API
  //─────────────────
  namespace adpWAPI{     
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //─────────────────
  // WEB Socket
  //─────────────────
  namespace adpWSOC{     
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //─────────────────
  // BLE
  //─────────────────
  namespace adpBLE{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //─────────────────
  // ESP NOW
  //─────────────────
  namespace adpESPN{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //─────────────────
  // IIC
  //─────────────────
  namespace adpI2C{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }