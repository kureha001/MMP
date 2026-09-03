// filename : adp.cpp
//========================================================
// アダプタ・マネージャ：アダプタを統括する
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/03) 
//========================================================
#pragma once

//┬
//■┐インクルード
  //■Arduinoシステム
  #include <vector>
  //│
  //■ＭＭＰシステム
  #include "adp.h"  // 通信アダプタ共通へ公開
  #include "cmd.h"  // コマンド・マネージャ
  //│
  //■ＭＭＰシステム(アダプタ群)
  #include "adapter/_API_.h"         // <<抽象基底クラス>>
  #include "adapter/UART.cpp"        // UART
  #include "adapter/TCP.cpp"         // TCP RAW
  #include "adapter/WEB_API.cpp"     // WEB API
  #include "adapter/WEB_Socket.cpp"  // WEB Socket
  #include "adapter/ESP_NOW.cpp"     // ESP-NOW
  #include "adapter/BLE.cpp"         // BLE
  #include "adapter/IIC.cpp"         // IIC
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源の所有（実体化）
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト
  //─────────────────
  MmpContext ctx;

  //─────────────────
  // 通信アダプタ群 (登録コンテナ)
  //─────────────────
  std::vector<AdapterBase*> ADAPTER; // 抽象基底ポインタのリスト

//========================================================
// 共通部品
//========================================================
  void FORMAT_URI(String &str){
    while (str.length() > 0) {
      char c = str.charAt(0);
      if (c=='/'||c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\0')
      {str.remove(0, 1);} else {break;}
    } 
    while (str.length() > 0) {
      char c = str.charAt(str.length() - 1);
      if (c=='/'||c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\0')
      {str.remove(str.length()-1);} else {break;}
    } 
  }

//========================================================
//【非公開機能】
//========================================================
 namespace adpBase{
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
    //○リクエストをデータ項目ごとに分解
    SET_ACD_CPATH();
    //│
    //●ユーザ認証を実施
    if (adpAUTH::CHECK()) return;
    //│＼（処理継続が不可の場合）
    //│ ▼終了：早期リターン
    //│
    //●コマンド・マネージャにコマンド実行を指示
    CommandManager::RunCommand();
    //┴

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
    //○ＭＭＰ本体からのレスポンスをコンテクストに反映
    ctx.resMSG = strRX;
    //┴
#endif // ----------------------------------------┘

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

} /* namespace adpBase */


//########################################################
//# 前空間：アダプタ・マネージャ
//########################################################
namespace AdapterManager{
  //========================================================
  //# アダプタの初期化
  //--------------------------------------------------------
  //【依存性注入の採用】
  // メイン側で固定生成した MmpContext の参照を各アダプタへ
  // 注入・共有することで、ヒープ断片化（動的確保）の防止と、
  // マルチプロトコル環境におけるグローバル変数汚染の回避を
  // 両立させる。
  //========================================================
  void INIT() {
    //┬
    //○メッセージ表示を開始
    Serial.println("<<アダプタの初期化>>");
    //│
  #if defined(MMP_TYPE_MAIN) //┨ＭＭＰ本体┠┐
    //○ユーザ認証の初期化
      adpAUTH::INIT_TBL();
  #endif //----------------------------------┘
    //│
    //●通信アダプタを初期化
  #if defined(ADP_COM_UART)
      ADAPTER.push_back(new AdapterUART(ctx));
  #endif
  #if defined(ADP_COM_TCP )
      ADAPTER.push_back(new AdapterTCP(ctx));
  #endif
  #if defined(ADP_COM_WAPI)
      ADAPTER.push_back(new AdapterWEB_API(ctx));
  #endif
  #if defined(ADP_COM_WSOC)
      ADAPTER.push_back(new AdapterWEB_Socket(ctx));
  #endif
  #if defined(ADP_COM_BLE )
      ADAPTER.push_back(new AdapterBLE(ctx));
  #endif
  #if defined(ADP_COM_ESPN)
      ADAPTER.push_back(new AdapterESPNOW(ctx));
  #endif
  #if defined(ADP_COM_I2C )
      ADAPTER.push_back(new AdapterIIC(ctx));
  #endif
    //│
    //○メッセージ表示を終了
    Serial.println("");
    //┴
  } /* INIT_ADAPTER() */

  //========================================================
  // アダプタのハンドル
  //========================================================
  void HANDLE(){
    for (auto* adp : ADAPTER) if (adp) adp->handle();
  } /* KICK_HANDLE() */

} /* namespace AdapterManager */