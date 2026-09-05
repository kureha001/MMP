// filename : connection/common/normal.cpp
//========================================================
// 経路アダプタ／作業標準／一般処理：アダプタを統括する
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================

//########################################################
//# 前空間：一般処理
//########################################################
 namespace adpFnBase{
//========================================================
//【公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  // デバッグログ表示
  //━━━━━━━━━━━━━━━━━
  void SHOW_LOG(){
    if (!ctx.sysLog) return;
    Serial.println(String("\n======================================"));
    Serial.println(String("strFrame["   ) + String(ctx.strFrame) + String("]"));
    Serial.print  (String("authCD["     ) + String(ctx.authCD  ));
    Serial.println(String("]   cmdPath[") + String(ctx.cmdPath ) + String("]"));
    Serial.print  (String("accID["      ) + String(ctx.accID   ));
    Serial.println(String("]   accIDS[" ) + String(ctx.accIDS  ) + String("]"));
    Serial.println(String("resMSG["     ) + String(ctx.resMSG  ) + String("]"));
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