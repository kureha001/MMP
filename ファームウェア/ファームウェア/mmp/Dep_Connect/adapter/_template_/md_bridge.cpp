// filename : Dep_Connect/adapter/_template_/md_bridge.cpp
//========================================================
// 接続部門／業務課／作業標準：モード処理係（ブリッジモード）
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/09) 
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
    //○転送先をコンテクストへ反映
    if      (cmd[0] == "BRIDGE/TCP" ) ctx.transID = ADP_ID_TCP ;
    else if (cmd[0] == "BRIDGE/WSOC") ctx.transID = ADP_ID_WSOC;
    else if (cmd[0] == "BRIDGE/WAPI") ctx.transID = ADP_ID_WAPI;
    else if (cmd[0] == "BRIDGE/BLE" ) ctx.transID = ADP_ID_BLE ;
    else if (cmd[0] == "BRIDGE/ESPN") ctx.transID = ADP_ID_ESPN;
    else if (ctx.transID < 0        ) {ctx.resMSG = "#TID!"; return;}
    else                              {ctx.transOn = true  ; return;}
    //│
    //○引数をコンテクストへ反映
    ctx.transDat1st = cmd[1];
    ctx.transDat2nd = cmd[2];
    ctx.transDat3rd = cmd[3];
    //┴
  } /* RUN() */

} /* namespace modeBridge */