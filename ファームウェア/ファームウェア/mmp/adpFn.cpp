// filename : adpFn.cpp
//========================================================
// 経路アダプタ／作業標準／一般処理：アダプタを統括する
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■ＭＭＰシステム
  #include "adp.h"
  //┴
//┴

//########################################################
//# 前空間：一般処理
//########################################################
 namespace adpFnBase{
//========================================================
//【非公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  // キュー毎の前処理
  //━━━━━━━━━━━━━━━━━
  void SETUP(int argAdpID, String argFrame){

    ctx.adpID    = argAdpID; // アダプタID

    ctx.strFrame = argFrame; // フレーム
    FORMAT_URI(ctx.strFrame);
    if (!ctx.strFrame.endsWith("!")) ctx.strFrame += "!";

    ctx.resMSG   = ""  ; // レスポンスメッセージ
    ctx.cmdPath  = ""  ; // コマンドパス
    ctx.authCD   = ""  ; // 認証コード
    ctx.accID    = -1  ; // アクセスID
  } /* SETUP() */

//========================================================
//【公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  // MMPコマンドを実行
  //━━━━━━━━━━━━━━━━━
  void RUN(int argAdpID, String argFrame){
    //┬
    //●セットアップ
    SETUP(argAdpID, argFrame);
    //│
    //●モード別に後続処理
    if (ctx.sysMode == MODE_MAIN  ) modeMain  ::RUN();
    if (ctx.sysMode == MODE_SUB   ) modeSub   ::RUN();
    if (ctx.sysMode == MODE_BRIDGE) modeBridge::RUN();
    //┴
  } /* RUN() */

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