// filename : Dep_Connect/adapter/_template_/md_bridge.cpp
//========================================================
// 接続部門／業務課／作業標準：モード処理係（ブリッジモード）
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/14)
// ・システムコマンド部をSys_Command()へ分離
// ・[AdapterQueueBase]転送処理の共通部を部品化
//========================================================

//########################################################
//# 処理詳細
//########################################################
 namespace modeBridge{
  String DAT[4];

//========================================================
//【非公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  // システムコマンドに応答
  //━━━━━━━━━━━━━━━━━
  bool Sys_Command() {
    //┬
    //●フレームを整形
    String tmpFrame = ctx.strFrame;
    adpFnBase::FORMAT_URI(tmpFrame);
    //│
    //○システムコマンドであるかを確認
    if (!tmpFrame.startsWith("SYS/")) return false;
    //│＼（システムコマンドの場合）
    //│ ▼終了：早期リターン（進行OK）
    //│
    //●コマンドを実行
    ctx.cmdPath = tmpFrame  ; // コマンド部門への準備
    DepCommand::RunCommand(); // コマンド実行結果はctx.resMSGにセット
    //│
    //▼終了：リターン（進行NG）
    return true;
  }

  //━━━━━━━━━━━━━━━━━
  // コマンド名と引数を取得
  //━━━━━━━━━━━━━━━━━
  void Make_Data() {
    //┬
    //○フレームを補正
    String strCMD = ctx.strFrame;
    strCMD.replace("!", "");
      //│
    //◎┐フレームをコマンド名と引数に分解
    int lastIndex = 0;
    for (int i = 0; i < 4; i++) {
      //│
      //○最後の値を格納
      int index = strCMD.indexOf(':', lastIndex);
      if (index == -1) {DAT[i] = strCMD.substring(lastIndex); break;}
      //│
      //○値を格納
      DAT[i] = strCMD.substring(lastIndex, index);
      lastIndex = index + 1;
      //┴
    } /* END-for */
    //┴
  }

  //━━━━━━━━━━━━━━━━━
  // 転送先をセット
  //━━━━━━━━━━━━━━━━━
  bool Trans_Route() {
    //┬
    //●コマンド名と引数を取得
    Make_Data();
    //│
    //○転送先（スレーブ）をセット
    bool isOn = false;
    int  ID   = -1;
    if      (DAT[0] == "BRIDGE/UART") {isOn = true;} // マスタはエラーにする
    else if (DAT[0] == "BRIDGE/TCP" ) {isOn = true; if (ADP_TCP ) ID = ADP_ID_TCP ;}
    else if (DAT[0] == "BRIDGE/WSOC") {isOn = true; if (ADP_WSOC) ID = ADP_ID_WSOC;}
    else if (DAT[0] == "BRIDGE/HTTP") {isOn = true; if (ADP_HTTP) ID = ADP_ID_HTTP;}
    else if (DAT[0] == "BRIDGE/BLE" ) {isOn = true; if (ADP_BLE ) ID = ADP_ID_BLE ;}
    else if (DAT[0] == "BRIDGE/ESPN") {isOn = true; if (ADP_ESPN) ID = ADP_ID_ESPN;}
    else if (DAT[0] == "BRIDGE/IIC" ) {isOn = true; if (ADP_IIC ) ID = ADP_ID_IIC ;}
    if (isOn) ctx.bridge.adpID = ID; // 転送先の指定があれば変更
    //│
    return isOn;
    //┴
  }

//========================================================
//【公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  // ブリッジモード
  //━━━━━━━━━━━━━━━━━
  void RUN(){
    //┬
    //●前処理
    if (Sys_Command()) return;
    //│＼（応答した場合）
    //│ ▼終了：早期リターン
    //│
    //●転送先をセット
    bool isTrans = Trans_Route();
    //│
    //○転送先の設定漏れを確認
    if (ctx.bridge.adpID < 0) {ctx.resMSG = "#BR1!"; return;}
    //│＼（転送先が[未設定]の場合）
    //│ ○コンテクストにエラーCDをセット
    //│ ▼終了：早期リターン
    //│
    //○転送先設定／コマンド実行の確認
    if (isTrans == false) return;
    //│＼（リクエストが[MMPコマンド実行]の場合）
    //│ ▼終了：早期リターン
    //│
    //○引数をレスポンスMSGへ反映
    ctx.bridge.Dat1 = DAT[1];
    ctx.bridge.Dat2 = DAT[2];
    ctx.bridge.Dat3 = DAT[3];
    //│
    //○コンテクストに[正常終了]をセット
    ctx.resMSG = "!!!!!";
    //┴
  } /* RUN() */

  //━━━━━━━━━━━━━━━━━
  // 転送処理を開始
  //━━━━━━━━━━━━━━━━━
  bool TARNS_BEGIN(int argAID) {
    //┬
    //○転送依頼を確認
    if (ctx.bridge.Stat != BSTAT::REQ || argAID != ctx.bridge.adpID) return true;
    //│＼（自分宛に転送依頼がない場合）
    //│ ▼終了：早期リターン
    //│
    //○進行状況を[処理中]にセット
    ctx.bridge.Stat = BSTAT::BUSY;
    //│
    //▼終了：正常終了
    return false;
    //┴
  } /* TARNS_BEGIN() */

  //━━━━━━━━━━━━━━━━━
  // 転送処理を終了
  //━━━━━━━━━━━━━━━━━
  void TARNS_END() {
    //┬
      //○処理結果を確認
    if (ctx.strFrame != "") ctx.bridge.Stat = BSTAT::DONE;
    //│＼（レスポンスが得られた場合）
    //│ ○進行状況を[処理済]にセット
    //│ ┴
    //┴
  } /* TARNS_END() */

} /* namespace modeBridge */