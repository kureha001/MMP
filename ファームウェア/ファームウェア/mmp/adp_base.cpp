// filename : adp_func.cpp
//========================================================
// 通信アダプタ共通：基本プロセス
//--------------------------------------------------------
// Ver 1.2.0 (2026/09/02) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■ＭＭＰシステム
  #include "adp.h"
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
#if defined(MMP_TYPE_MAIN) // --┨ＭＭＰ本体┠----┐
  //─────────────────
  // コマンド管理
  //─────────────────
  #include "cmd.h" // RUN_COMMAND()
  extern String RUN_COMMAND(); // 所在：parser.h
#endif // ----------------------------------------┘

//########################################################
//# 前空間
//########################################################
namespace adpBase{
//========================================================
//【非公開機能】
//========================================================
  //─────────────────
  // フレームから第１トークンを取り出す
  //----------------------------------
  // 戻り値：第1トークン文字列
  //─────────────────
  String GET_TOK1(String &pURI){
    //┬
    //◇┐URIから切り出す
    String retStr = ""   ; // リクエストURI
    int    pos    = pURI.indexOf('/');
    if (pos >= 0) {
      //├┐（URIに"/"がある）
        //○URIから第１トークンを取得
        retStr  = pURI.substring(0, pos) ;
        //┴
    } else {
      //└┐（その他；URIに"/"が無い）
        //○URIから第１トークンを取得
        retStr = pURI;
        //┴
    }   /* END-if */
    //│
    //●切り出した文字列を整形
    FORMAT_URI(retStr); // 参照渡しなので内容は上書き
    //│
    //▼返却：整形済みトークン
    return retStr  ;
  } /* GET_TOK1() */

  //─────────────────
  // フレームから第2トークン以降を取り出す
  //----------------------------------
  // 戻り値：第２トークン以降の文字列
  //─────────────────
  String GET_TOK2(String &pURI){
    //┬
    //◇┐URIから切り出す
    String retStr = ""   ; // 戻り値
    int    pos    = pURI.indexOf('/');
    if (pos >= 0) {
      //├┐（URIに"/"がある）
        //○URIから第２トークン以降を取得
        retStr = pURI.substring(pos + 1);
        //┴
      //└┐
        //┴
    } /* END-if */
    //│
    //●切り出した文字列を整形
    FORMAT_URI(retStr); // 参照渡しなので内容は上書き
    //│
    //▼返却：整形済みトークン以降
    return retStr    ;
  } /* GET_TOK2() */

  //━━━━━━━━━━━━━━━━━
  // フレームから認証CDとコマンドパスをセット
  //━━━━━━━━━━━━━━━━━
  void SET_ACD_CPATH() {
    //┬
    //◇┐認証CDを取得
    String tmpFrame = ctx.strFrame;
    if (tmpFrame.startsWith("@")) {
      //├┐（認証コードの開始文字がある場合）
        //○先頭の'@'を削除
        //●第１トークンを[認証CD]にセット
        //●第２トークン以降を[コマンドパス]にセット
        tmpFrame.remove(0, 1);
        ctx.authCD  = GET_TOK1(tmpFrame);
        ctx.cmdPath = GET_TOK2(tmpFrame);
        //┴
    } else {
      //└┐
        //○コマンドパスにフレーム全体(認証コード無し)をセット
         ctx.cmdPath = tmpFrame;
        //┴
    //│
    //○コマンドパスを大文字に置換
    ctx.cmdPath.toUpperCase();
    } /* END-if */
    //┴
  } /* SET_ACD_CPATH() */

  //━━━━━━━━━━━━━━━━━
  // 接続スロットごとの前処理
  //━━━━━━━━━━━━━━━━━
  void SETUP(String argAdpID, String argFrame){

    ctx.adpID    = argAdpID; // アダプタID
    ctx.strFrame = argFrame; // フレーム
    FORMAT_URI(ctx.strFrame);

    ctx.vStream.clear(); // 仮想ストリーム
    ctx.resMSG   = ""  ; // レスポンスメッセージ
    ctx.cmdPath  = ""  ; // コマンドパス
    ctx.authCD   = ""  ; // 認証コード
    ctx.accID    = -1  ; // アクセスID
  } /* SETUP() */


//========================================================
//【公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  //【公開】MMPコマンドを実行
  //----------------------------------
  // MMPメイン／サブで機能が異なる
  //━━━━━━━━━━━━━━━━━
  void RUN(String argAdpID, String argFrame){
    //┬
    //●セットアップ
    SETUP(argAdpID, argFrame);
    //│

#if defined(MMP_TYPE_MAIN) // --┨ＭＭＰ本体┠----┐
    //○リクエストをデータ項目に分解
    SET_ACD_CPATH();
    //│
    //●認証処理を実施
    if (adpAUTH::CHECK()) return;
    //│＼（処理継続が不可の場合）
    //│ ▼終了：早期リターン
    //│
    //●ＭＭＰコマンドを実行
    ctx.resMSG = RUN_COMMAND();

#else // -----------------------┨ＭＭＰサブ┠----┤
    //○コマンドをMMP本体にUART送信
    Serial1.print(ctx.strFrame);
    //│
    //◎┐受信待ちデータの取り込み
    String strRX = "";
    while (!strRX.endsWith("!")) {
      //│＼（終端に達した場合）
      //│ ▽完了：走査終了
      //│
      //○受信データを受信バッファに加える
      if (Serial1.available()) strRX += (char)Serial1.read();
      //┴
    } /* END-while */
    //│
    //○ＭＭＰ本体からのレスポンス
    ctx.resMSG = strRX;
#endif // ----------------------------------------┘

    //┴
  } /* RUN() */

  //━━━━━━━━━━━━━━━━━
  // デバッグログ表示
  //━━━━━━━━━━━━━━━━━
  void P9_SHOW_LOG(){
    if (!ctx.sysLog) return;
    Serial.println(String("\n======================================"));
    Serial.println(String("strFrame["   ) + String(ctx.strFrame) + String("]"));
    Serial.print  (String("authCD["     ) + String(ctx.authCD  ));
    Serial.println(String("]   cmdPath[") + String(ctx.cmdPath ) + String("]"));
    Serial.print  (String("accID["      ) + String(ctx.accID   ));
    Serial.println(String("]   accIDS[" ) + String(ctx.accIDS  ) + String("]"));
    Serial.println(String("vStream:"    ) + String(ctx.vStream.str()));
    Serial.println(String("======================================"));
  } /* P9_SHOW_LOG() */

} /* namespace adpBase */