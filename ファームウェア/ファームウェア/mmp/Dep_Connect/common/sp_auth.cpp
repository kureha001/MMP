// filename : Dep_Connect/common/sp_auth.cpp
//========================================================
// 接続部門／共通課：ユーザ認証係
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/14)
//========================================================

//########################################################
//# 処理詳細
//########################################################
namespace adpFnAuth{
  //━━━━━━━━━━━━━━━━━
  // 一般情報
  //━━━━━━━━━━━━━━━━━
  const uint32_t AUTH_TIME_LIMIT = 100000 ; // タイムアウト閾値

  //─────────────────
  // 構造体
  //─────────────────
  struct AU_SLOT_TYPE {
    bool     used       = false ; // 有効性判定
    String   authCD     = ""    ; // 認証CD
    uint32_t lastActive = 0     ; // 最終更新時刻 ※単位：ms
  };
  static AU_SLOT_TYPE* auTBL = nullptr; // 領域確保


  //─────────────────
  // スロット初期化
  //----------------------------------
  // 経路アダプタの名前空間で派生(名称:INIT_SLOT)
  //─────────────────
  void AUTH_INIT_SLOT(AU_SLOT_TYPE& argSlot){
    argSlot.used       = false ; // 有効性判定リセット(無効)
    argSlot.authCD     = ""    ; // 認証CDをクリア
    argSlot.lastActive = 0     ; // 最終更新時刻をリセット
  } /* AUTH_INIT_SLOT */


  //─────────────────
  // 認証CD生成
  //----------------------------------
  // 仕様:
  // ・AUTH_GROUPS の各グループから1文字採用
  // ・文字配置はランダム
  //----------------------------------
  // 戻り値：認証CD
  //─────────────────
  static const char* AUTH_GROUPS[] = { // 文字グループ
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",      // ・アルファベット大文字
    "abcdefghijklmnopqrstuvwxyz",      // ・アルファベット小文字
    "0123456789",                      // ・数字
    "-+(){}[]$%_^&"                    // ・記号
  };
  static constexpr int AUTH_CHR_GROUPS = sizeof(AUTH_GROUPS) / sizeof(AUTH_GROUPS[0]);
  //----------------------------------
  String AUTH_CREATE_ACD(){
    //┬
    //○前処理
    String tmpAID = "";
    //│
    //◎┐各グループから最低1文字取得
    for (int grpID = 0; grpID < AUTH_CHR_GROUPS; grpID++){
      //○カウンタ(グループID)を判定
      //│＼（全グループ処理が完了した場合）
      //│ ▽完了：走査終了
      //│
      //○当該グループの文字セットからランダムな１文字を追加
      int len = strlen(AUTH_GROUPS[grpID])   ; // データセット長
      tmpAID += AUTH_GROUPS[grpID][random(len)]; // ランダムな１文字を後方マージ
      //┴
    } /* END-for */
    //│
    //◎┐文字位置をシャッフル
    for (int nowID = 0; nowID < tmpAID.length(); nowID++){
      //│＼（最終桁に到達した場合）
      //│ ▽完了：走査終了
      //│
      //○当該桁の文字をランダムな桁の文字と入れ替え
      int  swapID    = random(tmpAID.length()); // 移動元桁数をランダムに取得
      char tmpChar   = tmpAID[nowID ]         ; // 当該桁の文字を退避
      tmpAID[nowID ] = tmpAID[swapID]         ; // 当該桁に移動元桁の文字を移送
      tmpAID[swapID] = tmpChar                ; // 移動元桁に退避した文字を移送
    }   /* for */
    //│
    //▼認証CDを返す
    return tmpAID;
  } /* AUTH_CREATE_ACD() */

  //─────────────────
  // 古いスロットを照会
  //----------------------------------
  // 該当条件：使用中 かつ タイムアウト
  //----------------------------------
  // 戻り値：
  // ・0～：タイムアウトしたスロットID
  // ・-1 ：空きスロットがない
  //─────────────────
  int AUTH_GET_ID_OLD() {
    //┬
    //◎┐先頭から走査
    for (int oldID = 0; oldID < ctx.accIDS; oldID++) {
      //│＼（スロットの上限に達した場合）
      //│ ▽完了：走査終了
      //│
      //○一致確認
      if ( auTBL[oldID].used &&
        millis() - auTBL[oldID].lastActive > AUTH_TIME_LIMIT) return oldID;
      //│＼（使用中でタイムアウトしている場合）
      //│ ▼返却：当該スロットID
      //┴
    } /* END-for */
    //│
    //▼返却：空きスロットがない
    return -1;
  } /* AUTH_GET_ID_OLD() */

  //─────────────────
  // ユーザ認証を実施
  //----------------------------------
  // 認証CDが一致するかを確認
  //----------------------------------
  // 引数：
  // ・認証CD ：検索キー
  //----------------------------------
  // 戻り値：認証ID(数値型)
  //  既データなし： -1
  //  既データあり： 0～
  //─────────────────
  int GET_EXIST_AID(String argACD){
    //┬
    //◎┐認証情報全体を照合
    for (int extID = 0; extID < ctx.accIDS; extID++){
      //│＼（スロットの上限に達した場合）
      //│ ▽完了：走査終了
      //│
      //○現在の認証情報と照合
      if (auTBL[extID].used && auTBL[extID].authCD == argACD) {
      //│＼（認証CDが一致)
          //○タイムスタンプを更新
          //▼返却：既データあり
          auTBL[extID].lastActive = millis();
          return extID;
      } /* END-if  */
      //┴
    } /* END-for */
    //│
    //▼返却：既データなし
    return -1;
  } /* GET_EXIST_AID() */

  //─────────────────
  // ユーザの認証管理を開始
  //----------------------------------
  // 新たな認証CDでスロットを作成
  // 新たな認証CDは認証情報TBL内で一意
  // 空きスロットが無い場合は失敗
  //----------------------------------
  // 戻り値：認証CD(文字列型)
  // ・成功： false
  // ・失敗： true
  //─────────────────
  bool NEW_USER(){
    //┬
    //○前処理
    String retCD = "" ; // 戻り値を[失敗]で初期化
    String newCD = "" ; // 新しい認証CD
    ctx.accID    = -1 ; // アクセスIDをクリア
    //│
    //◎┐新たな認証情報を登録
    for (int freeID = 0; freeID < ctx.accIDS; freeID++){
      //│＼（スロットの上限に達した場合）
      //│ ▽完了：走査終了
      //│
      //◇┐空きスロットに登録
      if (!auTBL[freeID].used){
        //├┐（スロットが未使用の場合)
          //◎┐新しい認証CDを生成
          while (true){
            //│
            //●認証CDを生成
            newCD = AUTH_CREATE_ACD();
            //│
            //●生成した認証CDが一意であるかを確認
            if (GET_EXIST_AID(newCD) < 0) break;
            // ＼（存在しない場合）
              //▽中断：作成した認証CDを採用
          } /* END-while */
          //│
          //○コンテクストを更新
          ctx.resMSG = newCD + "!"; // レスポンス
          ctx.authCD = newCD      ; // 認証CD
          ctx.accID  = freeID     ; // アクセスID
          //│
          //○空きスロットに登録
          auTBL[freeID].authCD     = newCD;
          auTBL[freeID].used       = true;
          auTBL[freeID].lastActive = millis();
          //│
          //▼返却：成功
          return false;
        //└┐（その他）
          //┴
      } /* END-if */
      //┴
    } /* END-for */
    //│
    //▼返却：失敗
    return true;
  } /* NEW_USER() */

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
// 担務（公開機能）
//========================================================
  //─────────────────
  // テーブル初期化
  //----------------------------------
  // アダプタ初期化の直前で実行
  //─────────────────
  void INIT_TBL() {
    auTBL = new AU_SLOT_TYPE[ctx.accIDS];
  } /* INIT_TBL() */

  //─────────────────
  // 認証を実施・認証開始コマンド応答
  //----------------------------------
  // 戻り値：認証結果（論理値）
  // ・false： 処理継続が可能
  // ・true ： 処理継続が不可
  //----------------------------------
  // 処理継続が可能の場合、早期エラーメッセージにセット
  //─────────────────
  bool CHECK(){
    //┬
    //○リクエストをデータ項目ごとに分解
    SET_ACD_CPATH();
    //│
    //◇┐認証開始要求に応答
    if (ctx.cmdPath == SP_CMD_START) {
      //├┐（「認証CD発行コマンド」の場合）
        //●認証管理に加える
        if(NEW_USER()){ctx.resMSG = RCD::AuthErr1;}
        //│＼（失敗した場合）
        //│ ○レスポンスにエラーIDをセット
        //│ ┴
        //│
        //▼返却：認証開始コマンド(要レスポンス)
        return true;
    } /* END-if */
    //│
    //○ユーザ認証対象を確認
    if (ctx.authCD == ""){ctx.accID = 0; return false;}
    //│＼（認証が不要の場合）
    //│ ○ユーザIDを共用IDにセット
    //│ ▼返却：認証が不要
    //│
    //●ユーザ認証を実施
    ctx.accID = GET_EXIST_AID(ctx.authCD);
    if (ctx.accID < 0){ctx.resMSG = RCD::AuthErr2; return true;}
    //│＼（認証に失敗した場合）
    //│ ▼返却：[3]認証に失敗(要レスポンス)
    //│
    //▼返却：認証に成功
    return false;
  } /* CHECK() */

} /* namespace adpFnAuth */