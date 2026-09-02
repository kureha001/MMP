// filename : adp.cpp
//========================================================
// アダプタ共通：アダプタ全体のライフサイクルを統括する
//--------------------------------------------------------
//【目的】
// ・アダプタのトリガを実行する
// ・各プロセスの共通処理を提供する
//--------------------------------------------------------
// Ver 1.2.0 (2026/09/02) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■ＭＭＰシステム
  #include "adp.h"  // 通信アダプタ共通へ公開
  //│
  //■ＭＭＰシステム(アダプタ群)
  #include "adapter/UART.cpp"
  #include "adapter/TCP.cpp"
  #include "adapter/WEB_API.cpp"
  #include "adapter/WEB_Socket.cpp"
  #include "adapter/ESP_NOW.cpp"
  #include "adapter/BLE.cpp"
  #include "adapter/IIC.cpp"
  #include "adapter/WEB_Admin.cpp"
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源の所有（実体化）
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト
  //----------------------------------
  //・定義元：mmpCtx.h
  //・利用法：externで実体をアクセス
  //─────────────────
  MmpContext ctx;


#if defined(MMP_TYPE_MAIN) // ------------┨ＭＭＰ本体┠----┐
  //─────────────────
  // コマンド管理
  //----------------------------------
  //・定義元：cmd.h
  //・利用法：公開ポインタ経由でアクセス
  //─────────────────
  #include "cmd.h"                // 定義元ソースファイル
  CmdManager  OBJ_CMD(ctx)      ; // 本体(依存性注入)
  CmdManager* INO_CMD = &OBJ_CMD; // 外部公開ポインタ
#endif // --------------------------------------------------┘


//========================================================
// 共通部品
//========================================================
  //─────────────────
  // 受信バッファをURI形式に変換
  //----------------------------------
  //・先頭/末尾の不要文字を除去
  //─────────────────
  void FORMAT_URI(String &str){
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
  } /* FORMAT_URI() */


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
#if defined(MMP_TYPE_MAIN) //┨ＭＭＰ本体┠┐
    //●認証情報TBLを初期化
    adpAUTH::INIT_TBL();
#endif //----------------------------------┘
    //│
    //●通信アダプタを初期化
#if defined(ADP_COM_UART ) //----┨UART┠--┐
    adpUART::START();
#endif //----------------------------------┘
    //│
#if defined(ADP_COM_TCP  ) //----┨TcpRaw┠┐
    adpTCP ::START();
#endif //----------------------------------┘
    //│
#if defined(ADP_COM_WAPI ) //----┨WebAPI┠┐
    adpWAPI::START();
#endif //----------------------------------┘
    //│
#if defined(ADP_COM_WSOC ) //----┨WebSoc┠┐
    adpWSOC::START();
#endif //----------------------------------┘
    //│    
#if defined(ADP_COM_BLE  ) //----┨ＢＬＥ┠┐
    adpBLE ::START();
#endif //----------------------------------┘
    //│    
#if defined(ADP_COM_ESPN ) //---┨ESP-NOW┠┐
    adpESPN::START();
#endif //----------------------------------┘
    //│    
#if defined(ADP_COM_I2C ) //-----┨ＩＩＣ┠┐
    adpI2C ::START();
#endif //----------------------------------┘
    //│
    #if defined(ADP_WEB ) //-----┨ＷＥＢ┠┐
    adpWEB ::START();
#endif //----------------------------------┘
    //│
    //○終了表示
    Serial.println("");
    //│
#if defined(MMP_TYPE_MAIN) //┨ＭＭＰ本体┠┐
    //●コマンド管理を初期化
    INO_CMD->START();
#endif //----------------------------------┘
    //┴
  } /* INIT_ADAPTER() */

//========================================================
// アダプタ起動のポーリング用ハンドル
//========================================================
  void KICK_HANDLE(){
    //┬
    //●ハンドルをキック
#if defined(ADP_COM_UART) //---┨UART┠┐
    adpUART::HANDLE();
#endif //------------------------------┘
    //│
#if defined(ADP_COM_TCP ) //-┨TcpRaw┠┐
    adpTCP ::HANDLE();
#endif //------------------------------┘
    //│
#if defined(ADP_COM_WAPI) //-┨WebAPI┠┐
    adpWAPI::HANDLE();
#endif //------------------------------┘
    //│
#if defined(ADP_COM_WSOC) //-┨WebSoc┠┐
    adpWSOC::HANDLE();
#endif //------------------------------┘
    //│
#if defined(ADP_COM_BLE ) //-┨ＢＬＥ┠┐
    adpBLE ::HANDLE();
#endif //------------------------------┘
    //│
#if defined(ADP_COM_ESPN) //┨ESP-NOW┠┐
    adpESPN::HANDLE();
#endif //------------------------------┘
    //│
#if defined(ADP_COM_I2C ) //-┨ｉ２ｃ┠┐
    adpI2C ::HANDLE();
#endif //------------------------------┘
    //│
#if defined(ADP_WEB     ) //-┨ＷＥＢ┠┐
    adpWEB ::HANDLE();
#endif //------------------------------┘
    //┴
  } /* KICK_HANDLE() */