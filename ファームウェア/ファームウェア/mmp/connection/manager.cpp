// filename : connection/manager.cpp
//========================================================
// クライアント接続部門：統括マネージャ
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
//┬
//□┐クライアント接続部門
  //□統括マネージャ
  #include "_index_.h"
//│┴
//│
//□通信デバイス部門 ※他部門と情報連携
#include "../device.h"
//┴

//########################################################
//# 処理詳細
//########################################################
namespace ConnectionManager{
  //─────────────────
  // 経路アダプタ管理
  //─────────────────
    //┬
    //□コンテナを用意
    std::vector<AdapterBase*> ADAPTER;
    //┴

//========================================================
// 公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 活動に必要な資源を初期化
  //━━━━━━━━━━━━━━━━━
  void INIT() {
    //┬
    //○メッセージ表示を開始
    Serial.println("<<経路アダプタの初期化>>");
    //│
    //○ユーザ認証の初期化
      adpFnAuth::INIT_TBL();
    //│
    //●経路アダプタを初期化
    #if (MODE!=MODE_MAIN) || ADP_UART //※メインモード以外は強制使用
      if (devUART::ENABLED) ADAPTER.push_back(new AdapterUART(ctx));
    #endif

    if (devWiFi::ENABLED) {
    #if ADP_TCP
      ADAPTER.push_back(new AdapterTCP(ctx));
    #endif
    #if ADP_WAPI
      ADAPTER.push_back(new AdapterWEB_API(ctx));
    #endif
    #if ADP_WSOC
      ADAPTER.push_back(new AdapterWEB_Socket(ctx));
    #endif
    #if ADP_ESPN
      ADAPTER.push_back(new AdapterESPNOW(ctx));
    #endif
    }

    #if ADP_BLE
      if (devBLE::ENABLED) ADAPTER.push_back(new AdapterBLE(ctx));
    #endif

    #if (MODE != MODE_MAIN) && ADP_IIC //※メインモードは使用不可
      if (devIIC::ENABLED) ADAPTER.push_back(new AdapterIIC(ctx));
    #endif
    //│
    //○メッセージ表示を終了
    Serial.println("");
    //┴
  } /* INIT_ADAPTER() */

  //━━━━━━━━━━━━━━━━━
  // 通常活動を実施
  //━━━━━━━━━━━━━━━━━
  void WORK(){
    //◎担当(経路アダプタ)にリクエスト処理を指示
    for (auto* adp : ADAPTER) if (adp) adp->handle();
  } /* WORK() */

} /* namespace ConnectionManager */