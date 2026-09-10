// filename : Dep_Connect/adapter/_template_/md_bridge.cpp
//========================================================
// 接続部門／業務課／作業標準：モード処理係（ブリッジモード）
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/11)
//========================================================

//########################################################
//# 処理詳細
//########################################################
 namespace modeBridge{
//========================================================
//【公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  // ブリッジモード
  //━━━━━━━━━━━━━━━━━
  void RUN(){
    //┬
    //○┐コマンド名と引数を取得
      //│
      //○フレームを補正
      String strCMD = ctx.strFrame;
      strCMD.replace("!", "");
      //│
      //◎┐フレームをコマンド名と引数に分解
      String cmd[4];
      int lastIndex = 0;
      for (int i = 0; i < 4; i++) {
        //│
        //○最後の値を格納
        int index = strCMD.indexOf(':', lastIndex);
        if (index == -1) {cmd[i] = strCMD.substring(lastIndex); break;}
        //│
        //○値を格納
        cmd[i] = strCMD.substring(lastIndex, index);
        lastIndex = index + 1;
        //┴
      } /* END-for */
      //┴
    //│
    //○転送先（スレーブ）をセット
    bool isOn = false;
    int  ID   = -1;
    if      (cmd[0] == "BRIDGE/UART") {isOn = true;} // マスタはエラーにする
    else if (cmd[0] == "BRIDGE/TCP" ) {isOn = true; if (ADP_TCP ) ID = ADP_ID_TCP ;}
    else if (cmd[0] == "BRIDGE/WSOC") {isOn = true; if (ADP_WSOC) ID = ADP_ID_WSOC;}
    else if (cmd[0] == "BRIDGE/HTTP") {isOn = true; if (ADP_HTTP) ID = ADP_ID_HTTP;}
    else if (cmd[0] == "BRIDGE/BLE" ) {isOn = true; if (ADP_BLE ) ID = ADP_ID_BLE ;}
    else if (cmd[0] == "BRIDGE/ESPN") {isOn = true; if (ADP_ESPN) ID = ADP_ID_ESPN;}
    else if (cmd[0] == "BRIDGE/IIC" ) {isOn = true; if (ADP_IIC ) ID = ADP_ID_IIC ;}
    if (isOn) ctx.bridge.adpID = ID; // 転送先の指定があれば変更
    //│
    //○転送先の設定漏れを確認
    if (ctx.bridge.adpID < 0) {ctx.resMSG = "#BR1!"; return;}
    //│＼（転送先が[未設定]の場合）
    //│ ○コンテクストにエラーCDをセット
    //│ ▼終了：早期リターン
    //│
    //○転送先設定／コマンド実行の確認
    if (isOn == false) return;
    //│＼（リクエストが[MMPコマンド実行]の場合）
    //│ ▼終了：早期リターン
    //│
    //○引数をレスポンスMSGへ反映
    ctx.bridge.Dat1 = cmd[1];
    ctx.bridge.Dat2 = cmd[2];
    ctx.bridge.Dat3 = cmd[3];
    //│
    //○コンテクストに[正常終了]をセット
    ctx.resMSG = "!!!!!";
    //┴
  } /* RUN() */

} /* namespace modeBridge */