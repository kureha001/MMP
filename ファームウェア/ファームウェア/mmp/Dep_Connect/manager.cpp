// filename : Dep_Connect/manager.cpp
//========================================================
// 接続部門：部門長
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/16)
// ・UARTポートの制限を見直し 
// ・プリプロセッサ判定を整理
// ・スロット構造体を削除
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
  inline constexpr int ADP_ID_HTTP = 2;
  inline constexpr int ADP_ID_WSOC = 3;
  inline constexpr int ADP_ID_BLE  = 4;
  inline constexpr int ADP_ID_ESPN = 5;
  inline constexpr int ADP_ID_IIC  = 6;
  //│
  //□ブリッジの進捗状況
  namespace BSTAT {
    inline constexpr int IDLE = 0; // 待機中
    inline constexpr int REQ  = 1; // 依頼中（マスタ→スレーブ）
    inline constexpr int BUSY = 2; // 処理中（スレーブ実行中）
    inline constexpr int DONE = 3; // 処理済（応答・完了）
  }
//┴┴

//========================================================
// 組織図
//========================================================
//┬
//□┐接続部門
  //□共通課
  #include "common/_index_.h"
  //│
  //□┐業務課
    //│
    //□作業標準：抽象基底クラス、モード部品
    #include "adapter/_template_/_index_.h"
    //│
    //□担当
    #include "adapter/member/_index_.h" 
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
          //○TCP担当
          #if ADP_TCP
          ADAPTER.push_back(new AdapterTCP(ctx));
          #endif
          //│
          //○HTTP担当
          #if ADP_HTTP
          ADAPTER.push_back(new AdapterHTTP(ctx));
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
      //○┐個別係
        //│
        //○UART担当(メイン・ブリッジは必須、サブは任意)
        #if ADP_UART || MODE == MODE_MAIN || MODE == MODE_BRIDGE
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