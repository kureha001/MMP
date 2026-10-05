// filename : Dep_Contact/mode/sub.cpp
//========================================================
// 接客部門／業務手順書（モード別処理）：サブ
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/30)
//========================================================

 namespace modeSub{
//========================================================
//§公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  // サブモード実行
  //━━━━━━━━━━━━━━━━━
  void HANDLE(){
    //┬
    //●システム系コマンドに応答
    if (adpFnBase::SysCmd(ctx.base.Frame)) return;
    //│＼（応答が[あり]の場合）
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
  } /* HANDLE() */

} /* namespace modeSub */