// filename : adpModeBridge.cpp
//========================================================
// 経路アダプタ／サブマネージャ：ブリッジモード
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■ＭＭＰシステム
  #include "adp.h"
  //┴
//┴

//########################################################
//# 前空間：転送先経路
//########################################################
 namespace TRANS{
    // TCP RAW
    namespace TCP{
      bool begin() {return false;}
      bool send()  {return false;}
      bool end()   {return false;}
    } /* namespace TCP */

    // WEB Socket
    namespace WSOC{
      bool begin() {return false;}
      bool send()  {return false;}
      bool end()   {return false;}
    } /* namespace WSOC */

    // WEB Socket
    namespace WAPI{
      bool begin() {return false;}
      bool send()  {return false;}
      bool end()   {return false;}
    } /* namespace WAPI */

    // BLE
    namespace BLE{
      bool begin() {return false;}
      bool send()  {return false;}
      bool end()   {return false;}
    } /* namespace BLE */

    // ESP-NOW
    namespace ESPN{
      bool begin() {return false;}
      bool send()  {return false;}
      bool end()   {return false;}
    } /* namespace ESPN */

    // IIC
    namespace IIC{
      bool begin() {return false;}
      bool send()  {return false;}
      bool end()   {return false;}
    } /* namespace IIC */
} /* namespace TRANS */


//########################################################
//# 前空間：ブリッジモード
//########################################################
 namespace modeBridge{
//========================================================
//【非公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本処理
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // コンテクストの転送経路と接続
    //─────────────────
    bool CONN_BEGIN() {
      switch (ctx.transID) {
        case ADP_ID_TCP  : TRANS::TCP ::begin(); break;
        case ADP_ID_WSOC : TRANS::WSOC::begin(); break;
        case ADP_ID_WAPI : TRANS::WAPI::begin(); break;
        case ADP_ID_BLE  : TRANS::BLE ::begin(); break;
        case ADP_ID_ESPN : TRANS::ESPN::begin(); break;
        case ADP_ID_IIC  : TRANS::IIC ::begin(); break;
        default          : return true;
      }
      return false;
    }
    //─────────────────
    // コンテクストの転送経路にリクエストを送信
    //─────────────────
    bool CONN_SEND() {
      switch (ctx.transID) {
        case ADP_ID_TCP  : TRANS::TCP ::send(); break;
        case ADP_ID_WSOC : TRANS::WSOC::send(); break;
        case ADP_ID_WAPI : TRANS::WAPI::send(); break;
        case ADP_ID_BLE  : TRANS::BLE ::send(); break;
        case ADP_ID_ESPN : TRANS::ESPN::send(); break;
        case ADP_ID_IIC  : TRANS::IIC ::send(); break;
        default          : return true;
      }
      return false;
    }
    //─────────────────
    // 指定された経路を切断
    //─────────────────
    bool CONN_END(int argRID) {
      switch (argRID) {
        case ADP_ID_TCP  : TRANS::TCP ::end(); break;
        case ADP_ID_WSOC : TRANS::WSOC::end(); break;
        case ADP_ID_WAPI : TRANS::WAPI::end(); break;
        case ADP_ID_BLE  : TRANS::BLE ::end(); break;
        case ADP_ID_ESPN : TRANS::ESPN::end(); break;
        case ADP_ID_IIC  : TRANS::IIC ::end(); break;
        default          : return true;
      }
      return false;
    }

  //━━━━━━━━━━━━━━━━━
  // ブリッジ用コマンド
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // コマンド名／引数を取得
    //─────────────────
    void MAKE_COMMAND(String argCMD[], int arraySize){
      String strCMD = ctx.strFrame;
      strCMD.replace("!", "");
      int lastIndex = 0;
      for (int i = 0; i < arraySize; i++) {
        int index = strCMD.indexOf(':', lastIndex);
        if (index == -1) {
          argCMD[i] = strCMD.substring(lastIndex);
          break;
        }
        argCMD[i] = strCMD.substring(lastIndex, index);
        lastIndex = index + 1;
      }
    //┴
    } /* MAKE_COMMAND() */

    //─────────────────
    // 特殊コマンドに応答
    //----------------------------------
    // 戻り値：コマンド実行有無（論理値）
    // ・ 0：コマンド実行「なし」
    // ・ 1：コマンド実行「あり」
    // ・-1：コマンド実行「あり」、旧経路の切断に失敗
    // ・-2：コマンド実行「なし」、新経路の接続に失敗
    //─────────────────
    int BRIDGE_COMMAND() {
        //┬
        //○コマンド・引数を取得
        String cmd[4];
        MAKE_COMMAND(cmd, 4);
        //│
        //○コンテクストに引数を保存
        ctx.transDat1st = cmd[1];
        ctx.transDat2nd = cmd[2];
        ctx.transDat3rd = cmd[3];
        //│
        //○切断対象の経路IDを退避
        int tmpRID = ctx.transID;
        //│
        //○コンテクストに転送先を保存
        if      (cmd[0] == "SYS/BRIDGE/@TCP" ) {ctx.transID = ADP_ID_TCP  ;}
        else if (cmd[0] == "SYS/BRIDGE/@WSOC") {ctx.transID = ADP_ID_WSOC ;}
        else if (cmd[0] == "SYS/BRIDGE/@WAPI") {ctx.transID = ADP_ID_WAPI ;}
        else if (cmd[0] == "SYS/BRIDGE/@BLE" ) {ctx.transID = ADP_ID_BLE  ;}
        else if (cmd[0] == "SYS/BRIDGE/@ESPN") {ctx.transID = ADP_ID_ESPN ;}
        else if (cmd[0] == "SYS/BRIDGE/@IIC" ) {ctx.transID = ADP_ID_IIC  ;}
        else                                   {ctx.transID = -1; return 0;}
        //│
        //○以前の転送先を切断
        if (CONN_END(tmpRID)) return -1;
        //│
        //○新しい転送先に接続
        if (CONN_BEGIN()    ) return -2;
        //│
        //▼返却：コマンド実行あり
        return 1;
        //┴
    } /* BRIDGE_COMMAND() */


//========================================================
//【公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  // ブリッジモード
  //━━━━━━━━━━━━━━━━━
  void RUN(){
    //┬
    //●特殊コマンドに応答
    if (BRIDGE_COMMAND() != 0) return;
    //│＼（特殊コマンドを実行した場合）
    //│ ▼中断：早期リターン
    //│
    //◇┐[リクエストを転送]または[クライアントへレスポンス]
    if (ctx.adpID == ADP_ID_UART) CONN_SEND();
    //├┐（UARTアダプタから受信した場合）
        //●リクエストを転送
        //┴
    else Serial.print(ctx.strFrame);
    //└┐（その他）
        //○MMPレスポンスをクライアント(USB-CDC)へ送信
        //┴
    //┴
  } /* RUN() */

} /* namespace modeBridge */