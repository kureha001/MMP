// filename : connection/mode/member/_index_.h
//========================================================
// 経路アダプタ／動作モード／ブリッジモード：経路アダプタ
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/07) 
//========================================================
#ifndef MEMBER_BRIDGE_H
#define MEMBER_BRIDGE_H
#pragma once

//========================================================
// 役割
//========================================================
  // TCP
  #include "TCP.cpp"
  namespace brdTCP{
    void BEGIN(int argMMP_IP4, uint16_t argPort);
    void SEND ();
    void END  ();
  } /* namespace brdESPN */

  // ESP-NOW
  #include "ESP_NOW.cpp"
  namespace brdESPN{
    void BEGIN(String argMACStr);
    void SEND ();
    void END  ();
  } /* namespace brdESPN */

#endif // MEMBER_BRIDGE_H