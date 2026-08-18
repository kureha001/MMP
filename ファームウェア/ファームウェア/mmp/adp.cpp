// filename : adp.cpp
//========================================================
// 通信アダプタ共通
//  - 経路IDの提供
//  - クライアント管理機能の提供
//--------------------------------------------------------
// Ver 1.0.1 (2026/08/13) α版 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <string.h>
  //│
  //■ＭＭＰシステム
  #include "adp.h" // 通信アダプタ共通
  #include "cmd.h" // RUN_COMMAND()
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コマンド管理
  //─────────────────
  extern String RUN_COMMAND(); // 所在：parser.h

//========================================================
// 基本情報
//========================================================
    //─────────────────
    // スロット初期化
    //----------------------------------
    // 通信アダプタの名前空間で派生(名称:INIT_SLOT)
    //─────────────────
    void INI_SS_SLOT_BASE(T_SS_BASE& argSlot){
      argSlot.used       = false    ; // スロット有効性をクリア
      argSlot.rx         = ""       ; // 受信バッファをクリア
      argSlot.rx.reserve(SS_RX_SIZE); // 受信バッファ容量を事前確保
      argSlot.isOverflow = false    ; // 容量超過フラグをクリア
    } /* INI_SS_SLOT_BASE */

//========================================================
// ユーザ認証
//========================================================
  //━━━━━━━━━━━━━━━━━
  // スロット
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // 初期化
    //----------------------------------
    // 通信アダプタの名前空間で派生(名称:INIT_SLOT)
    //─────────────────
    void INIT_SLOT_AUTH(T_AUTH_SLOT& argSlot){
      argSlot.used       = false ; // 有効性判定リセット(無効)
      argSlot.authCD     = ""    ; // 認証コードをクリア
      argSlot.lastActive = 0     ; // 最終更新時刻をリセット
    } /* INIT_SLOT_AUTH */

  //─────────────────
  // 認証コード定義
  //─────────────────
  static const char* AUTH_GROUPS[] = { // 文字グループ
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",      // ・アルファベット大文字
    "abcdefghijklmnopqrstuvwxyz",      // ・アルファベット小文字
    "0123456789"                       // ・数字
  };
  static constexpr int AUTH_CD_LENGTH  = 5; // 文字コード長
  static constexpr int AUTH_CHR_GROUPS = sizeof(AUTH_GROUPS) / sizeof(AUTH_GROUPS[0]);

  //─────────────────
  // ヘルパー：新しい認証コードを取得
  //----------------------------------
  // 仕様:
  // ・AUTH_GROUPS の各グループを最低1回使用
  // ・文字配置はランダム
  // ・余った桁は全グループからランダム選択
  //----------------------------------
  // 戻り値：認証コード
  //─────────────────
  static String AUTH_START_CREATE(){
    //┬
    //○前処理
    String code = "";
    //│
    //◎┐各グループから最低1文字取得
    for (int id = 0; id < AUTH_CHR_GROUPS; id++){
      //○カウンタ(グループID)を判定
        // ＼（全グループ処理が完了した場合）
          //▼この繰返し処理を中断
      //│
      //○当該グループの文字セットからランダムな１文字を追加
      int len = strlen(AUTH_GROUPS[id])   ; // データセット長
      code += AUTH_GROUPS[id][random(len)]; // ランダムな１文字を後方マージ
      //┴
    }   /* for */
    //│
    //◎┐不足文字分をランダムに追加
    while (code.length() < AUTH_CD_LENGTH){
      //○文字列長を判定
        // ＼（上限[認証コード長]に達した場合）
          //▼この繰返し処理を中断
      //│
      //○ランダムなグループの文字セットからランダムな１文字を追加
      int groupID = random(AUTH_CHR_GROUPS)     ; // ランダムなグループ
      int len     = strlen(AUTH_GROUPS[groupID]); // ランダムな文字
      code += AUTH_GROUPS[groupID][random(len)] ; // １文字を後方マージ
      //┴
    }   /* while */
    //│
    //◎┐文字位置をシャッフル
    for (int id = 0; id < code.length(); id++){
      //○文字列長を判定
        // ＼（上限[認証コード長]に達した場合）
          //▼この繰返し処理を中断
      //│
      //○当該桁の文字をランダムな桁の文字と入れ替え
      int swapID   = random(code.length()); // 移動元桁数をランダムに取得
      char tmp     = code[id]             ; // 当該桁の文字を退避
      code[id]     = code[swapID]         ; // 当該桁に移動元桁の文字を移送
      code[swapID] = tmp                  ; // 移動元桁に退避した文字を移送
    }   /* for */
    //│
    //▼認証コードを返す
    return code;
    //┴
  } /* AUTH_START_CREATE() */

  //─────────────────
  // 古いスロットを照会
  //----------------------------------
  // 該当条件：使用中 かつ タイムアウト
  //----------------------------------
  // 戻り値：
  // ・0,1,2...：タイムアウトしたスロットID
  // ・-1：タイムアウトしたスロットが無い
  //─────────────────
  int AUTH_GET_ID_OLD(T_AUTH_SLOT* pTBL) {
    //┬
    //◎┐先頭から走査
    for (int id = 0; id < AUTH_SLOTS; id++) {
      //○次データの捜査を開始
      // ＼（全スロットを走査し終えた場合）
      //▼ループ処理を中断
      //│
      //○一致確認
      if ( pTBL[id].used &&
        millis() - pTBL[id].lastActive > AUTH_TIME_LIMIT) return id;
        // ＼（使用中でタイムアウトしている場合）
          //▼当該スロットIDを返す
      //┴
    } /* END-for */
    //│
    //▼エラーコードを返す(空きスロットがない)
    return -1;
    //┴
  } /* AUTH_GET_ID_OLD() */


//========================================================
// 処理プロセス
//========================================================
//━━━━━━━━━━━━━━━━━
// デバッグログ表示
//━━━━━━━━━━━━━━━━━
  void LOG_PRINT(String argMsg){
    Serial.println(String("============================"));
    Serial.println(String("version  :") + String(ctx.version));

    Serial.println(String("----------------------------"));
    Serial.println(String("strFrame : ") + String(ctx.strFrame));
    Serial.println(String("cmdPath  : ") + String(ctx.cmdPath));
    Serial.println(String("authCD   : ") + String(ctx.authCD));

    Serial.println(String("----------------------------"));
    Serial.println(String("routeID  : ") + String(ctx.routeID));
    Serial.println(String("slotID   : ") + String(ctx.slotID));
    Serial.println(String("authID   : ") + String(ctx.authID));

    Serial.println(String("----------------------------"));
    Serial.println(String("accID    : ") + String(ctx.accID));
    Serial.println(String("accIDS   : ") + String(ctx.accIDS));

    Serial.println(String("----------------------------"));
    Serial.println(String("vStream  : ") + String(ctx.vStream.str()));
    Serial.println(String("response : ") + String(argMsg));
  } /* LOG_PRINT() */

//━━━━━━━━━━━━━━━━━
// ０．ポーリング ハンドル
//─────────────────
// 接続スロットごとに行う前処理
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクストをセットアップ
  //─────────────────
  void P0_SETUP_CTX(int argRID, int argSID){

    // ---フレームデータ---
    ctx.strFrame = ""    ; // フレーム
    ctx.cmdPath  = ""    ; // コマンドパス
    ctx.authCD   = ""    ; // 認証コード

    // ---グルーピング---
    ctx.routeID  = argRID; // 経路ID
    ctx.slotID   = argSID; // スロットID
    ctx.authID   = 0     ; // 認証ID（0：常時接続）

    // ---アクセスID---
    ctx.accID    = -1   ; // アクセスID
    ctx.accIDS   = -1   ; // アクセスIDの総数

  } /* P0_SETUP_CTX() */

//━━━━━━━━━━━━━━━━━
// ２．フレームを取得
//─────────────────
// 受付資源からフレーム文字列を取得する。
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 受信バッファをURI形式に変換
  //----------------------------------
  //・先頭/末尾の不要文字を除去
  //・エスケープ文字処理 ※予定
  //・URI書式へ整形　　　※予定
  //─────────────────
  void P2_FORMAT_URI(String &str){
    while (str.startsWith("/")){str.remove(0, 1);            } // 先頭の'/' をすべて削除
    while (str.endsWith("/")  ){str.remove(str.length() - 1);} // 末尾の'/'をすべて削除
  }

//━━━━━━━━━━━━━━━━━
// ３．基本情報を取得
//─────────────────
// フレームから[認証コード][コマンドパス]を取得する。
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // フレームから第１トークンを取り出す
  //----------------------------------
  // 戻り値：第1トークン文字列
  //・正常：余計な文字を省いた純粋な内容
  //─────────────────
  String P3_GET_TOKEN1(String &pURI){
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
    }   /* if */
    //│
    //●切り出した文字列を整形
    //▼RETURN：整形済みトークン ※末尾'!'があれば残る
    P2_FORMAT_URI(retStr); // 参照渡しなので内容は上書き
    return retStr  ;
    //┴
  } /* P3_GET_TOKEN1() */

  //─────────────────
  // フレームから第2トークン以降を取り出す
  //----------------------------------
  // 戻り値：第２トークン以降の文字列
  // ・正常：余計な文字を省いた純粋な内容
  //─────────────────
  String P3_GET_TOKEN2(String &pURI){
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
    //▼RETURN：整形済みトークン以降 ※末尾'!'があれば残る
    P2_FORMAT_URI(retStr); // 参照渡しなので内容は上書き
    return retStr    ;
    //┴
  } /* P3_GET_TOKEN2() */

//━━━━━━━━━━━━━━━━━
// ４．認証を実施
//─────────────────
// 新たに認証コードを発行し、当該ユーザの管理を開始する。
// 認証コードで、認証情報TBLを照会する。
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ユーザ認証を実施
  //----------------------------------
  // 認証コードが一致するスロットIDを取得
  //----------------------------------
  // 引数：
  // ・認証情報TBL：検索対象データセット
  // ・認証コード ：検索キー
  //----------------------------------
  // 戻り値：認証ID(数値型)
  //  データあり：0,1,....
  //  データなし：-1
  //─────────────────
  int P4_GET_ID(
    T_AUTH_SLOT*  pTBL,  // 認証情報TBL
    const String& pKeyCD // 認証コード(検索キー)
  ){
    //┬
    //◎┐認証情報全体を照合
    for (int id = 0; id < AUTH_SLOTS; id++){
      //○現在の認証情報と照合
      if (pTBL[id].used && pTBL[id].authCD == pKeyCD) {
        // ＼（認証コードが一致)
          //○タイムスタンプを更新
          //▼RETURN：データあり
          pTBL[id].lastActive = millis();
          return id;
      } /* END-if  */
    } /* END-for */
    //│
    //▼RETURN：重複無し
    return -1;
    //┴
  } /* P4_GET_ID() */

  //─────────────────
  // 認証管理に加える
  //----------------------------------
  // 新たな認証コードでスロットを作成
  // 新たな認証コードは認証情報TBL内で一意
  // 空きスロットが無い場合は失敗
  //----------------------------------
  // 引数：
  // ・認証情報TBL：作成対象データセット
  //----------------------------------
  // 戻り値：認証コード(文字列型)
  // ・成功：半角の大小文字を含む英数で5文字
  // ・失敗：空文字
  //─────────────────
  String P4_START(T_AUTH_SLOT* pTBL){
    //┬
    //○前処理
    String retCD = "" ; // 戻り値を[失敗]で初期化
    String newCD = "" ; // 新しい認証コード
    //│
    //◎┐新たな認証情報を登録
    for (int freeID = 0; freeID < AUTH_SLOTS; freeID++){
      //◇┐空きスロットに登録
      if (!pTBL[freeID].used){
        //├┐（スロットが未使用の場合)
          //◎┐新しい認証コードを生成
          while (true){
            //●認証コードを生成
            newCD = AUTH_START_CREATE();
            //│
            //●既存コードと照合
            if (P4_GET_ID(pTBL, newCD) == -1){break;}
            // ＼（同じ認証コードが存在しない場合）
              //▼作成した認証コードを採用
          } /* while */
          //│
          //○空きスロットに登録
          pTBL[freeID].authCD     = newCD;
          pTBL[freeID].used       = true;
          pTBL[freeID].lastActive = millis();
          //│
          //○戻り値をセット
          retCD = newCD;
          //│
          //▼この繰り返し処理を中断 ※登録は１度だけ
          break;
        //└┐（その他）
          //┴
      }
      //┴
    } /* END-for */
    //│
    //▼RETURN：認証コードを返す
    return retCD;
    //┴
  } /* P4_START() */

//━━━━━━━━━━━━━━━━━
// ５．MMPコマンドを実行
//─────────────────
// コンテクストの内容を用いてアクセスIDを求める。
// コマンド管理にＭＭＰコマンドの実行を指示する。
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // フレームから第2トークン以降を取り出す
  //----------------------------------
  // 戻り値：文字列
  // ・正常：コマンド管理のレスポンス
  //─────────────────
  String P5_RUN(){
    //┬
    //◇オフセットを求める
    int offsetNum = PORTS_SERIAL + PORTS_BLE;
    switch(ctx.routeID){
    case ROUTE_ID_SERIAL: offsetNum = 0 ; break;
    case ROUTE_ID_BLE   :                 break;
    case ROUTE_ID_TCP   : 
    case ROUTE_ID_HTTP  :
      offsetNum += AUTH_SLOTS * (ctx.routeID - ROUTE_ID_BLE -1);
      break;
    default             : offsetNum = -1; break;
    }
    //│
    //○アクセスIDをセット（オフセット ＋ スロットID ＋ 認証ID）
    ctx.accID = offsetNum + ctx.slotID + ctx.authID;
    //│
    //○アクセスIDの総数
    ctx.accIDS = (AUTH_SLOTS * AUTH_ROUTES) + PORTS_SERIAL;
    //│
    //●ＭＭＰコマンドを実行→リターン
    return RUN_COMMAND();
    //┴
  } /* P5_RUN() */


//========================================================
// サービス・アダプタ
//========================================================
  void InitAdapter() {
    //┬
    //○開始表示
    Serial.println("---------------------------");
    Serial.println("<<サービス・アダプタの初期化>>");
    //│
    //●通信アダプタ
    adpSerial::start();
    adpTcp   ::start();
    adpBLE   ::start();
    adpHttp  ::start();
    //│
    //●ＷＥＢアダプタ
    adpAdmin ::start();
    //│
    //○終了表示
    Serial.println("");
    //┴
  } /* InitAdapter() */

  void kickHandle(){
    adpSerial::handle();
    adpTcp   ::handle();
    adpHttp  ::handle();
    //adpBLE ::handle(); ※割込み処理されるので実行不要
    adpAdmin ::handle();
  } /* kickHandle() */