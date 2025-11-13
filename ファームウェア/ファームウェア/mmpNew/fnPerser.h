// filename : fnPerser.h
//========================================================
// コマンド パーサーのフロント・対象資源の登録
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/11) 初版
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
  // コンテクスト：スケッチで定義
  //─────────────────
  extern MmpContext ctx;

  //─────────────────
  // パーサー本体：前方宣言
  // 外部公開ポインタ：スケッチで定義
  //─────────────────
  class  Perser;
  extern Perser* g_PERSER;

  //─────────────────
  // 統一入口：前方宣言
  //─────────────────
  String MMP_REQUEST(const String& path, int clientID);


//━━━━━━━━━━━━━━━━━
// クライアント情報(RAIIガード)
//━━━━━━━━━━━━━━━━━
class ReplyScope {

  // データ退避ワーク
  MmpContext& ctxRef      ; // コンテキスト
  Stream*     oldVStream  ; // 仮想出力ストリーム
  int         oldClientID ; // クライアントID

public:
  //─────本体シグネチャ─────
  ReplyScope(
    MmpContext& c           , // コンテキスト
    Stream*     argVStream  , // 仮想出力ストリーム
    int         argClientID   // クライアントID
  ):
  //─────既存データ退避─────
  ctxRef(c)                 , // コンテキスト
  oldVStream(*c.vStream)    , // 仮想出力ストリーム
  oldClientID(c.clientID)     // クライアントID
  //─────コンストラクタ─────
  {
    *ctxRef.vStream = argVStream  ; // 仮想出力ストリーム
    ctxRef.clientID = argClientID ; // クライアントID
  }
  //───── デストラクタ ─────
  ~ReplyScope()
  {
    *ctxRef.vStream = oldVStream  ; // 仮想出力ストリーム
    ctxRef.clientID = oldClientID ; // クライアントID
  }
  //─────────────────
}; /* class ReplyScope */


//━━━━━━━━━━━━━━━━━
// パーサー
//━━━━━━━━━━━━━━━━━
class Perser {

  // 依存性注入
  MmpContext&               ctxRef;     // コンテクスト

  // 保有情報
  std::vector<ModuleBase*>  mods;       // 機能モジュール群

  // 仮想出力ストリーム(モジュールのprintを蓄積)
  class StringStream : public Stream {
    String out;
    public:
      size_t  write(uint8_t c) override { out += (char)c; return 1; }
      int     available()      override { return  0; }
      int     read()           override { return -1; }
      int     peek()           override { return -1; }
      void    flush()          override {}
      String  str()            const    { return out; }
  }; /* StringStream:Stream{} */

public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  Perser(MmpContext& c): ctxRef(c) {}

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
  String ExecuteString(const String& argPath, int argClientID){
    //┬
    //①┐ワークバッファへ安全コピー
    char buf[ REQUEST_LENGTH ];
    {
      //◇
      size_t n = argPath.length();
      if (n >= sizeof(buf)) n = sizeof(buf) - 1;
      memcpy(buf, argPath.c_str(), n);
      buf[n] = '\0';
    } /* ① */
      //┴
    //│
    //②┐末尾 '!'があれば除去
    {
      //◇
      size_t n = strlen(buf);
      if (n > 0 && buf[n-1] == '!') buf[n-1] = '\0';
    } /* ② */
      //┴
    //│
    //③┐トークンをコマンド・引数に分解
    char dat[ DAT_COUNT ][ DAT_LENGTH ];
    int  dat_cnt = 0;
    {
      //○区切文字の存在確認
      char* tok = strtok(buf, ":");
      //│
      //◎┐区切文字で分解
      while (tok && dat_cnt < DAT_COUNT){
        //○区切文字までをトークン追加
        strncpy(dat[dat_cnt], tok, sizeof(dat[0])-1);
        dat[dat_cnt][sizeof(dat[0])-1] = '\0';
        //│
        //○トークン数をインクリメント
        dat_cnt++;
        //│
        //○区切文字の存在確認
        tok = strtok(nullptr, ":");
      } /* while */
        //┴
      //│
      //◇
      if (dat_cnt == 0) return "#CMD!";
    } /* ③ */
      //┴
    //│
    //④┐モジュール機能を実行
    StringStream vs;  //仮想出力ストリーム
    {
      //○RAIIガード
      ReplyScope scope(ctxRef, &vs, argClientID);
      //│
      //◎┐モジュールを走査
      for (auto* m : mods){
        //◇在籍有無に応じて、モジュール機能を実行
        if (m->owns(dat[0])){
        //├→(当該モジュールに在籍する場合)
          //○モジュール機能を実行
          //▼RETURN:モジュールの戻り値をリターン
          m->handle(dat, dat_cnt);
          return vs.str();
        } /* if */
        //┴
      } /* for */
      //┴
    } /* ④ */
      //┴
    //│
    //⑤エラー（未登録コマンド）をリターン
    return "#CMD!";
    //┴
  } /* ExecuteString() */
}; /* class Perser */


//━━━━━━━━━━━━━━━━━
// 統一呼び出し
//━━━━━━━━━━━━━━━━━
inline String MMP_REQUEST(const String& argPath, int argClientID){

  // コマンド・パース処理
  return g_PERSER->ExecuteString(argPath, argClientID);

} /* MMP_REQUEST() */