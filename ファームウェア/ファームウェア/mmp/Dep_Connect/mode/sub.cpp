// filename : Dep_Connect/mode/sub.cpp
//========================================================
// 接続部門／処理手順：サブモード
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/30)
//========================================================

 namespace modeSub{
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
  bool Sys_Command() {
    //┬
    //●Sysコマンドを実行
    //▼終了：処理結果
    return adpFnBase::SysCmd(ctx.base.Frame);
    //┴
  }

//========================================================
//§公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  // サブモード実行
  //━━━━━━━━━━━━━━━━━
  void RUN(){
    //┬
    //●システムコマンドに応答
    if (Sys_Command()) return;
    //│＼（応答した場合）
    //│ ▼終了：早期リターン
    //│
    //○リクエストをMMPメインへ送信する
    Log::Outln("(1/3) Requested to MMP(MAIN).");
    Serial1.print(ctx.base.Frame);
    //│
    //◎┐受信待ちデータを取込む
    Log::Outln("(2/3) Reading from MMP(MAIN).");
    String strRX = "";
    unsigned long startTime = millis();
    while (!strRX.endsWith("!")) {
      //│＼（フレーム終端に達した場合）
      //│ ▽完了：走査を完了する
      //│
      //○経過時間を確認
      if (millis() - startTime > LIMIT::TIME_READ) {
      //│＼（タイムアウトした場合）
          //○処理結果にエラーCDをセットする
          //▼終了：早期リターンする
          ctx.base.Result = RCD::TimOut;
          Log::Outln("(3/3) Error:Response timeout from MMP(MAIN).");
          return;
      } //～if
      //│
      //○受信データをバッファに付け足す
      if (Serial1.available()) strRX += (char)Serial1.read();
      //┴
    } //～while
    //│
    //○処理結果に[MMP本体からのレスポンス]をセットする
    Log::Outln("(3/3) Success.");
    ctx.base.Result = strRX;
    //┴
  } /* RUN() */

} /* namespace modeSub */