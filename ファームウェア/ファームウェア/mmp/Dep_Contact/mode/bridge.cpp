// filename : Dep_Contact/mode/bridge.cpp
//========================================================
// 接客部門／業務手順書（モード別処理）：ブリッジ
//--------------------------------------------------------
// Ver 1.4.0 (2026/10/01)
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <functional> // 転送関数（ポインタ）で利用
//┴┴

 namespace modeBridge{
//========================================================
//§非公開機能
//========================================================
  //─────────────────
  // 転送パラメータ
  //─────────────────
  String DAT[4];

  //─────────────────
  // コマンド名と引数を取得
  //─────────────────
  void makeData() {
    //┬
    //○┐【前処理】
      //○フレームを補正
      String strCMD = ctx.trans.Frame;
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
    //┴┴
  } /* makeData() */

  //─────────────────
  // 転送先を切替
  //----------------------------------
  //【戻り値】
  // 転送先変更の有無(論理値型)
  //  true ：あり
  //  false：なし
  //─────────────────
  bool setTransRoute() {
    //┬
    //○┐【前処理】
      //●コマンド名と引数を取得
      makeData();
      //┴
    //│
    //○┐【主処理】
      //○転送先をセット
      bool isOn = false;
      int  ID   = -1;
      if      (DAT[0] == "BRIDGE/UART") {isOn = true;} // マスタはエラーにする
      else if (DAT[0] == "BRIDGE/UDP" ) {isOn = true; if (ADP_UDP ) ID = AID::UDP ;}
      else if (DAT[0] == "BRIDGE/TCP" ) {isOn = true; if (ADP_TCP ) ID = AID::TCP ;}
      else if (DAT[0] == "BRIDGE/WSOC") {isOn = true; if (ADP_WSOC) ID = AID::WSOC;}
      else if (DAT[0] == "BRIDGE/HTTP") {isOn = true; if (ADP_HTTP) ID = AID::HTTP;}
      else if (DAT[0] == "BRIDGE/ESPN") {isOn = true; if (ADP_ESPN) ID = AID::ESPN;}
      else if (DAT[0] == "BRIDGE/BLE" ) {isOn = true; if (ADP_BLE ) ID = AID::BLE ;}
      else if (DAT[0] == "BRIDGE/IIC" ) {isOn = true; if (ADP_IIC ) ID = AID::IIC ;}
      //│
      //○転送先を変更
      if (isOn) ctx.trans.AID = ID;
      //┴
    //│
    //○┐【後処理】
      //▼返却：正常終了（転送先変更の有無）
      return isOn;
    //┴
  } /* setTransRoute() */


//========================================================
//§公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // ブリッジモード実行
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  void HANDLE(){
    //┬
    //○┐【前処理】
      //○コンテクスト(ブリッジ用)を初期化
      ctx.trans.Frame  = ctx.base.Frame; //フレームを退避
      ctx.trans.Result = ""            ; //処理結果をクリア
      //┴
    //│
    //○┐【主処理】
      //○┐内部コマンドに応答
        //●システム系コマンドに応答
        if (adpFnBase::SysCmd(ctx.trans.Frame)) return;
        //│＼（応答が[あり]の場合）
        //│ ▼終了：早期リターン
        //│
        //●転送先を切替
        bool isTrans = setTransRoute();
        //┴
      //│
      //○┐現状を評価
        //○転送先を確認
        if (ctx.trans.AID < 0)
        {ctx.base.Result = RCD::Trn1Err; return;}
        //│＼（[未設定]の場合）
        //│ ○処理結果にエラーCDをセット
        //│ ▼終了：早期リターン
        //│
        //○転送先変更の有無を確認
        if (isTrans == false) return;
        //│＼（[あり]の場合）
        //│ ▼終了：早期リターン
        //┴
      //│
      //○転送先パラメータをセット
      ctx.trans.Dat1 = DAT[1];
      ctx.trans.Dat2 = DAT[2];
      ctx.trans.Dat3 = DAT[3];
      //┴
    //│
    //○┐【後処理】
      //○処理結果に[正常終了]をセット
      ctx.base.Result = RCD::OK;
    //┴┴
  } /* HANDLE() */

  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // 進行状況管理
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
    //──────────────────────────
    // 進行状況を進捗
    //----------------------------------------------------
    //【引数】
    // ・進捗状況ID
    // ・ログメッセージのオプション
    //──────────────────────────
    void MOVE_BSTAT(
      int argBSTAT, // 進捗状況ID
      String argMSG // ログメッセージのオプション
    ) {
      //┬
      //○┐【前処理】
        //◇┐ログメッセージを用意する
        String msg = "";
        switch (argBSTAT) {
        case BSTAT::IDLE: msg = "4.処理済→待機中"; break;
        case BSTAT::REQ : msg = "1.待機中→依頼中"; break;
        case BSTAT::BUSY: msg = "2.依頼済→処理中"; break;
        case BSTAT::DONE: msg = "3.処理中→処理済"; break;
        default : {Log::prtln("[ERROR] MOVE_BSTAT()"); return;}
        } //～switch
        //│
        //○ログメッセージにオプションを追加する
        if (argMSG != "") msg += "(" + argMSG + ")";
        //┴
      //│
      //○┐【主処理】
        //○進行状況を更新する
        ctx.trans.Stat = argBSTAT;
        //│
        //○処理結果にフレーム内容をセットする
        if (argBSTAT == BSTAT::REQ ) ctx.trans.SID   = ctx.base.SID; 
        if (argBSTAT == BSTAT::DONE) ctx.base.Result = ctx.base.Frame;

        //┴
      //│
      //○┐【後処理】
        //●ログを出力する
        Log::Outln(msg);
      //┴┴
    } /* MOVE_BSTAT() */

    //──────────────────────────
    // 前処理（マスタ用）
    //----------------------------------------------------
    //【引数】
    // ・レスポンス関数（ポインタ）
    //──────────────────────────
    bool MASTER(
      Stream*                      argConn,      // 実際に送信するストリーム
      std::function<void(Stream*)> argSendResult // 送信関数（ポインタ）
    ) {
      //┬
      //○┐【前処理】
        //○進行判定を求める
        switch (ctx.trans.Stat) {
        case BSTAT::IDLE: return false; // 待機中➡○リクエスト受付
        case BSTAT::REQ : return true ; // 依頼済➡×
        case BSTAT::BUSY: return true ; // 処理中➡×
        case BSTAT::DONE: break       ; // 処理済は後続処理へ
        default         : return true ; // 想定外➡×
        } //～switch
        //│＼（[処理済]以外の場合）
        //│ ▼終了：早期リターン（求めた進行判定）
        //┴
      //│
      //○┐【主処理】
        //●接続元に処理結果を送信する
        argSendResult(argConn);
        //│
        //●進行状況を[待機中]に進捗する
        MOVE_BSTAT(BSTAT::IDLE, "");
        //┴
      //│
      //○┐【後処理】
        //▼返却：正常終了(進行OK)
        return false;
      //┴
    } /* MASTER() */

    //──────────────────────────
    // 前処理（スレーブ用）
    //----------------------------------------------------
    //【引数】
    // ・通信アダプタID
    // ・転送関数（ポインタ）
    //──────────────────────────
    bool SLAVE(
      int                   argAID,        // アダプタID
      std::function<void()> argSendRequest // 送信関数（ポインタ）
    ) {
      //┬
      //○┐【前処理】
        //○スレーブであるかを確認する
        if (argAID != ctx.trans.AID) return true;
        //│＼（スレーブの通信アダプタIDと異なる場合）
        //│ ▼終了：早期リターン（進行NG)
        //│
        //○進行判定を求める
        switch (ctx.trans.Stat) {
        case BSTAT::IDLE: return true ; // 待機中➡×
        case BSTAT::REQ : break       ; // 依頼済は後続処理へ
        case BSTAT::BUSY: return false; // 処理中➡○キュー応答
        case BSTAT::DONE: return true ; // 処理済➡×
        default         : return true ; // 想定外➡×
        } //～switch
        //│＼（[依頼済]以外の場合）
        //│ ▼終了：早期リターン（求めた進行判定）
        //┴
      //│
      //○┐【主処理】
        //●進行状況を[処理中]に進捗する
        MOVE_BSTAT(BSTAT::BUSY, "");
        //│
        //●スレーブにリクエストを送信する
        argSendRequest();
        //│
        //◇┐即時応答の通信アダプタに対応
        if (ctx.trans.Result != "") {
          //├┐（処理結果がある場合）
            //●進行状況を[処理済]に進捗する
            MOVE_BSTAT(BSTAT::DONE, "即時応答");
            //┴
          //└┐（その他）
            //┴
        } //～if
        //┴
      //│
      //○┐【後処理】
        //▼返却：正常終了(進行NG)
        return true;
      //┴
    } /* SLAVE() */

} /* namespace modeBridge */