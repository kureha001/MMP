// filename : Dep_Connect/manager.cpp
//========================================================
// 接続部門：部門長
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/22)
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <vector> // 登録コンテナが使用
  #include <queue>  // 経路アダプタが使用
  #include <mutex>  // 経路アダプタが使用
//┴┴

//========================================================
// 組織図
//========================================================
//┬
//□┐接続部門
  //□共通課
  #include "common/__index.h"
  //│
  //□┐業務課
    //│
    //□作業標準：抽象基底クラス、モード部品
    #include "template/__index.h"
    //│
    //□担当
    #include "adapter/__index.h" 
//┴┴┴

//########################################################
//# 部門長の役務（詳細）
//########################################################
namespace DepConnect{
//========================================================
// 共有資源
//========================================================
  //┬
  //□部下の座席
  std::vector<AdapterBase*> ADAPTER;
  //┴

//========================================================
// 公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  //（１）本部の始業指示に応じる
  // → 部下を招集
  //━━━━━━━━━━━━━━━━━
  void INIT() {
    //┬
    //○始業のあいさつ（開始）
    Log::prtln("<<経路アダプタの初期化>>");
    //│
    //○┐共通課を招集
      //│
      //●ユーザ認証担当
      adpFnAuth::INIT_TBL();
      //┴
    //│
    //○┐業務課（経路アダプタ）を招集
      //│
      //◇┐WiFi係
      if (devWiFi::ENABLED) {
        //├┐（通信部門で[WiFi準備]が完了している場合）
          //○UDP担当
          #if ADP_UDP
          ADAPTER.push_back(new AdapterUDP(ctx));
          #endif
          //│
          //○TCP担当
          #if ADP_TCP
          ADAPTER.push_back(new AdapterTCP(ctx));
          #endif
          //│
          //○WebSocket担当
          #if ADP_WSOC
          ADAPTER.push_back(new AdapterWEB_Socket(ctx));
          #endif
          //│
          //○HTTP担当
          #if ADP_HTTP
          ADAPTER.push_back(new AdapterHTTP(ctx));
          #endif
          //│
          //○ESP-NOW担当
          #if ADP_ESPN
          ADAPTER.push_back(new AdapterESPNOW(ctx));
          #endif
          //┴
        //┴
      } /* END-if */
      //│
      //○┐個別係
        //│
        //○UART担当(ブリッジは必須)
        #if ADP_UART || MODE == MODE_BRIDGE
        if (devUART::ENABLED) ADAPTER.push_back(new AdapterUART(ctx));
        #endif
        //│
        //○BLE担当
        #if ADP_BLE
        if (devBLE::ENABLED) ADAPTER.push_back(new AdapterBLE(ctx));
        #endif
        //│
        //○┐IIC担当
        #if ADP_IIC //※メインモードは使用不可
        if (devIIC::ENABLED) ADAPTER.push_back(new AdapterIIC(ctx));
        #endif
        //┴
    //│
    //○始業のあいさつ（終了）
    Log::prtln("");
    //┴
  } /* INIT_ADAPTER() */

  //━━━━━━━━━━━━━━━━━
  //（２）本部の通常活動の指示に応じる
  // → 部下に業務遂行を指示
  //━━━━━━━━━━━━━━━━━
  void WORK(){
    //┬
    //◎┐業務課の担当に業務遂行を指示
    for (auto* adp : ADAPTER) {
      //│＼（全員に指示を終えた場合）
      //│ ▽完了:業務遂行の指示を終える
      //○この担当に指示
      if (adp) adp->handle();
      //┴
    //┴
    } /* END-for */
  } /* WORK() */

} /* namespace DepConnect */