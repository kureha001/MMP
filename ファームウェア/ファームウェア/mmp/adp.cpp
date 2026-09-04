// filename : adp.cpp
//========================================================
// 経路アダプタ／統括マネージャ：アダプタを統括
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <vector> // 登録コンテナが使用
  #include <queue>  // 経路アダプタが使用
  #include <mutex>  // 経路アダプタが使用
  //│
  //■ＭＭＰシステム
  #include "adp.h"
  //│
  //■ＭＭＰシステム(アダプタ群)
  #include "adapter/_API_.h"        // <<抽象基底クラス>>
  #include "adapter/UART.cpp"       // UART
  #include "adapter/TCP.cpp"        // TCP RAW
  #include "adapter/WEB_API.cpp"    // WEB API
  #include "adapter/WEB_Socket.cpp" // WEB Socket
  #include "adapter/ESP_NOW.cpp"    // ESP-NOW
  #include "adapter/BLE.cpp"        // BLE
  #include "adapter/IIC.cpp"        // IIC
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト（実体化）
  //─────────────────
  MmpContext ctx;

  //─────────────────
  // 経路アダプタ群 (登録コンテナ)
  //─────────────────
  std::vector<AdapterBase*> ADAPTER;

//########################################################
//# 前空間：経路アダプタ・マネージャ
//########################################################
namespace AdapterManager{
  //========================================================
  //# アダプタの初期化（抽象化・一括管理）
  //========================================================
  void INIT() {
    //┬
    //○メッセージ表示を開始
    Serial.println("<<経路アダプタの初期化>>");
    //│
    //○ユーザ認証の初期化
      adpFnAuth::INIT_TBL();
    //│
    //●経路アダプタを初期化
    #if defined(ADP_UART)
      ADAPTER.push_back(new AdapterUART(ctx));
    #endif
    #if defined(ADP_TCP )
      ADAPTER.push_back(new AdapterTCP(ctx));
    #endif
    #if defined(ADP_WAPI)
      ADAPTER.push_back(new AdapterWEB_API(ctx));
    #endif
    #if defined(ADP_WSOC)
      ADAPTER.push_back(new AdapterWEB_Socket(ctx));
    #endif
    #if defined(ADP_BLE )
      ADAPTER.push_back(new AdapterBLE(ctx));
    #endif
    #if defined(ADP_ESPN)
      ADAPTER.push_back(new AdapterESPNOW(ctx));
    #endif
    #if defined(ADP_I2C )
      ADAPTER.push_back(new AdapterIIC(ctx));
    #endif
    //│
    //○メッセージ表示を終了
    Serial.println("");
    //┴
  } /* INIT_ADAPTER() */

  //========================================================
  // アダプタのハンドル
  //========================================================
  void HANDLE(){
    for (auto* adp : ADAPTER) if (adp) adp->handle();
  } /* KICK_HANDLE() */

} /* namespace AdapterManager */