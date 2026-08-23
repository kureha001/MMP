// filename : adp.cpp
//========================================================
// アダプタ共通
//--------------------------------------------------------
//【目的】
// ・アダプタのトリガを実行する
// ・各プロセスの共通処理を提供する
//--------------------------------------------------------
//【アダプタのトリガ】
//・INIT_ADAPTER()：アダプタを初期化
//・KICK_HANDLE() ：ダプタをポーリング
//--------------------------------------------------------
//【共通資源】
//・SS_INI_SLOT_BASE()：接続管理スロット(ベース)を初期化する関数
//・F_SHOW_LOG()      ：デバッグ表示する関数
//・F0_SETUP()        ：コンテクストを初期化する関数
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/23) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <string.h>
  //│
  //■ＭＭＰシステム
  #include "adp.h"  // 通信アダプタ共通へ公開
  //┴
//┴

//========================================================
// 接続管理
//========================================================
    //─────────────────
    // スロット初期化
    //----------------------------------
    // 通信アダプタの名前空間で派生(名称:INIT_SLOT)
    //─────────────────
    void SS_INI_SLOT_BASE(SS_SLOT_TYPE& argSlot){
      argSlot.used   = false; // スロット有効性を「無効」
      argSlot.isOver = false; // 容量超過フラグを「OFF」
      argSlot.rx     = ""   ; // 受信バッファをクリア
      argSlot.rx.reserve(SS_RX_SIZE); // 容量確保
    } /* SS_INI_SLOT_BASE */

//========================================================
// 処理プロセス
//========================================================
  //━━━━━━━━━━━━━━━━━
  // デバッグログ表示
  //━━━━━━━━━━━━━━━━━
  void F_SHOW_LOG(String argMsg){
    Serial.println(String("======================================"));
    Serial.println(String("strFrame["   ) + String(ctx.strFrame) + String("]"));
    Serial.print  (String("authCD["     ) + String(ctx.authCD  ));
    Serial.println(String("]   cmdPath[") + String(ctx.cmdPath ) + String("]"));
    Serial.print  (String("routeID["    ) + String(ctx.routeID ));
    Serial.println(String("]   slotID[" ) + String(ctx.slotID  ) + String("]"));
    Serial.print  (String("accID["      ) + String(ctx.accID   ));
    Serial.println(String("]   accIDS[" ) + String(ctx.accIDS  ) + String("]"));
    Serial.println(String("vStream:"    ) + String(ctx.vStream.str()));
    Serial.println(String("======================================"));
  } /* F_SHOW_LOG() */

  //━━━━━━━━━━━━━━━━━
  // ポーリング ハンドル
  //─────────────────
  // 接続スロットごとに行う前処理
  //━━━━━━━━━━━━━━━━━
  void F0_SETUP(int argRID, int argSID){
    //----フレームデータ---
    ctx.vStream.clear()  ; // 仮想ストリーム
    ctx.strFrame = ""    ; // フレーム
    ctx.cmdPath  = ""    ; // コマンドパス
    ctx.authCD   = ""    ; // 認証コード
    //----グルーピング---
    ctx.routeID  = argRID; // 経路ID
    ctx.slotID   = argSID; // スロットID
    //----アクセスID---
    ctx.accID    = -1   ; // アクセスID
  } /* F0_SETUP() */

//========================================================
// サービス・アダプタの登録
//========================================================
  void INIT_ADAPTER() {
    //┬
    //○開始表示
    Serial.println("---------------------------");
    Serial.println("<<サービス・アダプタの初期化>>");
    //│
#if defined(MMP_TYPE_MAIN) //---┨ＭＭＰ本体┠----┐
    //●認証情報TBLを初期化
    AUTH_INIT_TBL();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_UART) //--┨UART通信┠----┐
    //●通信アダプタ
    adpUART::START();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_TCP   ) //--┨ＴＣＰ通信┠----┐
    adpTcp   ::START();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_HTTP  ) //--┨WebAPI通信┠----┐
    adpHttp  ::START();
#endif //-----------------------------------------┘
    //│    
#if defined(ADP_COM_BLE  ) //---┨ＢＬＥ通信┠----┐
    adpBLE   ::START();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_WEB      ) //---┨ＷＥＢ画面┠----┐
    //●ＷＥＢアダプタ
    adpWEB ::START();
#endif //-----------------------------------------┘
    //│
    //○終了表示
    Serial.println("");
    //┴
  } /* INIT_ADAPTER() */

//========================================================
// アダプタ起動のポーリング用ハンドル
//========================================================
  void KICK_HANDLE(){
    //┬
#if defined(ADP_COM_UART) //--┨UART通信┠----┐
    //●通信アダプタ（UART）
    adpUART::HANDLE();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_TCP   ) //--┨ＴＣＰ通信┠----┐
    //●通信アダプタ（TCPブリッジ）
    adpTcp   ::HANDLE();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_HTTP  ) //--┨WebAPI通信┠----┐
    //●通信アダプタ（WEB-API）
    adpHttp  ::HANDLE();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_BLE   ) //--┨ＢＬＥ通信┠----┐
    //●通信アダプタ（BLE）
    adpBLE   ::HANDLE();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_WEB       ) //--┨ＷＥＢ画面┠----┐
    //●ＷＥＢアダプタ
    adpWEB   ::HANDLE();
#endif //-----------------------------------------┘
    //┴
  } /* KICK_HANDLE() */