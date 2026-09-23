// filename : Dep_Connect/template/md_sub.cpp
//========================================================
// 接続部門／業務課／作業標準：モード処理係（サブモード）
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/21)
//========================================================

//########################################################
//# 処理詳細
//########################################################
 namespace modeSub{
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

//========================================================
//【公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  // サブモード
  //━━━━━━━━━━━━━━━━━
  void RUN(){
    //┬
    //●システムコマンドに応答
    if (Sys_Command()) return;
    //│＼（応答した場合）
    //│ ▼終了：早期リターン
    //│
    //○リクエストをMMPメインへ転送
    Log::Outln("(1/3) Requested to MMP(MAIN).");
    Serial1.print(ctx.strFrame);
    //│
    //◎┐受信待ちデータの取り込み
    Log::Outln("(2/3) Reading from MMP(MAIN).");
    String strRX = "";
    unsigned long startTime = millis();
    while (!strRX.endsWith("!")) {
      //│＼（終端に達した場合）
      //│ ▽完了：走査終了
      //│
      //○経過時間を確認
      if (millis() - startTime > LIMIT::TIME_READ) {
      //│＼（タイムアウトした場合）
          //○レスポンスMSGにエラーCDを反映
          //▼終了：早期リターン
          ctx.resMSG = RCD::TimOut;
          Log::Outln("(3/3) Error:Response timeout from MMP(MAIN).");
          return;
      } /* END-if */
      //│
      //○受信データを受信バッファに加える
      if (Serial1.available()) strRX += (char)Serial1.read();
      //┴
    } /* END-while */
    //│
    //○レスポンスMSGに[MMP本体からのレスポンス]を反映
    Log::Outln("(3/3) Success.");
    ctx.resMSG = strRX;
    //┴
  } /* RUN() */
} /* namespace modeSub */