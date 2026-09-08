// filename : Dep_Connect/adapter/bridge/_index_.h
//========================================================
// 接続部門／業務課／ブリッジ係：担当割一覧
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/07) 
//========================================================
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