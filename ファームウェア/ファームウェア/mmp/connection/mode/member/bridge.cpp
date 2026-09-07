// filename : connection/mode/member/bridge.cpp
//========================================================
// クライアント接続部門／動作モード／担当：ブリッジモード
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//□┐クライアント接続部門
  //□┐統括マネージャ
    //□┐担当マネージャ
      //□┐担当：ブリッジモード
        //□アダプター
        #include "bridge_adapter/_index_.h"
//┴┴┴┴┴


//########################################################
//# 処理詳細
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
    void CONN_BEGIN() {

      int      ip4  = 0;
      uint16_t port = 0;

      switch (ctx.transID) {

        //【TCP RAW】
        case ADP_ID_TCP:{
          ip4  = ctx.transDat1st.toInt();
          port = (uint16_t)ctx.transDat2nd.toInt();
          brdTCP::BEGIN(ip4, port); 
          break;
        }

        //case ADP_ID_WSOC : brdWSOC::BEGIN(); break;
        //case ADP_ID_WAPI : brdWAPI::BEGIN(); break;
        //case ADP_ID_BLE  : brdBLE ::BEGIN(); break;

        //【ESP-NOW】
        case ADP_ID_ESPN:{
          brdESPN::BEGIN(ctx.transDat1st);
          break;
        }

        default: ctx.resMSG = "#ERB!";
      }
    }
    //─────────────────
    // コンテクストの転送経路にリクエストを送信
    //─────────────────
    void CONN_SEND() {
      switch (ctx.transID) {
        case ADP_ID_TCP  : brdTCP ::SEND(); break;
        //case ADP_ID_WSOC : brdWSOC::SEND(); break;
        //case ADP_ID_WAPI : brdWAPI::SEND(); break;
        //case ADP_ID_BLE  : brdBLE ::SEND(); break;
        case ADP_ID_ESPN : brdESPN::SEND(); break;
        default          : ctx.resMSG = "#ERS!";
      }
    }
    //─────────────────
    // 指定された経路を切断
    //─────────────────
    void CONN_END(int argRID) {
      //○経路IDが有効化を確認
      if (argRID < 0) return;
      switch (argRID) {
        case ADP_ID_TCP  : brdTCP ::END(); break;
        //case ADP_ID_WSOC : brdWSOC::END(); break;
        //case ADP_ID_WAPI : brdWAPI::END(); break;
        //case ADP_ID_BLE  : brdBLE ::END(); break;
        case ADP_ID_ESPN : brdESPN::END(); break;
        default          : ctx.resMSG = "#ERE!";
      }
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
      ctx.accID = 0;
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
        if      (cmd[0] == "BRIDGE/TCP" ) {ctx.transID = ADP_ID_TCP  ;}
        //else if (cmd[0] == "BRIDGE/WSOC") {ctx.transID = ADP_ID_WSOC ;}
        //else if (cmd[0] == "BRIDGE/WAPI") {ctx.transID = ADP_ID_WAPI ;}
        //else if (cmd[0] == "BRIDGE/BLE" ) {ctx.transID = ADP_ID_BLE  ;}
        else if (cmd[0] == "BRIDGE/ESPN") {ctx.transID = ADP_ID_ESPN ;}
        else return 0; // ※特殊コマンドではないので早期リターン
        //│
        //○以前の転送先を切断
        CONN_END(tmpRID);
        if (ctx.resMSG != "") return 1;
        //│
        //○新しい転送先に接続
        CONN_BEGIN();
        if (ctx.resMSG != "") return 2;
        //│
        //▼返却：コマンド実行あり
        return 3;
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
    int ret = BRIDGE_COMMAND();
    if (ret != 0) return;
    //│＼（特殊コマンドを実行した場合）
    //│ ▼中断：早期リターン
    //│
    if (ctx.adpID == ADP_ID_UART) CONN_SEND();
    else Serial.print(ctx.strFrame);
    //◇┐[リクエストを転送]または[クライアントへレスポンス]
      //├┐（UARTアダプタから受信した場合）
      //│●リクエストを転送実行
      //│┴
      //└┐（その他）
        //○受信フレームをクライアント(USB-CDC)へレスポンス
        //┴
    //┴
  } /* RUN() */

} /* namespace modeBridge */