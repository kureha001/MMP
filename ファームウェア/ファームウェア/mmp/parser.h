// filename : parser.h
//========================================================
// コマンド パーサー
// - 機能モジュールの登録
// - 機能モジュールへのルーティング
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/14) α版
//========================================================
#pragma once
#include "cli.h"    // クライアント：共通ユーティリティ
#include "mod.h"    // 機能モジュール：抽象基底クラス
#include "modINF.h" // 機能モジュール：システム
#include "modANA.h" // 機能モジュール：アナログ入力
#include "modDIG.h" // 機能モジュール：デジタル入出力
#include "modPWM.h" // 機能モジュール：PWM出力
#include "modI2C.h" // 機能モジュール：I2C通信
#include "modMP3.h" // 機能モジュール：MP3プレイヤー

//━━━━━━━━━━━━━━━━━
// グローバル資源(宣言)
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト
  // - 型定義：mod.h
  // - 実　装：スケッチ
  //─────────────────
  extern MmpContext ctx;

  //─────────────────
  // パーサー本体：前方宣言
  // 外部公開ポインタ：スケッチで定義
  //─────────────────
  class  Parser;
  extern Parser* g_PARSER;

  //─────────────────
  // 統一入口：前方宣言
  //─────────────────
  String MMP_REQUEST(const String& path, int clientID);


//━━━━━━━━━━━━━━━━━
// パーサー
//━━━━━━━━━━━━━━━━━
class Parser {

  // 依存性注入
  MmpContext&               ctxRef;     // コンテクスト

  // 保有情報
  std::vector<ModuleBase*>  mods;       // 機能モジュール群


public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  Parser(MmpContext& c): ctxRef(c) {}

  //━━━━━━━━━━━━━━━━━
  // パーサーの初期化
  //━━━━━━━━━━━━━━━━━
  void Init(){
    // 機能モジュールを登録
    mods.push_back(new ModuleInfo   (ctxRef, RGB_INFO   ));
    mods.push_back(new ModuleAnalog (ctxRef, RGB_ANALOG ));
    mods.push_back(new ModuleDigital(ctxRef, RGB_DIGITAL));
    mods.push_back(new ModulePwm    (ctxRef, RGB_PWM    ));
    mods.push_back(new ModuleI2C    (ctxRef, RGB_I2C    ));
    mods.push_back(new ModuleMP3    (ctxRef, RGB_MP3    ));
  } /* Init() */

  //━━━━━━━━━━━━━━━━━
  // コマンド実行
  //━━━━━━━━━━━━━━━━━
  String RunCommand(const String& argPath){
    //┬
    //①┐清書したコマンドパスを取得
    char path[ REQUEST_LENGTH ];
    {
      //◇コマンドパスを取込(末尾処理あり)
      size_t pLen = argPath.length();
      if (pLen >= sizeof(path)) pLen = sizeof(path) - 1;
      memcpy(path, argPath.c_str(), pLen);
      path[pLen] = '\0';
      //│
      //◇コマンドパスの末尾に'!'があれば除去(末尾処理あり)
      pLen = strlen(path);
      if (pLen > 0 && path[pLen-1] == '!') path[pLen-1] = '\0';
    } /* ② */
      //┴
    //│
    //③┐コマンドデータ、データ数を取得
    char dat[ DAT_COUNT ][ DAT_LENGTH ];
    int  dat_cnt = 0;
    {
      //○コマンドパスの区切文字の存在確認
      char* tok = strtok(path, ":");
      //│
      //◎┐トークン毎にコマンドデータに追加
      while (tok && dat_cnt < DAT_COUNT){
        //○トークンをコマンドデータに追加(末尾処理あり)
        strncpy(dat[dat_cnt], tok, sizeof(dat[0])-1);
        dat[dat_cnt][sizeof(dat[0])-1] = '\0';
        //│
        //○トークン数をインクリメント
        dat_cnt++;
        //│
        //○コマンドパスの区切文字の存在確認
        tok = strtok(nullptr, ":");
      } /* while */
        //┴
      //│
      //◇エラー(未登録コマンド)をリターン
      if (dat_cnt == 0) return "#CMD!";
    } /* ③ */
      //┴
    //│
    //④┐モジュール機能を実行
      //○仮想出力ストリームを初期化
      ctx.vStream.clear();
      //│
      //◎┐モジュールを走査
      for (auto* m : mods){
        //◇在籍有無に応じて、モジュール機能を実行
        if (m->owns(dat[0])){
        //├→(当該モジュールに在籍する場合)
          //○モジュール機能を実行
          //▼RETURN:モジュールの戻り値をリターン
          m->handle(dat, dat_cnt);
          return ctx.vStream.str();
        } /* if */
        //┴
      } /* for */
      //┴
    //│
    //○エラー(未登録コマンド)をリターン
    return "#CMD!";
    //┴
  } /* RunCommand() */
}; /* class Parser */


//━━━━━━━━━━━━━━━━━
// 統一呼び出し
//━━━━━━━━━━━━━━━━━
inline String MMP_REQUEST(const String& argPath, int argClientID){

  // クライアントIDを更新
  ctx.clientID = argClientID;

  // コマンド・パース処理
  return g_PARSER->RunCommand(argPath);

} /* MMP_REQUEST() */