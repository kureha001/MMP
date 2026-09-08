// filename : Dep_Connect/manager.cpp
//========================================================
// 接続部門：部門長
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
// 共有資源
//========================================================
//┬
//□┐情報
  //│
  //□経路ID
  inline constexpr int ADP_ID_UART = 0;
  inline constexpr int ADP_ID_TCP  = 1;
  inline constexpr int ADP_ID_WAPI = 2;
  inline constexpr int ADP_ID_WSOC = 3;
  inline constexpr int ADP_ID_BLE  = 4;
  inline constexpr int ADP_ID_ESPN = 5;
  inline constexpr int ADP_ID_IIC  = 6;
  //│
  //□ストリーム受信型の接続管理
  struct SS_SLOT_TYPE {      // 接続管理スロット
    bool    used   = false ; // スロット有効性
    String  rx     = ""    ; // 受信バッファ
    bool    isOver = false ; // 受信バッファ容量超過判定
  };
  inline constexpr int SS_RX_SIZE = 128; // 受信バッファ容量
//┴┴

//========================================================
// 組織図
//========================================================
//┬
//□┐接続部門
  //□共通課
  #include "common/_index_.h"
  //│
  //□┐業務課：担当課長
  #include "adapter/_index_.h"
    //│
    //□ダイレクト課
    //□ブリッジ課
     #include "adapter/direct/_index_.h" 
    //#include "adapter/bridge/_index_.h" 
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
    Serial.println("<<経路アダプタの初期化>>");
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
  //（２）本部の通常活動の指示に応じる
  // → 部下に業務遂行を指示
  //━━━━━━━━━━━━━━━━━
  void WORK(){
    //┬
    //◎┐業務課の担当に業務遂行を指示
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