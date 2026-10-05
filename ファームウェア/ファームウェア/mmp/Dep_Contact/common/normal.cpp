// filename : Dep_Contact/common/normal.cpp
//========================================================
// 接客部門／庶務課：一般処理係
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/30)
//========================================================

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 処理詳細
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 namespace adpFnBase{
//========================================================
//§公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  // デバッグログ表示
  //━━━━━━━━━━━━━━━━━
  void LOG_CTX(){

    if (!Log::ENABLE) return;
    char msg[128];
    Log::prtln(String("\n============== MMP LOG ==============="));

    Log::prtln("Frame [" + String(ctx.base.Frame) + "]");

    snprintf(
      msg, sizeof(msg),
      "AID[%d] T:SID[%d] T:AID[%d](T:Stat[%d])",
      ctx.base.AID, ctx.trans.SID, ctx.trans.AID, ctx.trans.Stat
    ); Log::prtln(String(msg));

    snprintf(
      msg, sizeof(msg),
      "TDat[%s][%s][%s]",
      String(ctx.trans.Dat1), String(ctx.trans.Dat2), String(ctx.trans.Dat3)
    ); Log::prtln(String(msg));

    snprintf(
      msg, sizeof(msg),
      "ACD[%s] : AccID[%d]/[%d]",
      String(ctx.access.CD), ctx.access.ID, ctx.access.IDS
    ); Log::prtln(String(msg));

    Log::prtln("Path[" + String(ctx.base.CmdPath) + "] = MSG[" + String(ctx.base.Result ) + "]");

    Log::prtln(String("======================================"));
  } /* LOG_CTX() */

  //━━━━━━━━━━━━━━━━━
  // 文字列整形部品（URI形式）
  //━━━━━━━━━━━━━━━━━
  void FORMAT_URI(String &str){
    str.toUpperCase();

    while (str.length() > 0) {
      char c = str.charAt(0);
      if (c=='/'||c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\0')
      {str.remove(0, 1);} else {break;}
    } //～while

    while (str.length() > 0) {
      char c = str.charAt(str.length() - 1);
      if (c=='/'||c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\0')
      {str.remove(str.length()-1);} else {break;}
    } //～while
  } /* FORMAT_URI() */

  //━━━━━━━━━━━━━━━━━
  // コンテキストを初期化
  //━━━━━━━━━━━━━━━━━
  void SETUP_CTX(
    int    argAID,
    int    argSID,
    String argFrame
  ) {
    ctx.base.AID   = argAID  ; // 通信アダプタID
    ctx.base.SID   = argSID  ; // 接続スロットID
    ctx.base.Frame = argFrame; // フレーム
    if (!ctx.base.Frame.endsWith  ("!")) ctx.base.Frame += "!";
    if ( ctx.base.Frame.startsWith("/")) ctx.base.Frame.remove(0, 1);
    ctx.base.Result  = ""    ; // 処理結果
    ctx.base.CmdPath = ""    ; // コマンドパス
    ctx.access.CD    = ""    ; // 認証コード
    ctx.access.ID    = -1    ; // アクセスID
  } /* FORMAT_URI() */

  //─────────────────
  // システムコマンドに応答
  //----------------------------------
  //【戻り値】
  // 応答の有無(論理値型)
  //  true ：あり
  //  false：なし
  //─────────────────
  bool SysCmd(String argFrame) {
    //┬
    //○┐【前処理】
      //●フレームを整形
      adpFnBase::FORMAT_URI(argFrame);
      //│
      //○コマンドを確認
      if (!argFrame.startsWith("SYS/")) return false;
      //│＼（[システム以外]の場合）
      //│ ▼終了：早期リターン（なし）
      //┴
    //│
    //○┐【主処理】
      //●製造部門に後続処理を移譲する
      ctx.base.CmdPath = argFrame; // コマンドパスをセット
      DepProduct::HANDLE(); // 実行結果は[ctx.base.Result]にセットされる
      //┴
    //│
    //○┐【後処理】
      //▼終了：リターン（あり）
      return true;
    //┴
  } /* SysCmd() */

} /* namespace adpFnBase */