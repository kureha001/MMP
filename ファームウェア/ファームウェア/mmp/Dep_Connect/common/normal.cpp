// filename : Dep_Connect/common/normal.cpp
//========================================================
// 接続部門／共通課：一般処理係
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
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
    if (!ctx.sysLog) return;
    Serial.println(String("\n======================================"));
    Serial.printf("Frame[%s]\n", ctx.strFrame);
    Serial.printf("AID[%d] -> TID[%d] : isTrans[%d]\n", ctx.adpID, ctx.transID, ctx.transOn);
    Serial.printf("TDat[%s][%s][%s]\n", ctx.transDat1st, ctx.transDat2nd, ctx.transDat3rd);
    Serial.printf("ACD[%s] : AccID[%d]/[%d]\n",ctx.authCD, ctx.accID, ctx.accIDS);
    Serial.printf("Path[%s] = MSG[%s]\n",ctx.cmdPath, ctx.resMSG );
    Serial.println(String("======================================"));
  } /* P9_SHOW_LOG() */

  //━━━━━━━━━━━━━━━━━
  // 文字列整形部品（URI形式）
  //━━━━━━━━━━━━━━━━━
  void FORMAT_URI(String &str){
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

} /* namespace adpFnBase */