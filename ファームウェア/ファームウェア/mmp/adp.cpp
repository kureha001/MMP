// filename : adp.cpp
//========================================================
// アダプタ・マネージャ：アダプタを統括する
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <vector> // 登録コンテナが使用
  #include <queue>  // 経路アダプタが使用
  #include <mutex>  // 経路アダプタが使用
  //│
  //■ＭＭＰシステム
  #include "adp.h"
  #include "cmd.h"  // コマンド実行に使用
  //│
  //■ＭＭＰシステム(アダプタ群)
  #include "adapter/_API_.h"        // <<抽象基底クラス>>
  #include "adapter/UART.cpp"       // UART
  #include "adapter/TCP.cpp"        // TCP RAW
  #include "adapter/WEB_API.cpp"    // WEB API
  #include "adapter/WEB_Socket.cpp" // WEB Socket
  #include "adapter/ESP_NOW.cpp"    // ESP-NOW
  #include "adapter/BLE.cpp"        // BLE
  #include "adapter/IIC.cpp"        // IIC
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト（実体化）
  //─────────────────
  MmpContext ctx;

  //─────────────────
  // 経路アダプタ群 (登録コンテナ)
  //─────────────────
  std::vector<AdapterBase*> ADAPTER;


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

//########################################################
//# 前空間：処理手続き（基本）
//########################################################
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


  //━━━━━━━━━━━━━━━━━
  // ブリッジ用
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // ブリッジ・コマンド
    //----------------------------------
    // 戻り値：コマンド実行の有無（論理値）
    // ・true ：コマンド実行「あり」
    // ・false：コマンド実行「なし」
    //─────────────────
    bool BRIDGE_COMMAND(){
    /*
        //┬
        //○ctx.strFrame を コマンド、引数で配列に分解
        //│
        //○コマンドごとに転送先をセット
        if   (CMD[0] == "SYS/BRIDGE/@TCP" ) {break;}
        elif (CMD[0] == "SYS/BRIDGE/@WSOC") {break;}
        elif (CMD[0] == "SYS/BRIDGE/@WAPI") {break;}
        elif (CMD[0] == "SYS/BRIDGE/@BLE" ) {break;}
        elif (CMD[0] == "SYS/BRIDGE/@ESPN") {break;}
        elif (CMD[0] == "SYS/BRIDGE/@IIC" ) {break;}
        else {return false}
    */
        //│
        //▼返却：コマンド実行あり
        return true;
        //┴
    } /* BRIDGE_COMMAND() */

    //─────────────────
    // リクエストを転送
    //─────────────────
    void BRIDGE_TRANS(){
    // リクエストを指定経路に転送
    } /* BRIDGE_TRANS() */

    
  //━━━━━━━━━━━━━━━━━
  // モード別後続処理
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // １．本体モード
    //─────────────────
    void RUN_MAIN(){
      //┬
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
    } /* RUN_MAIN() */

    //─────────────────
    // ２．サブモード
    //─────────────────
    void RUN_SUB(){
      //┬
      //○コマンドをMMP本体のUARTへ転送
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
    } /* RUN_SUB() */

    //─────────────────
    // ３．ブリッジモード
    //─────────────────
    void RUN_BRIDGE(){
      //┬
      //●ブリッジ用コマンドに応答
      if (BRIDGE_COMMAND()) return;
      //│＼（専用コマンドを処理した場合）
      //│ ▼完了：早期リターン
      //│
      //○受信元に応じた経路に転送
      if (ctx.adpID == ADP_ID_UART) {
      //├┐（UARTの場合）
        //●リクエストを指定経路に転送
        BRIDGE_TRANS();
        //┴
      } else {
      //└┐（その他）
        //○MMP本体からのレスポンスをPCに転送
        Serial.print(ctx.strFrame);
        //┴
      }/* END-if */
      //┴
    } /* RUN_BRIDGE() */

//========================================================
//【公開機能】
//========================================================
  //━━━━━━━━━━━━━━━━━
  //【公開】MMPコマンドを実行
  //━━━━━━━━━━━━━━━━━
  void RUN(int argAdpID, String argFrame){
    //┬
    //●セットアップ
    SETUP(argAdpID, argFrame);
    //│
    //●モード別に後続処理
    if (ctx.sysMode == MODE_MAIN  ) RUN_MAIN();
    if (ctx.sysMode == MODE_SUB   ) RUN_SUB();
    if (ctx.sysMode == MODE_BRIDGE) RUN_BRIDGE();
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

} /* namespace adpBase */


//########################################################
//# 前空間：経路アダプタ・マネージャ
//########################################################
namespace AdapterManager{
  //========================================================
  //# アダプタの初期化（抽象化・一括管理）
  //========================================================
  void INIT() {
    //┬
    //○メッセージ表示を開始
    Serial.println("<<経路アダプタの初期化>>");
    //│
    //○ユーザ認証の初期化
      adpAUTH::INIT_TBL();
    //│
    //●経路アダプタを初期化
    #if defined(ADP_UART)
      ADAPTER.push_back(new AdapterUART(ctx));
    #endif
    #if defined(ADP_TCP )
      ADAPTER.push_back(new AdapterTCP(ctx));
    #endif
    #if defined(ADP_WAPI)
      ADAPTER.push_back(new AdapterWEB_API(ctx));
    #endif
    #if defined(ADP_WSOC)
      ADAPTER.push_back(new AdapterWEB_Socket(ctx));
    #endif
    #if defined(ADP_BLE )
      ADAPTER.push_back(new AdapterBLE(ctx));
    #endif
    #if defined(ADP_ESPN)
      ADAPTER.push_back(new AdapterESPNOW(ctx));
    #endif
    #if defined(ADP_I2C )
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