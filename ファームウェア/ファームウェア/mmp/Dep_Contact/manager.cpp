// filename : Dep_Contact/manager.cpp
//========================================================
// 接客部門：部門長
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/29)
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <vector> // 登録コンテナで利用
  #include <queue>  // 業務設計・通信アダプタで利用
  #include <mutex>  // 業務設計・通信アダプタで利用
//┴┴
//┬
//□┐接客部門
  //□庶務課（共通関数・共通定義など）
  #include "common/_index.h"
  //│
  //□業務手順書（モード別処理）
  #include "mode/_index.h"
  //│
  //□操作手順書（抽象クラス）
  #include "template/_index.h"
  //│
  //□担当（通信アダプタ）
  #include "adapter/_index.h" 
//┴┴

namespace DepContact{
//========================================================
// 共有資源
//========================================================
  //┬
  //□部下の座席：通信アダプタのコンテナ
  std::vector<AD*> ADAPTER;
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
      //●ユーザ認証担当
      adpFnAuth::INIT_TBL();
      //┴
    //│
    //○┐業務課（経路アダプタ）を招集
      //◇┐WiFi係
      if (devWiFi::ENABLED) {
        //├┐（通信部門で[WiFi準備]が完了している場合）
          //○UDP担当
          #if ADP_UDP
          ADAPTER.push_back(new AD_UDP(ctx));
          #endif
          //│
          //○TCP担当
          #if ADP_TCP
          ADAPTER.push_back(new AD_TCP(ctx));
          #endif
          //│
          //○WebSocket担当
          #if ADP_WSOC
          ADAPTER.push_back(new AD_WEBS(ctx));
          #endif
          //│
          //○HTTP担当
          #if ADP_HTTP
          ADAPTER.push_back(new AD_HTTP(ctx));
          #endif
          //│
          //○ESP-NOW担当
          #if ADP_ESPN
          ADAPTER.push_back(new AD_ESPN(ctx));
          #endif
          //┴
        //┴
      } //～if
      //│
      //○┐個別係
        //│
        //○UART担当(ブリッジは必須)
        #if ADP_UART || MODE == MODE_BRIDGE
        if (devUART::ENABLED) ADAPTER.push_back(new AD_UART(ctx));
        #endif
        //│
        //○BLE担当
        #if ADP_BLE
        if (devBLE::ENABLED) ADAPTER.push_back(new AD_BLE(ctx));
        #endif
        //│
        //○┐IIC担当
        #if ADP_IIC
        if (devIIC::ENABLED) ADAPTER.push_back(new AD_IIC(ctx));
        #endif
        //┴
    //│
    //○始業のあいさつ（終了）
    Log::prtln("");
    //┴
  } /* INIT_ADAPTER() */

  //━━━━━━━━━━━━━━━━━
  //（２）本部の(通常業務の)遂行指示に応じる
  // → 部下に業務遂行を指示
  //━━━━━━━━━━━━━━━━━
  void HANDLE(){
    //┬
    //◎┐担当に通常業務の遂行を指示する
    for (auto* adp : ADAPTER) {
      //│＼（全員に指示を終えた場合）
      //│ ▽完了:業務遂行の指示を終える
      //○この担当に指示
      if (adp) adp->handle();
      //┴
    //┴
    } //～for
  } /* WORK() */

} /* namespace DepConnect */