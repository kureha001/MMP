// filename : cmd.h
//========================================================
// コマンド管理
//--------------------------------------------------------
//【目的】
// ・コマンド・モジュールの登録
// ・コマンド・モジュールのルーティング
//--------------------------------------------------------
// Ver 1.2.0 (2026/09/02) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■ＭＭＰシステム
  #include "cmd_API.h" // 抽象基底クラス
  //│
  //★★★ コマンド・モジュール保守の対応箇所(1/4) ★★★
  //■ＭＭＰシステム(モジュール群)
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
  // コマンド管理
  //----------------------------------
  // ※定義：ここ、実装：adp.cpp
  //─────────────────
  class  CmdManager         ; // 前方宣言
  extern CmdManager* INO_CMD; // 実体を参照

  //─────────────────
  // 各アダプタからの進行移譲先
  //─────────────────
  void RUN_COMMAND()      ; // 前方宣言

  //─────────────────
  // クライアントからのリクエスト条件
  //─────────────────
  #define REQUEST_LENGTH 96 // リクエスト全体のバッファ長
  #define DAT_COUNT      10 // コマンド＋引数の個数


//========================================================
// コマンド・モジュール定義
//========================================================
  struct T_MOD {
    const char* name; // 名前
    const char* desc; // 説明
  };


//########################################################
//# 前空間：モジュールのリスト
//########################################################
namespace MMP_MOD {

  // 略名、正式名称を定義
  static const T_MOD SYS    = {"SYS"    , "System Management"   };
  static const T_MOD ANA_I  = {"ANALOG" , "Analog Input"        };
  static const T_MOD DIG_IO = {"DIGITAL", "Digital Input/Output"};
  static const T_MOD PWM    = {"PWM"    , "PWM Output"          };
  static const T_MOD IIC    = {"IIC"    , "IIC Read/Write"      };
  static const T_MOD MP3    = {"MP3"    , "MP3 Player"          };

  // モジュールのリストを定義
  static const T_MOD* const MOD_LIST[] = {
    &SYS,
    &ANA_I,
    &DIG_IO,
    &PWM,
    &IIC,
    &MP3,
  };

  static const size_t COUNT = sizeof(MOD_LIST) / sizeof(MOD_LIST[0]);

} /* namespace MMP_MOD */

//========================================================
// クラス：コマンドパーサ
//========================================================
class CmdManager {
  //┬
  //□コンテクスト(ポインタ)
  MmpContext&               ctxRef;     // ※スケッチで依存注入
  //｜
  //□保有情報
  std::vector<ModuleBase*>  mods;       // 登録モジュール群
  //┴

public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  // ※スケッチで実装化
  //━━━━━━━━━━━━━━━━━
  CmdManager(MmpContext& c): ctxRef(c) {}

  //─────────────────
  // コマンド・モジュールをアドイン
  //─────────────────
  void START(){
    //┬
    //○開始表示
    Serial.println("<<モジュールの初期化>>");
    //│
    //○コマンド・モジュールを登録
    mods.push_back(new ModuleSystem (ctxRef, MMP_MOD::SYS.name   , MMP_MOD::SYS.desc   ));
    mods.push_back(new ModuleAnalog (ctxRef, MMP_MOD::ANA_I.name , MMP_MOD::ANA_I.desc ));
    mods.push_back(new ModuleDigital(ctxRef, MMP_MOD::DIG_IO.name, MMP_MOD::DIG_IO.desc));
    mods.push_back(new ModulePwm    (ctxRef, MMP_MOD::PWM.name   , MMP_MOD::PWM.desc   ));
    mods.push_back(new ModuleIIC    (ctxRef, MMP_MOD::IIC.name   , MMP_MOD::IIC.desc   ));
    mods.push_back(new ModuleMP3    (ctxRef, MMP_MOD::MP3.name   , MMP_MOD::MP3.desc   ));
    //│
    //◎┐登録名を表示
    Serial.print(" Add In ->");
    for (auto* m : mods){
      //│＼（全モジュールを走査し終えた場合）
      //│ ▼ループ処理を中断
      //│
      //●モジュール名を表示
      Serial.print(String(" [") + String(m->getModName() + String("]")));
      //┴
    } /* END-for */
    //│
    //○終了表示
    Serial.println("");
    //┴
} /* START() */

private:
  //━━━━━━━━━━━━━━━━━
  // モジュール名を表示
  //━━━━━━━━━━━━━━━━━
  void SHOW_DESC(const char* argName){
  //┬
  //◎┐略名に対応する正式名称を取得
  String strDesc = "";
  for (size_t i = 0; i < MMP_MOD::COUNT; ++i){
    //○モジュール定義を取得
    const T_MOD& thisMod = *MMP_MOD::MOD_LIST[i];
    //│
    //○名称を確認
    if (strcmp(argName, thisMod.name) == 0) {strDesc = thisMod.desc; break;}
    //│＼（一致した場合）
    //│  ▼中断：走査を終了
    //┴
  } /* END-for */
  //│
  //○説明を表示
  Serial.printf("Run : %s\n", (strDesc != "") ? strDesc : "(Unknown)");
  //┴
  } /* SHOW_DESC() */

public:
  //─────────────────
  // コマンド実行
  //─────────────────
  void RunCommand(){
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
      for (auto* m : mods){
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
}; /* class CmdManager */

//========================================================
// 公開関数
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 各アダプタからの進行移譲先
  //━━━━━━━━━━━━━━━━━
  inline void RUN_COMMAND(){
  //┬
  //○コマンド・パース処理
  INO_CMD->RunCommand();
  //┴
  } /* RUN_COMMAND() */