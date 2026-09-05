// filename : adapter/mod/member/main.cpp
//========================================================
// 経路アダプタ／動作モード／メンバー：メインモード
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#include "../../../cmd.h"


//########################################################
//# 前空間：メンバー（メインモード）
//########################################################
 namespace modeMain{
//========================================================
//【非公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 文字列整形部品（URI形式）
  //━━━━━━━━━━━━━━━━━
  void FORMAT_TOK(String &str){
    while (str.length() > 0) {
      char c = str.charAt(0);
      if (c=='/') str.remove(0, 1); else break;
    } /* END-if */
    while (str.length() > 0) {
      char c = str.charAt(str.length() - 1);
      if (c=='/') str.remove(str.length()-1); else break;
    } /* END-if */
  } /* FORMAT_TOK() */

  //─────────────────
  // 第１トークンを取得
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
    FORMAT_TOK(retStr); // 参照渡しなので内容は上書き
    //│
    //▼返却：整形済みトークン
    return retStr  ;
  } /* GET_TOK1() */

  //─────────────────
  // 第2トークン以降を取得
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
    FORMAT_TOK(retStr); // 参照渡しなので内容は上書き
    //│
    //▼返却：整形済みトークン以降
    return retStr    ;
  } /* GET_TOK2() */

  //━━━━━━━━━━━━━━━━━
  // 認証CD／コマンドパスを取得
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


//========================================================
//【公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  // メインモード
  //━━━━━━━━━━━━━━━━━
  void RUN(){
    //┬
    //○リクエストをデータ項目ごとに分解
    SET_ACD_CPATH();
    //│
    //●ユーザ認証を実施
    if (adpFnAuth::CHECK()) return;
    //│＼（処理継続が不可の場合）
    //│ ▼終了：早期リターン
    //│
    //●コマンド・マネージャに処理を移譲
    CommandManager::RunCommand();
    //┴
  } /* RUN() */
} /* namespace modeMain */