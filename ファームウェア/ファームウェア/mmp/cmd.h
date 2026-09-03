// filename : cmd.h
//========================================================
// コマンド・マネージャ：通信アダプタと機能モジュールを連携する
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/03) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■ＭＭＰシステム
  #include "conf.h"
  #include "context.h"
  //│
  //■ＭＭＰシステム(モジュール群)
  #include "module/_API_.h"   // <<抽象基底クラス>>
  #include "module/system.h"  // システム管理
  #include "module/analog.h"  // アナログ入力
  #include "module/digital.h" // デジタル入出力
  #include "module/pwm.h"     // PWM出力
  #include "module/IIC.h"     // IIC通信
  #include "module/mp3.h"     // MP3プレイヤー
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト
  //─────────────────
  extern MmpContext ctx;


//########################################################
//# 前空間：コマンド・マネージャ
//########################################################
namespace CommandManager {
  //─────────────────
  // 機能モジュール管理
  //─────────────────
    //┬
    //□機能モジュール・コンテナ
    inline std::vector<ModuleBase*> MODULE;
    //│
    //□機能モジュール構造体
    struct T_MOD {
        const char* name; // 名前
        const char* desc; // 説明
    };
    //│
    //□機能モジュールのプロファイル
    static const T_MOD modSYS    = {"SYS"    , "System Management"   };
    static const T_MOD modANA_I  = {"ANALOG" , "Analog Input"        };
    static const T_MOD modDIG_IO = {"DIGITAL", "Digital Input/Output"};
    static const T_MOD modPWM    = {"PWM"    , "PWM Output"          };
    static const T_MOD modIIC    = {"IIC"    , "IIC Read/Write"      };
    static const T_MOD modMP3    = {"MP3"    , "MP3 Player"          };
    //│
    // 機能モジュールのエントリー
    static const T_MOD* const MOD_LIST[] = {
        &modSYS,
        &modANA_I,
        &modDIG_IO,
        &modPWM,
        &modIIC,
        &modMP3,
    };
    //│
    //□機能モジュール総数
    static const size_t MODs = sizeof(MOD_LIST) / sizeof(MOD_LIST[0]);
    //┴

  //─────────────────
  // クライアントからのリクエスト条件
  //─────────────────
  #define REQUEST_LENGTH 96 // リクエスト全体のバッファ長
  #define DAT_COUNT      10 // コマンド＋引数の個数
  #define DAT_LENGTH     20 // トークン最大長（未定義時のフォールバック）

  //─────────────────
  // コマンド・モジュールをアドイン
  //─────────────────
  inline void INIT(){
    //┬
    //○開始表示
    Serial.println("<<モジュールの初期化>>");
    //│
    //○コマンド・モジュールを登録
    // メイン側で固定生成した MmpContext の参照を各モジュールへ
    // 注入・共有することで、ヒープ断片化（動的確保）の防止と、
    // マルチプロトコル環境におけるグローバル変数汚染の回避を
    // 両立させる。
    MODULE.push_back(new ModuleSystem (ctx, modSYS.name   , modSYS.desc   ));
    MODULE.push_back(new ModuleAnalog (ctx, modANA_I.name , modANA_I.desc ));
    MODULE.push_back(new ModuleDigital(ctx, modDIG_IO.name, modDIG_IO.desc));
    MODULE.push_back(new ModulePwm    (ctx, modPWM.name   , modPWM.desc   ));
    MODULE.push_back(new ModuleIIC    (ctx, modIIC.name   , modIIC.desc   ));
    MODULE.push_back(new ModuleMP3    (ctx, modMP3.name   , modMP3.desc   ));
    //│
    //◎┐登録名を表示
    Serial.print(" Add In ->");
    for (auto* mod : MODULE){
      //│＼（全モジュールを走査し終えた場合）
      //│ ▼ループ処理を中断
      //│
      //●モジュール名を表示
      Serial.print(String(" [") + String(mod->getModName()) + String("]"));
      //┴
    } /* END-for */
    //│
    //○終了表示
    Serial.println("");
    //┴
  } /* INIT() */

  //━━━━━━━━━━━━━━━━━
  // モジュール名を表示
  //━━━━━━━━━━━━━━━━━
  inline void SHOW_DESC(String argName){
    //┬
    //◎┐略名に対応する正式名称を取得
    String strDesc = "";
    for (size_t i = 0; i < MODs; ++i){
      //○モジュール定義を取得
      const T_MOD& thisMod = *MOD_LIST[i];
      //│
      //○名称を確認
      if (argName == thisMod.name) {strDesc = thisMod.desc; break;}
      //│＼（一致した場合）
      //│  ▼中断：走査を終了
      //┴
    } /* END-for */
    //│
    //○説明を表示
    Serial.printf("Run : %s\n", (strDesc != "") ? strDesc.c_str() : "(Unknown)");
    //┴
  } /* SHOW_DESC() */

  //─────────────────
  // コマンド実行
  //─────────────────
  inline void RunCommand(){
    //┬
    //①┐コマンドパスを整形
    char pPath[ REQUEST_LENGTH ];
    {
      //◇超過分を削除
      size_t pLen = ctx.cmdPath.length();
      if (pLen >= sizeof(pPath)) pLen = sizeof(pPath) - 1;
      memcpy(pPath, ctx.cmdPath.c_str(), pLen);
      pPath[pLen] = '\0';
      //│
      //◇末尾'!'を除去
      pLen = strlen(pPath);
      if (pLen > 0 && pPath[pLen-1] == '!') pPath[pLen-1] = '\0';
      //┴
    }   /* ① */
    //│
    //②┐コマンドパラメータを取得
    char dat[ DAT_COUNT ][ DAT_LENGTH ]; // 登録バッファ（コマンド、引数１...引数n）
    int  regCount = 0                  ; // 登録数（コマンド名＋引数）
    {
      //○先頭のトークンを取得
      char* tok = strtok(pPath, ":");
      //│
      //◎┐トークン毎を登録バッファに登録
      while (tok && regCount < DAT_COUNT){
        //│＼（データ数の上限を超えた場合）
        //│ ▼ループ処理を中断
        //│
        //○当該トークンを登録バッファに登録
        strncpy(dat[regCount], tok, sizeof(dat[0])-1);
        dat[regCount][sizeof(dat[0])-1] = '\0';
        //│
        //○登録数をカウントアップ
        regCount++;
        //│
        //○次のトークンを取得
        tok = strtok(nullptr, ":");
      } /* while */
        //┴
      //│
      //○エラーメッセージを返却
      if (regCount == 0){ctx.resMSG = "#CMD!"; return;}
        // ＼（登録数がゼロの場合）
          //▼エラーメッセージを返却
      //┴
    }   /* ② */
    //│
    //③┐モジュール機能を実行
      //○レスポンスを初期化
      ctx.resMSG = "";
      //│
      //◎┐モジュールを走査
      for (auto* m : MODULE){
        //│＼（全モジュールを走査し終えた場合）
        //│ ▼ループ処理を中断
        //│
        //◇┐当該モジュールを実行
        if (m->owns(dat[0])){
          //├→(コマンド所有者の場合)
            //●モジュール説明を表示
//          SHOW_DESC(m->getModName());
            //│
            //○モジュールを実行
            m->handle(dat, regCount);          
            //│
            //▼実行結果をリターン
            return;
        } /* END-if */
          //└┐（その他）
            //┴
        //┴
      //┴
      } /* END-for */
    //│
    //○エラーメッセージを返却
    ctx.resMSG = "#NOM!";
    //┴
  } /* RunCommand() */

}; /* namespace CommandManager */