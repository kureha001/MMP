// filename : Dep_Connect/manager.cpp
//========================================================
// 接続部門：統括マネージャ
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <vector> // 登録コンテナが使用
  #include <queue>  // 経路アダプタが使用
  #include <mutex>  // 経路アダプタが使用
//┴┴

//========================================================
// 必要な資源
//========================================================
//┬
//□接続部門
#include "_index_.h"
//│
//□通信部門 ※WiFiの状態を参照
#include "../Dep_Network.h"
//┴

//########################################################
//# 統括マネージャの役務（詳細）
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
  // １．部下を招集
  //━━━━━━━━━━━━━━━━━
  void INIT() {
    //┬
    //○始業のあいさつ（開始）
    Serial.println("<<経路アダプタの初期化>>");
    //│
    //○┐スタッフ係を招集
      //│
      //●ユーザ認証担当
      adpFnAuth::INIT_TBL();
      //┴
    //│
    //○┐業務係（経路アダプタ）を招集
      //│
      //◇┐WiFi係
      if (devWiFi::ENABLED) {
        //├┐（通信部門で[WiFi準備]が完了している場合）
          //○TCP担当
          #if ADP_TCP
          ADAPTER.push_back(new AdapterTCP(ctx));
          #endif
          //│
          //○WebAPI担当
          #if ADP_WAPI
          ADAPTER.push_back(new AdapterWEB_API(ctx));
          #endif
          //│
          //○WebSocket担当
          #if ADP_WSOC
          ADAPTER.push_back(new AdapterWEB_Socket(ctx));
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
      //○┐専門係
        //│
        //○UART担当
        #if (MODE!=MODE_MAIN) || ADP_UART //※メインモード以外は強制使用
        if (devUART::ENABLED) ADAPTER.push_back(new AdapterUART(ctx));
        #endif
        //│
        //○BLE担当
        #if ADP_BLE
        if (devBLE::ENABLED) ADAPTER.push_back(new AdapterBLE(ctx));
        #endif
        //│
        //○┐IIC担当
        #if (MODE != MODE_MAIN) && ADP_IIC //※メインモードは使用不可
        if (devIIC::ENABLED) ADAPTER.push_back(new AdapterIIC(ctx));
        #endif
        //┴
    //│
    //○始業のあいさつ（終了）
    Serial.println("");
    //┴
  } /* INIT_ADAPTER() */

  //━━━━━━━━━━━━━━━━━
  // ２．部下に業務遂行を指示
  //━━━━━━━━━━━━━━━━━
  void WORK(){
    //┬
    //◎┐業務係に業務遂行を指示
    for (auto* adp : ADAPTER) {
      //│＼（全員に指示を終えた場合）
      //│ ▽完了:業務遂行の指示を終える
      //│
      //○この担当に指示
      if (adp) adp->handle();
      //┴
    //┴
    } /* END-for */
  } /* WORK() */

} /* namespace DepConnect */