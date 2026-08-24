// filename : cmd.h
//========================================================
// コマンド管理
//--------------------------------------------------------
// - コマンド・モジュールの登録
// - コマンド・モジュールのルーティング
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/23) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  //│
  //■ＭＭＰシステム
  #include "cmdAPI.h" // 抽象基底クラス
  //│
  //★★★ コマンド・モジュール保守の対応箇所(1/4) ★★★
  //■ＭＭＰシステム(モジュール)
  #include "modSYS.h" // システム
  #include "modANA.h" // アナログ入力
  #include "modDIG.h" // デジタル入出力
  #include "modPWM.h" // PWM出力
  #include "modI2C.h" // I2C通信
  #include "modMP3.h" // MP3プレイヤー
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // RGB-LED
  //----------------------------------
  // ※定義・実装：dev.cpp
  //─────────────────
  #include <Adafruit_NeoPixel.h>
  extern Adafruit_NeoPixel INO_PIXEL;

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
  String RUN_COMMAND()      ; // 前方宣言

  //─────────────────
  // クライアントからのリクエスト条件
  //─────────────────
  #define REQUEST_LENGTH 96 // リクエスト全体のバッファ長
  #define DAT_COUNT      10 // コマンド＋引数の個数


//========================================================
// コマンド・モジュール定義
//========================================================
  struct T_MOD {
    const char* name;
    uint8_t     r;
    uint8_t     g;
    uint8_t     b;
  };


//########################################################
//# 専用名の前空間
//########################################################
namespace MMP_MOD {

  //★★★ コマンド・モジュール保守の対応箇所(2/4) ★★★
  static const T_MOD SYS    = { "SYS"    ,  5,  5,  5 };
  static const T_MOD ANA_I  = { "ANALOG" , 10,  0, 10 };
  static const T_MOD DIG_IO = { "DIGITAL", 10,  0,  0 };
  static const T_MOD PWM    = { "PWM"    ,  0,  0, 50 };
  static const T_MOD I2C    = { "I2C"    , 10, 10,  0 };
  static const T_MOD MP3    = { "MP3"    ,  0, 10,  0 };

  //★★★ コマンド・モジュール保守の対応箇所(3/4) ★★★
  static const T_MOD* const LIST[] = {
    &SYS,
    &ANA_I,
    &DIG_IO,
    &PWM,
    &I2C,
    &MP3,
  };

  static const size_t COUNT = sizeof(LIST) / sizeof(LIST[0]);

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
  std::vector<ModuleBase*>  mods;       // コマンド・モジュール群
  //┴

public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  // ※スケッチで実装化
  //━━━━━━━━━━━━━━━━━
  CmdManager(MmpContext& c): ctxRef(c) {}

  //─────────────────
  // コマンド・モジュールの初期化
  //─────────────────
  void START(){
    //┬
    //○開始表示
    Serial.println("<<モジュールの初期化>>");
    //│
    //○コマンド・モジュールを登録
    //★★★ コマンド・モジュール保守の対応箇所(4/4) ★★★
    mods.push_back(new ModuleSystem (ctxRef, MMP_MOD::SYS.name   ));
    mods.push_back(new ModuleAnalog (ctxRef, MMP_MOD::ANA_I.name ));
    mods.push_back(new ModuleDigital(ctxRef, MMP_MOD::DIG_IO.name));
    mods.push_back(new ModulePwm    (ctxRef, MMP_MOD::PWM.name   ));
    mods.push_back(new ModuleI2C    (ctxRef, MMP_MOD::I2C.name   ));
    mods.push_back(new ModuleMP3    (ctxRef, MMP_MOD::MP3.name   ));
    //│
    //◎┐登録名を表示
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
  void SHOW_NAME(const char* argName){
  //┬
  //◎┐モジュール名に対応するRGB値を取得
  T_MOD col = {nullptr, 255, 255, 255}; // 初期値
  for (size_t i = 0; i < MMP_MOD::COUNT; ++i){
    const T_MOD& def = *MMP_MOD::LIST[i];
    if (strcmp(argName, def.name) == 0){ col = def; break; }
  } /* END-for */
  //│
  //○LEDを発光
  INO_PIXEL.setPixelColor(0, INO_PIXEL.Color(col.g, col.r, col.b));
  INO_PIXEL.show();
  //┴
  } /* SHOW_NAME() */

public:
  //─────────────────
  // コマンド実行
  //─────────────────
  String RunCommand(){
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
      if (regCount == 0) return "#CMD!"; // コマンド名不正
        // ＼（登録数がゼロの場合）
          //▼エラーメッセージを返却
      //┴
    }   /* ② */
    //│
    //③┐モジュール機能を実行
      //○仮想出力ストリームを初期化
      ctx.vStream.clear();
      //│
      //◎┐モジュールを走査
      for (auto* m : mods){
        //│＼（全モジュールを走査し終えた場合）
        //│ ▼ループ処理を中断
        //│
        //◇┐当該モジュールを実行
        if (m->owns(dat[0])){
          //├→(コマンド所有者の場合)
            //●モジュール名を表示
            //○モジュールを実行
            //▼実行結果をリターン
            SHOW_NAME(m->getModName());
            m->handle(dat, regCount);          
            return ctx.vStream.str();
        } /* END-if */
          //└┐（その他）
            //┴
        //┴
      //┴
      } /* END-for */
    //│
    //○エラーメッセージを返却
    return "#NOM!"; //モジュール不在
    //┴
  } /* RunCommand() */
}; /* class CmdManager */

//========================================================
// 公開関数
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 各アダプタからの進行移譲先
  //━━━━━━━━━━━━━━━━━
  inline String RUN_COMMAND(){
  //┬
  //○コマンド・パース処理
  return INO_CMD->RunCommand();
  //┴
  } /* RUN_COMMAND() */