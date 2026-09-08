// filename : Dep_Connect/adapter/bridge/_index_.h
//========================================================
// 接続部門／業務課／ブリッジ係：担当割一覧
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/07) 
//========================================================
#pragma once

//========================================================
// 担務
//========================================================

  //━━━━━━━━━━━━━━━━━
  // TCP RAW
  //━━━━━━━━━━━━━━━━━
  #include "TCP.cpp"
  namespace brdTCP{
    void BEGIN(int argMMP_IP4, uint16_t argPort);
    void SEND ();
    void END  ();
  } /* namespace brdESPN */

  //━━━━━━━━━━━━━━━━━
  // WEB Socket
  //━━━━━━━━━━━━━━━━━

  //━━━━━━━━━━━━━━━━━
  // WEB API
  //━━━━━━━━━━━━━━━━━

  //━━━━━━━━━━━━━━━━━
  // ESP NOW
  //━━━━━━━━━━━━━━━━━
  #include "ESP_NOW.cpp"
  namespace brdESPN{
    void BEGIN(String argMACStr);
    void SEND ();
    void END  ();
  } /* namespace brdESPN */

  //━━━━━━━━━━━━━━━━━
  // BLE
  //━━━━━━━━━━━━━━━━━
