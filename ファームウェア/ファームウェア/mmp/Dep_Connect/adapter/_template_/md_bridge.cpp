// filename : Dep_Connect/adapter/_template_/md_bridge.cpp
//========================================================
// 接続部門／業務課／作業標準：モード処理係（ブリッジモード）
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/10)
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
    //○転送先をレスポンスMSGへ反映
    bool isOn = false;
    if      (cmd[0] == "BRIDGE/TCP" ) {ctx.bridge.adpID = ADP_ID_TCP ; isOn = true;}
    else if (cmd[0] == "BRIDGE/WSOC") {ctx.bridge.adpID = ADP_ID_WSOC; isOn = true;}
    else if (cmd[0] == "BRIDGE/HTTP") {ctx.bridge.adpID = ADP_ID_HTTP; isOn = true;}
    else if (cmd[0] == "BRIDGE/BLE" ) {ctx.bridge.adpID = ADP_ID_BLE ; isOn = true;}
    else if (cmd[0] == "BRIDGE/ESPN") {ctx.bridge.adpID = ADP_ID_ESPN; isOn = true;}
    //│
    //○転送先の設定漏れを確認
    if (ctx.bridge.adpID < 0) {ctx.resMSG = "#TID!"; return;}
    //│＼（未設定の場合）
    //│ ○レスポンスMSGにエラーIDをセット
    //│ ▼終了：早期リターン
    //│
    //○転送先設定／コマンド実行の確認
    if (isOn == false  ) return;
    //│＼（コマンド実行の場合）
    //│ ▼終了：早期リターン
    //│
    //○引数をレスポンスMSGへ反映
    ctx.bridge.Dat1 = cmd[1];
    ctx.bridge.Dat2 = cmd[2];
    ctx.bridge.Dat3 = cmd[3];
    //│
    //○レスポンスMSGに[正常終了]をセット
    ctx.resMSG = "!!!!!";
    //┴
  } /* RUN() */

} /* namespace modeBridge */