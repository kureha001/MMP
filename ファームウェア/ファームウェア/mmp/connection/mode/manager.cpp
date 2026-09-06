// filename : connection/mode/_manager_.cpp
//========================================================
// クライアント接続部門／動作モード：担当マネージャー
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//=============================================

//########################################################
//# 処理詳細
//########################################################
 namespace mode{
//========================================================
//【非公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  // キュー毎の前処理
  //━━━━━━━━━━━━━━━━━
  void SETUP(int argAdpID, String argFrame){

    ctx.adpID    = argAdpID; // アダプタID

    ctx.strFrame = argFrame; // フレーム
    if (!ctx.strFrame.endsWith ("!")) ctx.strFrame += "!";
    if (ctx.strFrame.startsWith("/")) ctx.strFrame.remove(0, 1);
    
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
    if (MODE == MODE_MAIN  ) modeMain  ::RUN();
    if (MODE == MODE_SUB   ) modeSub   ::RUN();
    if (MODE == MODE_BRIDGE) modeBridge::RUN();
    //┴
  } /* RUN() */
  } /* namespace mode */
