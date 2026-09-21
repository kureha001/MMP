// filename : Dep_Connect/common/normal.cpp
//========================================================
// 接続部門／共通課：一般処理係
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/21)
//========================================================

//########################################################
//# 処理詳細
//########################################################
 namespace adpFnBase{
//========================================================
// 担務（公開機能）
//========================================================
  //━━━━━━━━━━━━━━━━━
  // デバッグログ表示
  //━━━━━━━━━━━━━━━━━
  void SHOW_LOG(){

    if (!Log::ENABLE) return;
    char msg[128];
    Log::prtln(String("\n============== MMP LOG ==============="));

    Log::prtln("Frame [" + String(ctx.strFrame) + "]");

    snprintf(
      msg, sizeof(msg),
      "AID[%d] SID[%d] TID[%d](Stat[%d])",
      ctx.adpID, ctx.bridge.slotID, ctx.bridge.adpID, ctx.bridge.Stat
    ); Log::prtln(String(msg));

    snprintf(
      msg, sizeof(msg),
      "TDat[%s][%s][%s]",
      String(ctx.bridge.Dat1), String(ctx.bridge.Dat2), String(ctx.bridge.Dat3)
    ); Log::prtln(String(msg));

    snprintf(
      msg, sizeof(msg),
      "ACD[%s] : AccID[%d]/[%d]",
      String(ctx.authCD), ctx.accID, ctx.accIDS
    ); Log::prtln(String(msg));

    Log::prtln("Path[" + String(ctx.cmdPath) + "] = MSG[" + String(ctx.resMSG ) + "]");

    Log::prtln(String("======================================"));
  } /* SHOW_LOG() */

  //━━━━━━━━━━━━━━━━━
  // 文字列整形部品（URI形式）
  //━━━━━━━━━━━━━━━━━
  void FORMAT_URI(String &str){
    str.toUpperCase();
    while (str.length() > 0) {
      char c = str.charAt(0);
      if (c=='/'||c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\0')
      {str.remove(0, 1);} else {break;}
    } /* END-if */
    while (str.length() > 0) {
      char c = str.charAt(str.length() - 1);
      if (c=='/'||c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\0')
      {str.remove(str.length()-1);} else {break;}
    } /* END-if */
  } /* FORMAT_URI() */

  //━━━━━━━━━━━━━━━━━
  // コンテキストを初期化
  //━━━━━━━━━━━━━━━━━
  void SETUP_CTX(int argAID, String argFrame) {
    ctx.adpID    = argAID; // アダプタID
    ctx.strFrame = argFrame; // フレーム
    if (!ctx.strFrame.endsWith ("!")) ctx.strFrame += "!";
    if (ctx.strFrame.startsWith("/")) ctx.strFrame.remove(0, 1);
    ctx.resMSG  = ""  ; // レスポンスMSG
    ctx.cmdPath = ""  ; // コマンドパス
    ctx.authCD  = ""  ; // 認証コード
    ctx.accID   = -1  ; // アクセスID
  } /* FORMAT_URI() */

} /* namespace adpFnBase */