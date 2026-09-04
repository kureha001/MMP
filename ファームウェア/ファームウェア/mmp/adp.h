// filename : adp.h
//========================================================
// アダプタ・マネージャ：アダプタを統括する
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <Arduino.h>
  //│
  //■ＭＭＰシステム
  #include "conf.h"    // 初期設定
  #include "context.h" // コンテクスト
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト
  // ※_API_.h より前に宣言が必要
  //─────────────────
  extern MmpContext ctx;

  //─────────────────
  // 経路アダプタID
  //─────────────────
  const int ADP_ID_UART = 0;
  const int ADP_ID_TCP  = 1;
  const int ADP_ID_WAPI = 2;
  const int ADP_ID_WSOC = 3;
  const int ADP_ID_BLE  = 4;
  const int ADP_ID_ESPN = 5;
  const int ADP_ID_IIC  = 6;


//========================================================
// 統括マネージャ
//========================================================
namespace AdapterManager{
  void INIT()  ; // 初期化
  void HANDLE(); // ハンドルをキック
}

//========================================================
// メンバー（経路アダプタ）
//========================================================
  //─────────────────
  // UART
  //─────────────────
  namespace adpUART{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //─────────────────
  // TCP RAW
  //─────────────────
  namespace adpTCP{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //─────────────────
  // WEB API
  //─────────────────
  namespace adpWAPI{     
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //─────────────────
  // WEB Socket
  //─────────────────
  namespace adpWSOC{     
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //─────────────────
  // BLE
  //─────────────────
  namespace adpBLE{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //─────────────────
  // ESP NOW
  //─────────────────
  namespace adpESPN{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

  //─────────────────
  // IIC
  //─────────────────
  namespace adpI2C{
    void START()  ; // サービス開始の指示
    void HANDLE() ; // ポーリングのハンドル
  }

//========================================================
// サブマネージャ（動作モード）
//========================================================
  namespace modeMain  {void RUN();} // メインモード
  namespace modeSub   {void RUN();} // サブモード
  namespace modeBridge{void RUN();} //ブリッジモード


//========================================================
// 作業標準
//========================================================
namespace adpFnBase{
  //━━━━━━━━━━━━━━━━━
  // 一般処理：ユーザ認証
  //━━━━━━━━━━━━━━━━━
    void FORMAT_URI(String &str);  // [adpFnBase][adpFnStream]で利用
    void RUN(int argAdpID, String argFrame);
    void SHOW_LOG();
  }

  //━━━━━━━━━━━━━━━━━
  // 専門処理：ユーザ認証
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // 特殊コマンド名
    //─────────────────
    static const String SP_CMD_START = "_START_!"; // 認証コード発行

    //─────────────────
    // 実行部品
    //─────────────────
    namespace adpFnAuth{
      void INIT_TBL(); // [AdapterManager]で利用
      bool CHECK()   ; // [adpFnBase]で利用
    }

  //━━━━━━━━━━━━━━━━━
  // 専門処理：ストリーム受信
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // 基本情報
    //─────────────────
    static const int SS_RX_SIZE = 128 ; // 受信バッファ容量

    //─────────────────
    // 構造体
    //─────────────────
    struct SS_SLOT_TYPE {
      bool    used   = false ; // スロット有効性
      String  rx     = ""    ; // 受信バッファ
      bool    isOver = false ; // 容量超過フラグ
    };

    //─────────────────
    // 実行部品
    //─────────────────
    namespace adpFnStream{
      void   SS_INI_SLOT_BASE(SS_SLOT_TYPE& argSlot);
      String GET_FRAME(Stream& argConn, SS_SLOT_TYPE argBASES);
    }