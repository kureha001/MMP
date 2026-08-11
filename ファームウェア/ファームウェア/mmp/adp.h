// filename : adp.h
//========================================================
// 通信アダプタ共通
//  - 経路IDの提供
//  - クライアント管理機能の提供
//--------------------------------------------------------
// Ver 1.0.1 (2026/08/11) α版 
//・ファイル名を変更
//・全体的にリファクタリング
//・インクルードファイルを最適化
//・コメントを強化
//・セッション管理方式を変更
//・ユーザ認証機能を追加
//・クライアント管理機能を追加
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <string.h>
  #include <WiFi.h>      // TCPブリッジ用
  #include <WebServer.h> // WebAPI用
  //│
  //■ＭＭＰシステム
  //┴
//┴

//========================================================
// 基本情報
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 通信経路：この種類ごとにアダプタがある
  //━━━━━━━━━━━━━━━━━
  static const int PORTS_SERIAL    = 2 ; // シリアル系経路の総数
  static const int ROUTE_ID_SERIAL = 0 ; // シリアル用
  static const int ROUTE_ID_TCP    = 1 ; // TCPブリッジ用
  static const int ROUTE_ID_HTTP   = 2 ; // WebAPI用

//========================================================
// 接続管理：ユーザを接続単位で受信状態を管理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
  static const int SS_SLOTS      = 10    ; // 経路毎のスロット総数
  static const int SS_RX_SIZE    = 256   ; // 受信バッファ容量
  const uint32_t   SS_TIME_LIMIT = 30000 ; // タイムアウト閾値

  //━━━━━━━━━━━━━━━━━
  // スロット
  //─────────────────
  // SLOT_BASE      used/rx/isOverflow
  // ├ SLOT_STREAM ├ conn(Stream)/accID
  // ├ SLOT_TCP    ├ conn(WiFiClient)/authCD/lastActive
  // └ SLOT_HTTP   └ conn(WebServer)/authCD(認証情報TBL検索用キー)
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // Ｚ．共通部
    //─────────────────
    struct SLOT_BASE {
      bool    used       = false ; // 使用状況フラグ
      String  rx         = ""    ; // 受信バッファ
      bool    isOverflow = false ; // 容量超過フラグ
    };
    //----------------------------------
    void INIT_SLOT_BASE(SLOT_BASE& argSlot){
      argSlot.used       = false    ; // 使用状況を未使用に設定
      argSlot.rx         = ""       ; // 受信バッファをクリア
      argSlot.rx.reserve(SS_RX_SIZE); // 受信バッファ容量を事前確保
      argSlot.isOverflow = false    ; // 容量超過フラグをクリア
    }
    //─────────────────
    // Ａ．ストリーム資源(ポインタ)
    //----------------------------------
    // 所有：シリアル(ストリーム)を所有しない ※Arduinoに存在する資源
    // 参照：外部生成されたストリーム資源
    // 割当：特定の物理ポートを処理
    // 持続：永続的に利用 ※start()で一度だけ初期化
    //─────────────────
    struct SLOT_STREAM : SLOT_BASE { // Ｚ．共通部
      Stream* conn  = nullptr ; // 接続資源(個別ストリームを参照)
      int     accID = -1      ; // ユーザID(物理ポート別の固定値)
    };
    //----------------------------------
    void INIT_SLOT_STREAM(SLOT_STREAM& argSlot){
      INIT_SLOT_BASE(argSlot) ; // Ｚ．共通部
      argSlot.conn  = nullptr ; // 参照解除
      argSlot.accID = -1      ; // クリア
    }
    //─────────────────
    // Ｂ．WiFiサーバ資源(接続確認あり)
    //----------------------------------
    // 所有：TCP接続オブジェクトを所有する
    // 保持：個別TCP接続情報を保持
    // 割当：単一のTCP接続を処理
    // 持続：ポーリング中に新規接続で生成／切断で破棄
    //─────────────────
    struct SLOT_TCP : SLOT_BASE{   // Ｚ．共通部
      WiFiClient conn            ; // 接続資源(個別TCP接続の実体)
      String     authCD     = "" ; // 認証情報TBL検索用キー
      uint32_t   lastActive = 0  ; // 最終更新時刻(ms)
    };
    //----------------------------------
    void INIT_SLOT_TCP(SLOT_TCP& argSlot){
      INIT_SLOT_BASE(argSlot) ; // Ｚ．共通部
      argSlot.conn.stop()     ; // 資源破棄
      argSlot.authCD     = "" ; // クリア
      argSlot.lastActive = 0  ; // クリア
    }
    //─────────────────
    // Ｃ．WEBサーバ資源(ポインタ)
    //----------------------------------
    // 所有：HTTPサーバ資源を所有しない
    // 参照：start()で生成された受付資源を参照
    // 担当：複数のHTTP要求を処理
    // 持続：永続的に利用 ※start()で一度だけ初期化
    //─────────────────
    struct SLOT_HTTP : SLOT_BASE{  // Ｚ．共通部
      WebServer* conn   = nullptr; // 受付資源(HTTPサーバの参照)
      String     authCD = ""     ; // 認証情報TBL検索用キー
    };
    //----------------------------------
    void INIT_SLOT_HTTP(SLOT_HTTP& argSlot){
      INIT_SLOT_BASE(argSlot)    ; // Ｚ．共通部
      argSlot.conn   = nullptr   ; // 参照解除
      argSlot.authCD = ""        ; // クリア
    }
    //─────────────────

//========================================================
// ユーザ認証：ユーザを認証コードで管理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
  static const int AUTH_SLOTS      = 15     ; // 認証情報総数
  static const int AUTH_ROUTES     = 2      ; // 認証を用いる通信経路の総数
  const  uint32_t  AUTH_TIME_LIMIT = 100000 ; // タイムアウト閾値

  //━━━━━━━━━━━━━━━━━
  // スロット
  //─────────────────
  // ※必要とする通信アダプタごとに自分の資源として作成する
  //━━━━━━━━━━━━━━━━━
  struct TYPE_AUTH_SLOT {
    String   authCD     = ""    ; // 認証コード
    bool     used       = false ; // 有効性判定
    uint32_t lastActive = 0     ; // 最終更新時刻(ms) ※ タイムアウトで使用
  };
 
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
  static String AUTH_CREATE_CD(){
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
  } /* AUTH_CREATE_CD() */

  //─────────────────
  // 4-2-1.ユーザ認証を実施
  //----------------------------------
  // 認証コードと一致する認証情報TBLのデータ位置を取得
  //----------------------------------
  // 引数：
  // ・認証情報TBL：検索対象データセット
  // ・認証コード ：検索キー
  //----------------------------------
  // 戻り値：認証ID(数値型)
  //  データあり：0,1,....
  //  データなし：-1
  //─────────────────
  static int AUTH_GET_ID(
    TYPE_AUTH_SLOT* pTBL   , // 認証情報TBL
    const String&   pKeyCD   // 認証コード(検索キー)
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
    } /* if  */
  } /* for */
  //│
  //▼RETURN：重複無し
  return -1;
  //┴
  } /* AUTH_GET_ID() */

  //─────────────────
  // 4-1-1.この接続を認証管理に加える
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
  static String AUTH_START(TYPE_AUTH_SLOT* pTBL){
    //┬
    //○前処理
    String retCD = "" ; // 戻り値を[失敗]で初期化
    String newCD = "" ; // 新しい認証コード
    //│
    //◎┐新たな認証情報を登録
    for (int freeID = 0; freeID < AUTH_SLOTS; freeID++){
      //◇┐空きスロットに登録
      if (!pTBL[freeID].used){
        //├→（スロットが未使用の場合)
          //◎┐新しい認証コードを生成
          while (true){
            //●認証コードを生成
            newCD = AUTH_CREATE_CD();
            //│
            //●既存コードと照合
            if (AUTH_GET_ID(pTBL, newCD) == -1){break;}
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
      }
      //┴
    } /* END-for */
    //│
    //▼RETURN：認証コードを返す
    return retCD;
    //┴
  } /* AUTH_START() */


//========================================================
// 受信データ：フレーム（URI)を編集
//========================================================
  //─────────────────
  // 3-1.受信バッファをURI形式に変換
  //----------------------------------
  //・先頭/末尾の不要文字を除去
  //・エスケープ文字処理 ※予定
  //・URI書式へ整形　　　※予定
  //─────────────────
  static void FormatURI(String &str){
    while (str.startsWith("/")){str.remove(0, 1);            } // 先頭の'/' をすべて削除
    while (str.endsWith("/")  ){str.remove(str.length() - 1);} // 末尾の'/'をすべて削除
  }

  //─────────────────
  // 3-2-1.認証コードを取得
  //----------------------------------
  // フレームから第１トークンを取り出す
  //・認証コード
  //・発行依頼コマンド
  //----------------------------------
  // 戻り値：第1トークン文字列
  //・正常：余計な文字を省いた純粋な内容
  //─────────────────
  static String GetToken1(String &pURI){
    //┬
    //◇┐URIから切り出す
    String retStr = ""   ; // リクエストURI
    int    pos    = pURI.indexOf('/');
    if (pos >= 0) {
      //├→（URIに"/"がある）
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
    FormatURI(retStr); // 参照渡しなので内容は上書き
    return retStr  ;
    //┴
  }

  //─────────────────
  // 3-2-2.コマンドパスを取得
  //----------------------------------
  // フレームから第2トークン以降を取り出す
  //----------------------------------
  // 戻り値：第２トークン以降の文字列
  // ・正常：余計な文字を省いた純粋な内容
  //─────────────────
  static String GetToken2(String &pURI){
    //┬
    //◇┐URIから切り出す
    String retStr = ""   ; // 戻り値
    int    pos    = pURI.indexOf('/');

    if (pos >= 0) {
      //├→（URIに"/"がある）
        //○URIから第２トークン以降を取得
        retStr = pURI.substring(pos + 1);
        //┴
    }
    //│
    //●切り出した文字列を整形
    //▼RETURN：整形済みトークン以降 ※末尾'!'があれば残る
    FormatURI(retStr); // 参照渡しなので内容は上書き
    return retStr    ;
    //┴
  }


//========================================================
// ユーザメモリ：機能モジュールのユーザメモリを管理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
  // ユーザ総数：ユーザ認証総数にシリアル総数を加えた数
  static constexpr int USER_COUNT = (AUTH_SLOTS * AUTH_ROUTES) + PORTS_SERIAL;

  //─────────────────
  // 4-1-1.対象ユーザを特定
  //----------------------------------
  // ユーザIDを取得
  //----------------------------------
  // 引数：
  // ・経路ID：対象の通信経路用
  // ・認証ID：シリアル系はダミー値
  //----------------------------------
  // 戻り値：整数型
  // ・正常：ユーザID
  // ・異常：-1
  //─────────────────
  static int GET_USER_ID(int routeID, int authID){
    int offset = -1;
    switch(routeID){
    case ROUTE_ID_SERIAL: offset = 0; break; // シリアルは先頭なのでオフセット無し
    case ROUTE_ID_TCP   : offset = PORTS_SERIAL + AUTH_SLOTS * 0; break;
    case ROUTE_ID_HTTP  : offset = PORTS_SERIAL + AUTH_SLOTS * 1; break;
    }
    if (offset < 0) return -1;
    return (offset + authID);
  } /* GetUserID() */
