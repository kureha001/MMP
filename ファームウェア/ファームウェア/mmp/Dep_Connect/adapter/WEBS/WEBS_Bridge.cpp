// filename : Dep_Connect/adapter/WEBS/WEBS_Bridge.cpp
//========================================================
// 接続部門／担当：WEB Socket（ブリッジモード）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/28)
//========================================================

//========================================================
//§開始処理・終了処理
//========================================================
  //───────────────────────────
  // 開始処理：コンストラクタ
  //───────────────────────────
  String CONSTRACT() override final {
    //┬
    //▼：返却：起動ログ表示のMSG
    return String(" [OK] WEB Socket");
    //┴
  } /* CONSTRACT() */

  //───────────────────────────
  // 終了処理：スレーブにリクエストを送信する
  //───────────────────────────
  void SEND_REQUEST() override final {
  //┬
  //○┐【前処理】
    //●初期化の健全性を確認する
    if (SETUP_NORMAL()) {ctx.trans.Msg = RCD::Trn1Err; return;}
    //│＼（問題がある場合）
    //│ ○完了MSGにエラーCDをセット
    //│ ▼終了：早期リターンする
    //│
    //○宛先情報を用意する
    String toIP = ctx.trans.Dat1;
    //┴
  //│
  //○┐【主処理】
    //◇┐クライアント処理を開始する
    if (!MY_NET.isConnected()) {
      //├┐（WebSocketの状態が[未接続]の場合）
        //○並列処理を開始する
        MY_TASK  = this;
        MY_NET.onEvent(ON_RECIVE); 
        //│
        //○WebSocketを接続する
        MY_NET.begin(toIP.c_str(), MY_PORT, "/");
        //│
        //◎┐接続が完了するまで待つ
        unsigned long startTime = millis();
        while(!MY_NET.isConnected() && ((millis() - startTime) < LIMIT::TIME_CONNECT))
        {delay(200); MY_NET.loop();}
          //│＼（[接続OK][タイムアウト]いずれかの場合）
          //│ ▽完了：リトライを終了する
          //│
          //○少し待ちながら処理を進める
          //┴
        //│
        //○接続状態を確認する
        if (!MY_NET.isConnected()) {ctx.trans.Msg = RCD::Trn2Err; return;}
        //│＼（状態が[未接続]の場合）
        //│ ○完了MSGにエラーCDをセット
        //│ ▼終了：早期リターンする
        //┴
      //└┐（その他）
        //┴
    } //～if 
    //│
    //○この通信アダプタにリクエストを送信する
    MY_NET.sendTXT(ctx.trans.Frame);
    //┴
  //│
  //○┐【後処理】
    //●ログ出力
    adpFnBase::SHOW_LOG();
  //┴┴
  } /* SEND_REQUEST() */

//========================================================
//§ハンドルの事前処理と進行判定
//========================================================
  //───────────────────────────
  // 一般用：追加事項 ※SETUP_NORMAL()に包括される
  //------------------------------------------------------
  //【戻り値】進行判定
  // true ：進行NG
  // false：進行OK
  //───────────────────────────
  bool SETUP_OPTION() override final {
  //┬
  //○┐【前処理】
    //○WebSocetの接続状況を確認する
    if (!MY_NET.isConnected()) {
      Log::prtln("[ERROR] WEB Socketが未接続です。");
      return true;
    } //～if
    //│＼（機能していない場合）
    //│ ▼終了：早期リターンする(進行NG)
    //┴
  //│
  //○┐【主処理】
    //○WebSocketの処理を進める
    MY_NET.loop();
    //┴
  //│
  //○┐【後処理】
    //▼返却：正常終了（進行OK）
    return false;
  //┴
  } /* SETUP_OPTION() */
