// filename : bridge_adapter/mode/bridge_adapter/_index_.h
//========================================================
// 経路アダプタ／動作モード／ブリッジモード：経路アダプタ
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/07) 
//========================================================
#ifndef MEMBER_BRIDGE_H
#define MEMBER_BRIDGE_H
#pragma once

//========================================================
// 必要な資源
//========================================================
//┬
//□┐接続部門
  //□┐統括マネージャ
    //□担当：経路アダプタ
    #include "../_api0_.h" // 基本
    #include "../_api1_.h" // 上記にキューイング処理を派生追加
//┴┴┴

//========================================================
// 役割
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 共通部品
  //━━━━━━━━━━━━━━━━━
  #include "_run_bridge.cpp"
  namespace modeBridge{void RUN();} // ブリッジモード

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