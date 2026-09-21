// filename : Dep_Connect/adapter/_template_/md_bridge.cpp
//========================================================
// 接続部門／業務課／作業標準：モード処理係（ブリッジモード）
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/21)
// ・ブリッジの初期化を共通部品化
// ・ブリッジの不具合対応
// ・システムコマンド部をRun_SysCmd()へ分離
// ・[AdapterQueueBase]転送処理の共通部を部品化
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <functional>
//┴┴

//########################################################
//# 処理詳細
//########################################################
 namespace modeBridge{
//========================================================
//§共有資源
//========================================================
  //─────────────────
  // 転送パラメータ
  //─────────────────
  String DAT[4];

//========================================================
//§非公開機能
//========================================================
  //─────────────────
  // 内部コマンドに応答
  //----------------------------------
  //【戻り値】
  // 応答の有無(論理値型)
  //  true ：あり
  //  false：なし
  //─────────────────
  bool Run_SysCmd() {
    //┬
    //○┐【前処理】
      //●フレームを整形
      adpFnBase::FORMAT_URI(ctx.bridge.Frame);
      //│
      //○コマンドを確認
      if (!ctx.bridge.Frame.startsWith("SYS/")) return false;
      //│＼（[システム以外]の場合）
      //│ ▼終了：早期リターン（なし）
      //┴
    //│
    //○┐【主処理】
      //●コマンドを実行
      ctx.cmdPath = ctx.bridge.Frame; // コマンドパスをセット
      DepCommand::RunCommand()      ; // 実行結果は[ctx.resMSG]にセットされる
      //┴
    //│
    //○┐【後処理】
      //▼終了：リターン（あり）
      return true;
    //┴
  } /* Run_SysCmd() */

  //─────────────────
  // コマンド名と引数を取得
  //─────────────────
  void Run_MakeData() {
    //┬
    //○┐【前処理】
      //○フレームを補正
      String strCMD = ctx.bridge.Frame;
      strCMD.replace("!", "");
      //┴
    //│
    //○┐【主処理】
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
      } /* for */
      //┴
    //│
    //○┐【後処理】
      //○（処理なし）
      //┴
    //┴
  } /* Run_MakeData() */

  //─────────────────
  // 転送先を切替
  //----------------------------------
  //【戻り値】
  // 転送先変更の有無(論理値型)
  //  true ：あり
  //  false：なし
  //─────────────────
  bool Run_TransRoute() {
    //┬
    //○┐【前処理】
      //●コマンド名と引数を取得
      Run_MakeData();
      //┴
    //│
    //○┐【主処理】
      //○転送先をセット
      bool isOn = false;
      int  ID   = -1;
      if      (DAT[0] == "BRIDGE/UART") {isOn = true;} // マスタはエラーにする
      else if (DAT[0] == "BRIDGE/TCP" ) {isOn = true; if (ADP_TCP ) ID = ADP_ID_TCP ;}
      else if (DAT[0] == "BRIDGE/WSOC") {isOn = true; if (ADP_WSOC) ID = ADP_ID_WSOC;}
      else if (DAT[0] == "BRIDGE/HTTP") {isOn = true; if (ADP_HTTP) ID = ADP_ID_HTTP;}
      else if (DAT[0] == "BRIDGE/BLE" ) {isOn = true; if (ADP_BLE ) ID = ADP_ID_BLE ;}
      else if (DAT[0] == "BRIDGE/ESPN") {isOn = true; if (ADP_ESPN) ID = ADP_ID_ESPN;}
      else if (DAT[0] == "BRIDGE/IIC" ) {isOn = true; if (ADP_IIC ) ID = ADP_ID_IIC ;}
      //│
      //○転送先を変更
      if (isOn) ctx.bridge.adpID = ID;
      //┴
    //│
    //○┐【後処理】
      //▼返却：正常終了（転送先変更の有無）
      return isOn;
    //┴
  } /* Run_TransRoute() */


//========================================================
//§公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  // ブリッジモード実行
  //━━━━━━━━━━━━━━━━━
  void RUN(int argSID, String argFrame){
    //┬
    //○┐【前処理】
      //○コンテクスト(ブリッジ用)を初期化
      ctx.bridge.slotID = argSID  ; //スロットIDを退避
      ctx.bridge.Frame  = argFrame; //フレームを退避
      ctx.bridge.MSG    = ""      ; //完了MSGをクリア
      //┴
    //│
    //○┐【主処理】
      //○┐内部コマンドに応答
        //●内部コマンドに応答(内部処理)
        if (Run_SysCmd()) return;
        //│＼（[あり]の場合）
        //│ ▼終了：早期リターン
        //│
        //●転送先を切替
        bool isTrans = Run_TransRoute();
        //┴
      //│
      //○┐現状を評価
        //○転送先を確認
        if (ctx.bridge.adpID < 0) {ctx.resMSG = RCD::Trn1Err; return;}
        //│＼（[未設定]の場合）
        //│ ○レスポンスMSGにエラーCDをセット
        //│ ▼終了：早期リターン
        //│
        //○転送先変更の有無を確認
        if (isTrans == false) return;
        //│＼（[あり]の場合）
        //│ ▼終了：早期リターン
        //┴
      //│
      //○転送先パラメータをセット
      ctx.bridge.Dat1 = DAT[1];
      ctx.bridge.Dat2 = DAT[2];
      ctx.bridge.Dat3 = DAT[3];
      //┴
    //│
    //○┐【後処理】
      //○レスポンスMSGに[正常終了]をセット
      ctx.resMSG = RCD::OK;
      //┴
    //┴
  } /* RUN() */

  //━━━━━━━━━━━━━━━━━
  // 前処理（マスタ用）
  //【引数】
  // ・レスポンス関数（ポインタ）
  //━━━━━━━━━━━━━━━━━
  bool MASTER(
    Stream*                      argConn,    // 実際に送信するストリーム
    std::function<void(Stream*)> argSendConn // 送信処理の関数
  ) {
    //┬
    //○┐【前処理】
      //●マスタとして進行判定
      switch (ctx.bridge.Stat) {
        case BSTAT::IDLE: return false; // 待機中➡○リクエスト受付
        case BSTAT::REQ : return true ; // 依頼済➡×
        case BSTAT::BUSY: return true ; // 処理中➡×
        case BSTAT::DONE: break       ; // 処理済は後続処理へ
        default         : return true ; // 想定外➡×
      } /* switch */
      //┴
    //│
    //○┐【主処理】
      //●クライアントにレスポンスト
      argSendConn(argConn);
      //│
      //○進行状況を[待機中]に遷移
      Log::Outln("4.処理済→待機中");
      ctx.bridge.Stat = BSTAT::IDLE;
      //┴
    //│
    //○┐【後処理】
      //▼返却：正常終了(進行OK)
      return false;
      //┴
    //┴
  } /* MASTER() */

  //━━━━━━━━━━━━━━━━━
  // 前処理（スレーブ用）
  //----------------------------------
  //【引数】
  // ・アダプタID
  // ・転送関数（ポインタ）
  //━━━━━━━━━━━━━━━━━
  bool SLAVE(
    int                   argAID,  // アダプタID
    std::function<void()> argTrans // 転送関数（ポインタ）
  ) {
    //┬
    //○┐【前処理】
      //○スレーブを確認
      if (argAID != ctx.bridge.adpID) return true;
      //│＼（スレーブではない場合）
      //│ ▼終了：早期リターン（進行NG)
      //│
      //○進捗状況による進行判定
      switch (ctx.bridge.Stat) {
        case BSTAT::IDLE: return true ; // 待機中➡×
        case BSTAT::REQ : break       ; // 依頼済は後続処理へ
        case BSTAT::BUSY: return false; // 処理中➡○キュー応答
        case BSTAT::DONE: return true ; // 処理済➡×
        default         : return true ; // 想定外➡×
      } /* switch */
      //┴
    //│
    //○┐【主処理】
      //○進行状況を[処理中]に遷移
      Log::Outln("2.依頼済→処理中");
      ctx.bridge.Stat = BSTAT::BUSY;
      //│
      //●転送を実施
      argTrans();
      //│
      //◇┐即時応答に対応
      if (ctx.bridge.MSG != "") {
        //├┐（完了MSGが[内容あり]の場合）
          //○マスタが処理できるようレスポンスMSGへ反映
          //○進行状況を[処理済]にセット
          Log::Outln("3.処理中→処理済(即時)");
          ctx.resMSG      = ctx.bridge.MSG;
          ctx.bridge.Stat = BSTAT::DONE;
          //┴
        //└┐（その他）
          //┴
      } /* if */
    //│
    //○┐【後処理】
      //▼返却：正常終了(進行NG)
      return true;
    //┴
  } /* SLAVE() */

} /* namespace modeBridge */