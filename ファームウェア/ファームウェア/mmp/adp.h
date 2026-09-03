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
  #include <queue>
  #include <mutex>
  //│
  //■ＭＭＰシステム
  #include "conf.h"
  #include "context.h"
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト
  //─────────────────
  extern MmpContext ctx;


//========================================================
// 経路アダプタ・マネージャ
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
// 共通部品
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 特殊コマンド名
  //━━━━━━━━━━━━━━━━━
    static const String SP_CMD_START = "_START_!"; // 認証コード発行

  //━━━━━━━━━━━━━━━━━
  // 接続情報：接続ごとの受信状態を管理
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

  //━━━━━━━━━━━━━━━━━
  // 通常プロセス
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // URI整形
    //─────────────────
    void FORMAT_URI(String &str);  // [adpBase][adpStream]で利用

    //─────────────────
    // 基本
    //─────────────────
    namespace adpBase{
      void RUN(String argAdpID, String argFrame);
      void SHOW_LOG();
    }

    //─────────────────
    // ストリーム処理
    //─────────────────
    namespace adpStream{
      void   SS_INI_SLOT_BASE(SS_SLOT_TYPE& argSlot);
      String GET_FRAME(Stream& argConn, SS_SLOT_TYPE argBASES);
    }

    //─────────────────
    // 認証処理
    //─────────────────
    namespace adpAUTH{
      void INIT_TBL(); // [adp]で利用
      bool CHECK()   ; // [adpBase]で利用
    }