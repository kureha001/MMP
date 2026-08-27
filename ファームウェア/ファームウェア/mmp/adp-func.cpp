// filename : adp-func.cpp
//========================================================
// 通信アダプタ共通：処理プロセス用
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/23) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■ＭＭＰシステム
  #include "adp.h"  // 通信アダプタ共通へ公開
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

//========================================================
// 処理プロセス用の部品
//========================================================
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

  //━━━━━━━━━━━━━━━━━
  // ポーリング ハンドル
  //─────────────────
  // 接続スロットごとに行う前処理
  //━━━━━━━━━━━━━━━━━
  void P0_SETUP_CONTEXT(String argAdpID, String argFrame){

    ctx.adpID    = argAdpID; // アダプタID
    ctx.strFrame = argFrame; // フレーム
    P1_FORMAT_URI(ctx.strFrame);

    ctx.vStream.clear(); // 仮想ストリーム
    ctx.resMSG   = ""  ; // レスポンスメッセージ
    ctx.cmdPath  = ""  ; // コマンドパス
    ctx.authCD   = ""  ; // 認証コード
    ctx.accID    = -1  ; // アクセスID
  } /* P0_SETUP_CONTEXT() */

  //─────────────────
  // 受信バッファをURI形式に変換
  //----------------------------------
  //・先頭/末尾の不要文字を除去
  //─────────────────
  void P1_FORMAT_URI(String &str){
    //┬
    //○先頭の不要な文字をすべて削除
    while (str.length() > 0) {
      char c = str.charAt(0);
      if (c=='/'||c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\0')
      {str.remove(0, 1);} else {break;}
      } /* END-while */ 
    //│
    //○末尾の不要な文字をすべて削除
    while (str.length() > 0) {
      char c = str.charAt(str.length() - 1);
      if (c=='/'||c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\0')
      {str.remove(str.length()-1);} else {break;}
      } /* END-while */ 
    //┴
  } /* P1_FORMAT_URI() */


//━━━━━━━━━━━━━━━━━
// ３．基本情報を取得
//─────────────────
// フレームから[認証コード][コマンドパス]を取得する。
//━━━━━━━━━━━━━━━━━
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
    P1_FORMAT_URI(retStr); // 参照渡しなので内容は上書き
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
    P1_FORMAT_URI(retStr); // 参照渡しなので内容は上書き
    //│
    //▼返却：整形済みトークン以降
    return retStr    ;
  } /* GET_TOK2() */

  //━━━━━━━━━━━━━━━━━
  // フレームから認証CDとコマンドパスをセット
  //━━━━━━━━━━━━━━━━━
  void P1_SET_ACD_CPATH() {
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
  } /* P1_SET_ACD_CPATH() */

//━━━━━━━━━━━━━━━━━
// ５．MMPコマンドを実行
//━━━━━━━━━━━━━━━━━
  //━━━━━━━━━━━━━━━━━
  // コマンドを実行 または ＭＭＰ本体へ移譲
  //----------------------------------
  // MMP本体：コマンド管理にＭＭＰコマンドの実行を指示
  // MMPサブ：MMP本体にUARTコマンドをリクエスト
  //----------------------------------
  // 戻り値：コマンド実行結果(文字列)
  //━━━━━━━━━━━━━━━━━
  void P3_RUN(){
#if defined(MMP_TYPE_MAIN) // --┨ＭＭＰ本体┠----┐
    //┬
    //●ＭＭＰコマンドを実行
    ctx.resMSG = RUN_COMMAND();
    //┴
#else // -----------------------┨ＭＭＰサブ┠----┤
    //┬
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
    //┴
#endif // ----------------------------------------┘
  } /* P3_RUN() */