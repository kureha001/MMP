// filename : parser.h
//========================================================
// コマンド パーサー
// - 機能モジュールの登録
// - 機能モジュールへのルーティング
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/13) α版 
//・LED表示処理をこちらに移設
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <Adafruit_NeoPixel.h>
  //│
  //■ＭＭＰシステム
  #include "mod.h"    // 抽象基底クラス
  //★★★ 機能モジュール保守の対応箇所(1/4) ★★★
  #include "modINF.h" // 機能：システム
  #include "modANA.h" // 機能：アナログ入力
  #include "modDIG.h" // 機能：デジタル入出力
  #include "modPWM.h" // 機能：PWM出力
  #include "modI2C.h" // 機能：I2C通信
  #include "modMP3.h" // 機能：MP3プレイヤー
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // RGB-LED
  //─────────────────
  extern Adafruit_NeoPixel INO_PIXEL;

  //─────────────────
  // コンテクスト
  //─────────────────
  extern MmpContext ctx;

  //─────────────────
  // パーサ公開
  //─────────────────
  class  Parser             ; // 前方宣言
  extern Parser* INO_PARSER ; // ※実体の所有者はスケッチ（依存方向制御用公開ポインタ）

  //─────────────────
  // 各アダプタからの進行移譲先
  //─────────────────
  String MMP_REQUEST()      ; // 前方宣言

  //─────────────────
  // クライアントからのリクエスト条件
  //─────────────────
  #define REQUEST_LENGTH  96  // リクエスト全体のバッファ長
  #define DAT_COUNT       10  // コマンド＋引数の個数


//========================================================
// 機能モジュール定義
//========================================================
  struct T_MMP_MOD {
    const char* name;
    uint8_t     r;
    uint8_t     g;
    uint8_t     b;
  };

  namespace MMP_MOD {

    //★★★ 機能モジュール保守の対応箇所(2/4) ★★★
    static const T_MMP_MOD INF    = { "INF"   ,  5,  5,  5 };
    static const T_MMP_MOD ANA_I  = { "ANA_I" , 10,  0, 10 };
    static const T_MMP_MOD DIG_IO = { "DIG_IO", 10,  0,  0 };
    static const T_MMP_MOD PWM    = { "PWM"   ,  0,  0, 50 };
    static const T_MMP_MOD I2C    = { "I2C"   , 10, 10,  0 };
    static const T_MMP_MOD MP3    = { "MP3"   ,  0, 10,  0 };

    //★★★ 機能モジュール保守の対応箇所(3/4) ★★★
    static const T_MMP_MOD* const LIST[] = {
      &INF,
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
class Parser {
  //┬
  //□コンテクスト(ポインタ)
  MmpContext&               ctxRef;     // ※スケッチで依存注入
  //｜
  //□保有情報
  std::vector<ModuleBase*>  mods;       // 機能モジュール群
  //┴

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
    //┬
    //○モジュールベースに登録
    //★★★ 機能モジュール保守の対応箇所(4/4) ★★★
    mods.push_back(new ModuleInfo   (ctxRef, MMP_MOD::INF.name   ));
    mods.push_back(new ModuleAnalog (ctxRef, MMP_MOD::ANA_I.name ));
    mods.push_back(new ModuleDigital(ctxRef, MMP_MOD::DIG_IO.name));
    mods.push_back(new ModulePwm    (ctxRef, MMP_MOD::PWM.name   ));
    mods.push_back(new ModuleI2C    (ctxRef, MMP_MOD::I2C.name   ));
    mods.push_back(new ModuleMP3    (ctxRef, MMP_MOD::MP3.name   ));
    //│
    //○開始表示
    Serial.println("---------------------------");
    Serial.println("<<機能モジュールの初期化>>");
    //│
    //◎┐機能モジュールをログ表示
    for (auto* m : mods){
      //│＼（全機能モジュールを走査し終えた場合）
      //│ ▼ループ処理を中断
      //│
      //●機能名を表示
      Serial.println(String("　installed  : ") + String(m->getModName()));
      //┴
    } /* END-for */
    //│
    //○終了表示
    Serial.println("");
    //┴
} /* Init() */

private:
  //━━━━━━━━━━━━━━━━━
  // 機能名を表示
  //━━━━━━━━━━━━━━━━━
  void SHOW_NAME(const char* argName){
  //┬
  //◎┐機能名に対応するRGB値を取得
  T_MMP_MOD col = {nullptr, 255, 255, 255}; // 初期値
  for (size_t i = 0; i < MMP_MOD::COUNT; ++i){
    const T_MMP_MOD& def = *MMP_MOD::LIST[i];
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
      //◎┐機能モジュールを走査
      for (auto* m : mods){
        //│＼（全機能モジュールを走査し終えた場合）
        //│ ▼ループ処理を中断
        //│
        //◇┐当該機能モジュールを実行
        if (m->owns(dat[0])){
          //├→(コマンドが在籍する場合)
            //●機能名を表示
            //○機能モジュールを実行
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
    return "#CMD!"; //コマンド名不正
    //┴
  } /* RunCommand() */
}; /* class Parser */

//========================================================
// 公開関数
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 各アダプタからの進行移譲先
  //----------------------------------
  // パーサのメソッドをエイリアス
  //━━━━━━━━━━━━━━━━━
  inline String MMP_REQUEST(){
  //┬
  //○コマンド・パース処理
  return INO_PARSER->RunCommand();
  //┴
  } /* MMP_REQUEST() */