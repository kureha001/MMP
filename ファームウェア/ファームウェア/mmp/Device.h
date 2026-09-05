// filename : dev.h
//========================================================
// 通信デバイス部門
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once
//┬
//■インクルード
  #include <Arduino.h>
  //│
  //■ＭＭＰシステム
  #include "conf.h"    // 初期設定
  //┴
//┴

  //│
  //□組織（統括マネージャ）
  #include "device/manager.cpp"
  namespace DeviceManager{
    void INIT();
  } /* namespace DeviceManager */