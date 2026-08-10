// filename : parser.h
//========================================================
// コマンド パーサー
// - 機能モジュールの登録
// - 機能モジュールへのルーティング
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/10) α版 
//・LED表示処理を移設
//========================================================
#pragma once
#include "adp.h"    // 通信アダプタ共通
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
  //─────────────────
  extern MmpContext ctx           ; // 定義：mod.h、実装：mmp.ino
  extern Adafruit_NeoPixel g_PIXEL; // スケッチの資源を利用

  //─────────────────
  // パーサー本体：前方宣言
  // 外部公開ポインタ：スケッチで定義
  //─────────────────
  class  Parser;
  extern Parser* G_PARSER;

  //─────────────────
  // 統一入口：前方宣言
  //─────────────────
  String MMP_REQUEST();


//━━━━━━━━━━━━━━━━━
// パーサー
//━━━━━━━━━━━━━━━━━
class Parser {

  // 依存性注入
  // ※スケッチで依存注入
  MmpContext&               ctxRef;     // コンテクスト

  // 保有情報
  std::vector<ModuleBase*>  mods;       // 機能モジュール群


public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  // ※スケッチで実装化
  //━━━━━━━━━━━━━━━━━
  Parser(MmpContext& c): ctxRef(c) {}

  //─────────────────
  // パーサーの初期化
  //─────────────────
  void Init(){
    // 機能モジュールを登録
    mods.push_back(new ModuleInfo   (ctxRef, "INFO"   ));
    mods.push_back(new ModuleAnalog (ctxRef, "ANALOG" ));
    mods.push_back(new ModuleDigital(ctxRef, "DIGITAL"));
    mods.push_back(new ModulePwm    (ctxRef, "PWM"    ));
    mods.push_back(new ModuleI2C    (ctxRef, "I2C"    ));
    mods.push_back(new ModuleMP3    (ctxRef, "MP3"    ));
  } /* Init() */

private:
  //━━━━━━━━━━━━━━━━━
  // 機能名表示
  //━━━━━━━━━━━━━━━━━
  void SHOW_NAME(const char* argName){
  //┬
  //○RGB値を初期化
  struct typeColor { uint8_t r,g,b; };
  typeColor col = {255,255,255};
  //│
  //○RGB値を選択
  if      (strcmp(argName, "INFO"   ) == 0) col = {  5,  5,  5};
  else if (strcmp(argName, "ANALOG" ) == 0) col = { 10,  0, 10};
  else if (strcmp(argName, "DIGITAL") == 0) col = { 10,  0,  0};
  else if (strcmp(argName, "PWM"    ) == 0) col = {  0,  0, 50};
  else if (strcmp(argName, "I2C"    ) == 0) col = { 10, 10,  0};
  else if (strcmp(argName, "MP3"    ) == 0) col = {  0, 10,  0};
  //│
  //○RGB値をセット
   g_PIXEL.setPixelColor(
    0,
    g_PIXEL.Color(col.g, col.r, col.b)
  );
  //│
  //○LEDを発光
  g_PIXEL.show();
  //┴
  } /* SHOW_NAME() */

public:
  //─────────────────
  // コマンド実行
  //─────────────────
  String RunCommand(){
    //┬
    //①┐清書したコマンドパスを取得
    char path[ REQUEST_LENGTH ];
    {
      //◇コマンドパスを取込(末尾処理あり)
      size_t pLen = ctx.cmdPath.length();
      if (pLen >= sizeof(path)) pLen = sizeof(path) - 1;
      memcpy(path, ctx.cmdPath.c_str(), pLen);
      path[pLen] = '\0';
      //│
      //◇コマンドパスの末尾に'!'があれば除去(末尾処理あり)
      pLen = strlen(path);
      if (pLen > 0 && path[pLen-1] == '!') path[pLen-1] = '\0';
      //┴
    }   /* ① */
    //│
    //②┐コマンドデータ、データ数を取得
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
      //┴
    }   /* ② */
    //│
    //③┐モジュール機能を実行
      //○仮想出力ストリームを初期化
      ctx.vStream.clear();
      //│
      //◎┐モジュールを走査
      for (auto* m : mods){
        //◇在籍有無に応じて、モジュール機能を実行
        if (m->owns(dat[0])){
        //├→(当該モジュールに在籍する場合)
          //●RGB-LEDを発光
          //○モジュール機能を実行
          //▼RETURN:モジュールの戻り値をリターン
          SHOW_NAME(m->getModName());
          m->handle(dat, dat_cnt);          
          return ctx.vStream.str();
        } /* if */
        //┴
      //┴
      } /* for */
    //│
    //○エラー(未登録コマンド)をリターン
    return "#CMD!";
    //┴
  } /* RunCommand() */
}; /* class Parser */


//━━━━━━━━━━━━━━━━━
// 統一呼び出し
// ユーザID は通信経路から提供される実行コンテキスト情報であり、
// コマンド実行時に一時的に更新する
//━━━━━━━━━━━━━━━━━
inline String MMP_REQUEST(){

  // コマンド・パース処理
  return G_PARSER->RunCommand();

} /* MMP_REQUEST() */