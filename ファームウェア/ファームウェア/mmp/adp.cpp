// filename : adp.cpp
//========================================================
// 通信アダプタ共通
//--------------------------------------------------------
// 2026/08/21 : 大幅にリファクタリング
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
      argSlot.used       = false    ; // スロット有効性をクリア
      argSlot.rx         = ""       ; // 受信バッファをクリア
      argSlot.rx.reserve(SS_RX_SIZE); // 受信バッファ容量を事前確保
      argSlot.isOverflow = false    ; // 容量超過フラグをクリア
    } /* SS_INI_SLOT_BASE */

//========================================================
// 処理プロセス
//========================================================
  //━━━━━━━━━━━━━━━━━
  // ポーリング ハンドル
  //─────────────────
  // 接続スロットごとに行う前処理
  //━━━━━━━━━━━━━━━━━
  void SETUP_CTX(int argRID, int argSID){
    // ---フレームデータ---
    ctx.strFrame = ""    ; // フレーム
    ctx.cmdPath  = ""    ; // コマンドパス
    ctx.authCD   = ""    ; // 認証コード
    // ---グルーピング---
    ctx.routeID  = argRID; // 経路ID
    ctx.slotID   = argSID; // スロットID
    // ---アクセスID---
    ctx.accID    = -1   ; // アクセスID
  } /* SETUP_CTX() */

  //━━━━━━━━━━━━━━━━━
  // デバッグログ表示
  //━━━━━━━━━━━━━━━━━
  void LOG_PRINT(String argMsg){
    Serial.println(String("\n======================================"));
    Serial.println(String("strFrame["   ) + String(ctx.strFrame) + String("]"));
    Serial.print  (String("authCD["     ) + String(ctx.authCD  ));
    Serial.println(String("]   cmdPath[") + String(ctx.cmdPath ) + String("]"));
    Serial.print  (String("routeID["    ) + String(ctx.routeID ));
    Serial.println(String("]   slotID[" ) + String(ctx.slotID  ) + String("]"));
    Serial.print  (String("accID["      ) + String(ctx.accID   ));
    Serial.println(String("]   accIDS[" ) + String(ctx.accIDS  ) + String("]"));
    Serial.println(String("vStream:"    ) + String(ctx.vStream.str()));
    Serial.println(String("--------------------------------------"));
    Serial.println(String(argMsg));
    Serial.println(String("======================================\n"));
  } /* LOG_PRINT() */


//========================================================
// サービス・アダプタの登録
//========================================================
  void InitAdapter() {
    //┬
    //○開始表示
    Serial.println("---------------------------");
    Serial.println("<<サービス・アダプタの初期化>>");
    //│
#if defined(MMP_TYPE_MAIN) // --┨ＭＭＰ本体┠----┐
    //●認証情報TBLを初期化
    AUTH_INIT_TBL();
#endif // ----------------------------------------┘
    //│
    //●通信アダプタ
    adpSerial::start();
    //│
// -----------------┨ＴＣＰ通信｜WebAPI通信┠----┐
#if defined(ADP_COM_TCP) || defined(ADP_COM_HTTP)
    adpTcp   ::start();
    adpHttp  ::start();
#endif // ----------------------------------------┘
    //│    
#if defined(ADP_COM_BLE  ) // --┨ＢＬＥ通信┨----┐
    adpBLE   ::start();
#endif // ----------------------------------------┘
    //│
#if defined(ADP_WEB      ) // --┨ＷＥＢ画面┠----┐
    //●ＷＥＢアダプタ
    adpWEB ::start();
#endif // ----------------------------------------┘
    //│
    //○終了表示
    Serial.println("");
    //┴
  } /* InitAdapter() */


 //========================================================
// アダプタ起動のポーリング用ハンドル
//========================================================
  void kickHandle(){
    //┬
    //●通信アダプタ（シリアル）
    adpSerial::handle();
    //│
#if defined(ADP_COM_TCP  ) // --┨ＴＣＰ通信┠----┐
    //●通信アダプタ（TCPブリッジ）
    adpTcp   ::handle();
#endif // ----------------------------------------┘
    //│
#if defined(ADP_COM_HTTP ) // --┨WebAPI通信┠----┐
    //●通信アダプタ（WEB-API）
    adpHttp  ::handle();
#endif // ----------------------------------------┘
    //│
#if defined(ADP_WEB      ) // --┨ＷＥＢ画面┠----┐
    //●ＷＥＢアダプタ
    adpWEB   ::handle();
#endif // ----------------------------------------┘
    //┴
  } /* kickHandle() */