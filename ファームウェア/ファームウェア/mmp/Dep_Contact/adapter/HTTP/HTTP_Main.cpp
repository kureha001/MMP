// filename : Dep_Contact/adapter/HTTP/HTTP_Main.cpp
//========================================================
// 接客部門／担当(通信アダプタ)：HTTP（メイン）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/30)
//========================================================

//========================================================
//§最終処理
//========================================================
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // JSON形式でレスポンス
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
    //──────────────────────────
    // 接続元にSON形式でレスポンスする
    //──────────────────────────
    // JSON形式でレスポンス
    inline void JSON_SEND(const String& argJSON) {
      //┬
      //○JSONをレスポンス
      ADD_CROSS(*MY_NET);
      MY_NET->send(200, "application/json; charset=utf-8", argJSON);
      //│
    //●コンテクスト・ログを出力する
      adpFnBase::LOG_CTX();
      //┴
    } /* JSON_SEND() */

    //──────────────────────────
    // コマンド実行結果の型判定(数値型)
    //──────────────────────────
    static bool JSON_IS_VAL(const String& argBody){
      if (argBody.length() != 4) return false;
      int start = (argBody[0]=='-') ? 1 : 0;
      for (int i=start; i<4; ++i){
        if (!isDigit((unsigned char)argBody[i])) return false;
      } // for
      return true;
    } /* JSON_IS_VAL() */

    //──────────────────────────
    // コマンド実行結果の型判定(文字列型)
    //──────────────────────────
    static bool JSON_IS_STR(const String& argBody){
      if (argBody.startsWith("#")) return false;
      if (argBody.startsWith("!")) return false;
      return true;
    } /* JSON_IS_STR() */

    //──────────────────────────
    // コマンド実行結果を変換(数値)
    //──────────────────────────
    static int JSON_CONV_VAL(const String& argBody){
      bool neg = (argBody[0]=='-');
      int v = 0;
      for (int i = neg ? 1 : 0; i < 4; ++i) v = v*10 + (argBody[i]-'0');
      return neg ? -v : v;
    } /* JSON_CONV_VAL() */

    //──────────────────────────
    // 処理結果に該当する説明文を取得
    //──────────────────────────
    static const char* JSON_MSG(const String& argID){

      // 共通のコード
      if (argID == RCD::OK      ) return "OK:戻り値無し"            ;
      if (argID == RCD::NotMod  ) return "NG:機能モジュールが無い"  ;
      if (argID == RCD::NotCmd  ) return "NG:コマンド名が不正"      ;
      if (argID == RCD::ChkErr  ) return "NG:引数チェックで違反"    ;
      if (argID == RCD::IniErr  ) return "NG:データが未初期化"      ;
      if (argID == RCD::DevErr  ) return "NG:使用不可のデバイス"    ;
      if (argID == RCD::FilErr  ) return "NG:ファイル操作が異常終了";
      if (argID == RCD::NoDErr  ) return "NG:データ項目名が不正"    ;
      if (argID == RCD::ValErr  ) return "NG:数値が基底範囲外"      ;

      // ユーザ認証のコード
      if (argID == RCD::AuthErr1) return "NG:認証管理の開始に失敗"  ;
      if (argID == RCD::AuthErr2) return "NG:認証に失敗"            ;

      // アダプタ独自のコード
      if (argID == RCD::OK_Auth ) return "OK:ユーザ認証に成功"      ;
      if (argID == RCD::OK_VAL  ) return "OK:数値"                  ;
      if (argID == RCD::OK_STR  ) return "OK:文字列"                ;
  
      return "NG:その他のエラー";
    } /* JSON_MSG() */

    //──────────────────────────
    // 接続元にMSGをJSON形式でレスポンスする
    //──────────────────────────
    struct JSON_DATA{
      bool    Res = false; // MMPの処理結果      {OK:true | NG:false}
      String  Msg = ""   ; // エラーMSG          {正常の場合は空}
      int     Val = -1000; // 戻値が数値の場合   {-999～9999、対象外は-1000 }
      String  Str = ""   ; // 戻値が文字列の場合 {４バイトの文字列、対象外は空}
    }; /* JSON_DATA */
    //─────────────────
    void SEND_RESULT_JSON(){
      //┬
      //○【前処理】
      JSON_DATA jsDat ;
      String    js    ;
      String    msgID = ctx.base.Result; // 処理結果
      //│
      //◇┐JSON内容編集
      if (ctx.base.CmdPath == SP_CMD_START){
        //├┐（認証コード発行の場合）
          //○MSGIDを独自IDに書き換え
          //○取得値を文字列型にセット
          //○処理結果をセット
          msgID     = RCD::OK_Auth; // 認証開始
          jsDat.Str = ctx.base.Result; // 取得値(文字列)
          jsDat.Res = true        ; // 正常
          //┴

      } else if (msgID == RCD::OK) {
        //├┐（正常系：戻り値なし の場合）
          //○処理結果を正常にセット
          jsDat.Res = true ; // 正常
          //┴

      } else {
        //└┐（その他）
          //◇┐データ型に応じて編集
          String body = msgID.substring(0, msgID.length()-1);
          if (JSON_IS_VAL(body)) {
            //├┐（戻り値が数値型の場合）
              //○MSGIDを独自IDに書き換え
              //○処理結果をセット
              //●取得値を数値型にセット
              msgID = RCD::OK_VAL            ; // 数値型
              jsDat.Val = JSON_CONV_VAL(body); // 取得値(数値)
              jsDat.Res = true               ; // 正常
              //┴

          } else if (JSON_IS_STR(msgID)) {
            //├┐（戻り値が文字列型の場合）
              //○MSGIDを独自IDに書き換え
              //○処理結果をセット
              //●取得値を数値型にセット
              msgID = RCD::OK_STR            ; // 文字列型
              jsDat.Str = ctx.base.Result    ; // 取得値(文字列)
              jsDat.Res = true               ; // 正常
              //┴

          } else {
            //└┐（その他）
              //○処理結果をセット
              jsDat.Res = false              ; // 異常
              //┴
          } //～if 
          //┴
      } //～if 
      //│
      //○メッセージを取得
      jsDat.Msg = JSON_MSG(msgID);
      //│
      //○JSON形式に編集
      js.reserve(160) ; // 予備確保
      js += F("{\"ok\":true"   )                                           ; // 処理結果：HTTP通信の成功
      js += F(",\"source\":\"" ); js += ctx.base.Result.c_str(); js += '"' ; // MMPの戻り値
      js += F(",\"result\":"   ); js += (jsDat.Res ? "true" : "false")     ; // 処理結果：MMPコマンドの成功
      js += F(",\"message\":\""); js += jsDat.Msg; js += '"'               ; // メッセージ
      js += F(",\"value\":"    ); js += String(jsDat.Val)                  ; // 戻値（数値）
      js += F(",\"string\":\"" ); js += jsDat.Str                          ; // 戻値（文字列）
      js += "\"}"               ;
      //│
      //○通信経路にJSON形式でレスポンス
      JSON_SEND(js);
      //┴
    } /* SEND_RESULT_JSON() */

//========================================================
//§受信処理
//========================================================
  //───────────────────────────
  // ルーティング登録
  //───────────────────────────
    //──────────────────────────
    // ハンドラの主処理
    //──────────────────────────
    void HANDLE_CORE() {
      //┬
      //●ＭＭＰコマンドを実行する
      //●接続元に処理結果を送信する
      modeMain::HANDLE();
      IS_JSON ? SEND_RESULT_JSON() : SEND_RESULT("");
      //┴
    } /* HANDLE()() */
