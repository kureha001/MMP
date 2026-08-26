// filename : adp.cpp
//========================================================
// アダプタ共通：アダプタ全体のライフサイクルを統括する
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
//・P0_SETUP_CONTEXT()        ：コンテクストを初期化する関数
//・P1_FORMAT_URI()   ：受信バッファをURI形式に変換
//・P9_SHOW_LOG()      ：デバッグ表示する関数
//--------------------------------------------------------
// Ver 1.2.0 (2026/08/25) 
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

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト
  //----------------------------------
  //・定義元：mmpCtx.h
  //・利用法：externで実体をアクセス
  //・用途等：
  //  - 通信アダプタ  ：受信データ・処理状態の共有領域
  //  - コマンド管理  ：コマンド実行情報の共有領域
  //  - 機能モジュール：レスポンス生成領域、ユーザメモリ管理
  //─────────────────
  MmpContext ctx;

 #if defined(MMP_TYPE_MAIN) // --┨ＭＭＰ本体┠----┐
  //─────────────────
  // コマンド管理
  //----------------------------------
  //・定義元：cmd.h
  //・利用法：公開ポインタ経由でアクセス
  //・用途等：
  //  - 通信アダプタ  ：コマンド管理への移譲
  //  - コマンド管理  ：定義元自身
  //─────────────────
  #include "cmd.h"                // 定義元ソースファイル
  CmdManager  OBJ_CMD(ctx)      ; // 本体(依存性注入)
  CmdManager* INO_CMD = &OBJ_CMD; // 外部公開ポインタ
#endif // ----------------------------------------┘

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
  // ポーリング ハンドル
  //─────────────────
  // 接続スロットごとに行う前処理
  //━━━━━━━━━━━━━━━━━
  void P0_SETUP_CONTEXT(String argAdpID, String argFrame){

    ctx.adpID    = argAdpID; // アダプタID
    ctx.strFrame = argFrame; // フレーム
    P1_FORMAT_URI(ctx.strFrame);

    ctx.vStream.clear(); // 仮想ストリーム
    ctx.resMSG   = ""  ; // レスポンスメッセージ
    ctx.cmdPath  = ""  ; // コマンドパス
    ctx.authCD   = ""  ; // 認証コード
    ctx.accID    = -1  ; // アクセスID
  } /* P0_SETUP_CONTEXT() */

  //─────────────────
  // 受信バッファをURI形式に変換
  //----------------------------------
  //・先頭/末尾の不要文字を除去
  //─────────────────
  void P1_FORMAT_URI(String &str){
    //┬
    //○先頭の不要な文字をすべて削除
    while (str.length() > 0) {
      char c = str.charAt(0);
      if (c=='/'||c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\0')
      {str.remove(0, 1);} else {break;}
      } /* END-while */ 
    //│
    //○末尾の不要な文字をすべて削除
    while (str.length() > 0) {
      char c = str.charAt(str.length() - 1);
      if (c=='/'||c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\0')
      {str.remove(str.length()-1);} else {break;}
      } /* END-while */ 
    //┴
  } /* P1_FORMAT_URI() */

  //━━━━━━━━━━━━━━━━━
  // デバッグログ表示
  //━━━━━━━━━━━━━━━━━
  void P9_SHOW_LOG(){
    if (ctx.sysLog < 0) return;
    Serial.println(String("\n======================================"));
    Serial.println(String("strFrame["   ) + String(ctx.strFrame) + String("]"));
    Serial.print  (String("authCD["     ) + String(ctx.authCD  ));
    Serial.println(String("]   cmdPath[") + String(ctx.cmdPath ) + String("]"));
    Serial.print  (String("accID["      ) + String(ctx.accID   ));
    Serial.println(String("]   accIDS[" ) + String(ctx.accIDS  ) + String("]"));
    Serial.println(String("vStream:"    ) + String(ctx.vStream.str()));
    Serial.println(String("======================================"));
  } /* P9_SHOW_LOG() */


//========================================================
// アダプタの初期化
//--------------------------------------------------------
// 認証TBL・アダプタ・コマンド管理を開始
//========================================================
  void INIT_ADAPTER() {
    //┬
    //○開始表示
    Serial.println("<<アダプタの初期化>>");
    //│
#if defined(MMP_TYPE_MAIN) //---┨ＭＭＰ本体┠----┐
    //●認証情報TBLを初期化
    AUTH_INIT_TBL();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_UART) //----┨UART通信┠------┐
    //●通信アダプタ
    adpUART::START();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_TCP   ) //--┨TcpRaw通信┠----┐
    adpTCP ::START();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_WAPI  ) //--┨WebAPI通信┠----┐
    adpWAPI::START();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_WSOC  ) //--┨WebSoc通信┠----┐
    adpWSOC::START();
#endif //-----------------------------------------┘
    //│    
#if defined(ADP_COM_BLE  ) //---┨ＢＬＥ通信┠----┐
    adpBLE ::START();
#endif //-----------------------------------------┘
    //│    
#if defined(ADP_COM_ESPN  ) //--┨ESP-NOW通信┠---┐
    adpESPN::START();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_WEB      ) //---┨ＷＥＢ画面┠----┐
    adpWEB ::START();
#endif //-----------------------------------------┘
    //│
    //○終了表示
    Serial.println("");
    //│
#if defined(MMP_TYPE_MAIN) //---┨ＭＭＰ本体┠----┐
    INO_CMD->START();
#endif //-----------------------------------------┘
    //┴
  } /* INIT_ADAPTER() */

//========================================================
// アダプタ起動のポーリング用ハンドル
//========================================================
  void KICK_HANDLE(){
    //┬
#if defined(ADP_COM_UART) //--┨UART通信┠----┐
    adpUART::HANDLE();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_TCP   ) //--┨TcpRaw通信┠----┐
    adpTCP ::HANDLE();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_WAPI  ) //--┨WebAPI通信┠----┐
    adpWAPI::HANDLE();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_WSOC  ) //--┨WebSoc通信┠----┐
    adpWSOC::HANDLE();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_BLE   ) //--┨ＢＬＥ通信┠----┐
    adpBLE ::HANDLE();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_COM_ESPN  ) //--┨ESP-NOW通信┠---┐
    adpESPN::HANDLE();
#endif //-----------------------------------------┘
    //│
#if defined(ADP_WEB       ) //--┨ＷＥＢ画面┠----┐
    adpWEB ::HANDLE();
#endif //-----------------------------------------┘
    //┴
  } /* KICK_HANDLE() */