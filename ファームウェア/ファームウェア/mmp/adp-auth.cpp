// filename : adp.cpp
//========================================================
// 通信アダプタ共通：ユーザ認証
//--------------------------------------------------------
//【目的】
// ・各プロセスの共通処理（ユーザ認証のみ）を提供する
//--------------------------------------------------------
//【共通資源】
//・F4_CHECK_AUTH()：認証を実施・認証開始コマンド応答
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/23) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <string.h>
  //│
  //■ＭＭＰシステム
  #include "conf.h"   // 各種設定
  #include "adp.h" // 通信アダプタ共通
  //┴
//┴

//========================================================
// 処理プロセス（ユーザ認証のみ）
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
  const uint32_t AUTH_TIME_LIMIT = 100000 ; // タイムアウト閾値

  //─────────────────
  // 構造体
  //─────────────────
  struct AU_SLOT_TYPE {
    bool     used       = false ; // 有効性判定
    String   authCD     = ""    ; // 認証コード
    uint32_t lastActive = 0     ; // 最終更新時刻 ※単位：ms
  };
  static AU_SLOT_TYPE* auTBL = nullptr; // 領域確保


  //━━━━━━━━━━━━━━━━━
  // 初期化
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // テーブル初期化
    //----------------------------------
    // アダプタ初期化の直前で実行
    //─────────────────
    void AUTH_INIT_TBL() {
      auTBL = new AU_SLOT_TYPE[ctx.accIDS];
    }

    //─────────────────
    // スロット初期化
    //----------------------------------
    // 通信アダプタの名前空間で派生(名称:INIT_SLOT)
    //─────────────────
    void AUTH_INIT_SLOT(AU_SLOT_TYPE& argSlot){
      argSlot.used       = false ; // 有効性判定リセット(無効)
      argSlot.authCD     = ""    ; // 認証コードをクリア
      argSlot.lastActive = 0     ; // 最終更新時刻をリセット
    } /* AUTH_INIT_SLOT */


  //─────────────────
  // 認証コード生成
  //----------------------------------
  // 仕様:
  // ・AUTH_GROUPS の各グループを最低1回使用
  // ・文字配置はランダム
  // ・余った桁は全グループからランダム選択
  //----------------------------------
  // 戻り値：認証コード
  //─────────────────
  static const char* AUTH_GROUPS[] = { // 文字グループ
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",      // ・アルファベット大文字
    "abcdefghijklmnopqrstuvwxyz",      // ・アルファベット小文字
    "0123456789"                       // ・数字
  };
  static constexpr int AUTH_CD_LENGTH  = 5; // 文字コード長
  static constexpr int AUTH_CHR_GROUPS = sizeof(AUTH_GROUPS) / sizeof(AUTH_GROUPS[0]);
  //----------------------------------
  String AUTH_CREATE_ACD(){
    //┬
    //○前処理
    String code = "";
    //│
    //◎┐各グループから最低1文字取得
    for (int id = 0; id < AUTH_CHR_GROUPS; id++){
      //○カウンタ(グループID)を判定
      //│＼（全グループ処理が完了した場合）
      //│ ▽完了：走査終了
      //│
      //○当該グループの文字セットからランダムな１文字を追加
      int len = strlen(AUTH_GROUPS[id])   ; // データセット長
      code += AUTH_GROUPS[id][random(len)]; // ランダムな１文字を後方マージ
      //┴
    }   /* for */
    //│
    //◎┐不足文字分をランダムに追加
    while (code.length() < AUTH_CD_LENGTH){
      //│＼（上限[認証コード長]に達した場合）
      //│ ▽完了：走査終了
      //│
      //○ランダムなグループの文字セットからランダムな１文字を追加
      int groupID = random(AUTH_CHR_GROUPS)     ; // ランダムなグループ
      int len     = strlen(AUTH_GROUPS[groupID]); // ランダムな文字
      code += AUTH_GROUPS[groupID][random(len)] ; // １文字を後方マージ
      //┴
    }   /* END-while */
    //│
    //◎┐文字位置をシャッフル
    for (int nowID = 0; nowID < code.length(); nowID++){
      //│＼（最終桁に到達した場合）
      //│ ▽完了：走査終了
      //│
      //○当該桁の文字をランダムな桁の文字と入れ替え
      int swapID   = random(code.length()); // 移動元桁数をランダムに取得
      char tmpChar = code[nowID ]         ; // 当該桁の文字を退避
      code[nowID]  = code[swapID]         ; // 当該桁に移動元桁の文字を移送
      code[swapID] = tmpChar              ; // 移動元桁に退避した文字を移送
    }   /* for */
    //│
    //▼認証コードを返す
    return code;
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
    for (int id = 0; id < ctx.accIDS; id++) {
      //│＼（スロットの上限に達した場合）
      //│ ▽完了：走査終了
      //│
      //○一致確認
      if ( auTBL[id].used &&
        millis() - auTBL[id].lastActive > AUTH_TIME_LIMIT) return id;
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
  // 認証コードが一致するかを確認
  //----------------------------------
  // 引数：
  // ・認証コード ：検索キー
  //----------------------------------
  // 戻り値：認証ID(数値型)
  //  既データなし： -1
  //  既データあり： 0～
  //─────────────────
  int GET_EXIST_AID(String argACD){
    //┬
    //◎┐認証情報全体を照合
    for (int id = 0; id < ctx.accIDS; id++){
      //│＼（スロットの上限に達した場合）
      //│ ▽完了：走査終了
      //│
      //○現在の認証情報と照合
      if (auTBL[id].used && auTBL[id].authCD == argACD) {
      //│＼（認証コードが一致)
          //○タイムスタンプを更新
          //▼返却：既データあり
          auTBL[id].lastActive = millis();
          return id;
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
  // 新たな認証コードでスロットを作成
  // 新たな認証コードは認証情報TBL内で一意
  // 空きスロットが無い場合は失敗
  //----------------------------------
  // 戻り値：認証コード(文字列型)
  // ・成功： false
  // ・失敗： true
  //─────────────────
  bool NEW_USER(){
    //┬
    //○前処理
    String retCD = "" ; // 戻り値を[失敗]で初期化
    String newCD = "" ; // 新しい認証コード
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
          //◎┐新しい認証コードを生成
          while (true){
            //●認証コードを生成
            newCD = AUTH_CREATE_ACD();
            //│
            //●重複していないかを確認
            if (GET_EXIST_AID(newCD) < 0){break;}
            // ＼（存在しない場合）
              //▽中断：作成した認証コードを採用
          } /* END-while */
          //│
          //○コンテクストを更新
          ctx.authCD = newCD;
          ctx.accID = freeID;
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
// 処理プロセス（ユーザ認証関連）
//========================================================
  //─────────────────
  // ４．認証を実施・認証開始コマンド応答
  //----------------------------------
  // 戻り値：認証結果（論理値）
  // ・false： 処理継続が可能
  // ・true ： 処理継続が不可
  //----------------------------------
  // 処理継続が可能の場合、早期エラーメッセージにセット
  //─────────────────
  bool F4_CHECK_AUTH(){
    //┬
    //◇┐認証開始要求に応答
    if (ctx.cmdPath == "_START_!" || ctx.cmdPath == "_START_") {
      //├┐（「認証コード発行コマンド」の場合）
        //●認証管理に加える
        if(NEW_USER()){ctx.errMSG = "#SS1!"; return true;}
        //│＼（失敗した場合）
        //│ ▼返却：[1]認証開始に失敗(要レスポンス)
        //│
        //▼RETURN：[2]認証開始に成功(要レスポンス)
        ctx.errMSG = String("$") + ctx.authCD.c_str() + String("$");
        return true;
        //└┐（その他）
        //┴
    } /* END-if */
    //│
    //○ユーザ認証対象を確認
    if (ctx.authCD == ""){ctx.accID = 0; return false;}
    //│＼（認証が不要の場合）
    //│ ○ユーザIDを共用IDにセット
    //│ ▼返却：(1)認証が不要
    //│
    //○ユーザ認証を実施
    ctx.accID = GET_EXIST_AID(ctx.authCD);
    if (ctx.accID < 0){ctx.errMSG = "#SS2!"; return true;}
    //│＼（認証に失敗した場合）
    //│ ▼返却：[3]認証に失敗(要レスポンス)
    //│
    //▼返却：(2)認証に成功
    return false;
  } /* F4_CHECK_AUTH() */