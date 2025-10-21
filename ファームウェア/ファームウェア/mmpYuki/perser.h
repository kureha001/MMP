// filename : perser.h
//========================================================
// コマンド パーサーのフロント・対象資源の登録
//-------------------------------------------------------- 
// Ver0.5.00 (2025/10/13)
//   - クライアントからのリクエスト条件をmoudule.hへ移管
//========================================================
#pragma once
#include <vector>
#include "module.h"

// 監視対象のモジュール
#include "mINF.h"
#include "mANA.h"
#include "mDIG.h"
#include "mPWM.h"
#include "mI2C.h"
#include "mMP3.h"

// コンテクストのメンバ
Stream*     g_serial        = nullptr;

//━━━━━━━━━━━━━━━━━
// ストリーム(RAIIガード実行)
//━━━━━━━━━━━━━━━━━
class SerialScope {
  MmpContext& ctx;          // コンテクスト
  Stream*     prevSerial;   //
  int         prevClientID; //
public:
  //───── コンストラクタ(対象ストリーム) ─────
  SerialScope(MmpContext& c, Stream* argSerial, int argClientID) : ctx(c){
    prevSerial    = *ctx.serial;
    prevClientID  = ctx.clientID;
    *ctx.serial   = argSerial;
    ctx.clientID  = argClientID;
  }
  //───── デストラクタ(元のストリーム) ─────
  ~SerialScope(){
    *ctx.serial   = prevSerial;
    ctx.clientID  = prevClientID;
  }
};

//━━━━━━━━━━━━━━━━━
// パーサー
//━━━━━━━━━━━━━━━━━
class Perser {

  // ストリーム(クライアント)定義
  struct Source {
    Stream*     s;                      // ストリーム オブジェクト
    const char* name;                   // 名称
    int         clientIndex;            // クライアントID：0=USB, 1=UART0
    char        buf[ REQUEST_LENGTH ];  // リクエスト受信バッファ
    int         len = 0;                // リクエスト長カウンタ 
  };

  // 依存性注入
  MmpContext&               ctx;        //コンテクスト

  // 保有情報
  std::vector<ModuleBase*>  modules;    // 監視対象のモジュール
  std::vector<Source>       sources;    // 監視対象のクライアント

  //━━━━━━━━━━━━━━━━━
  // コマンド実行
  //━━━━━━━━━━━━━━━━━
  void runCommand(Source& src){

    char dat[ DAT_COUNT ][ DAT_LENGTH ];  // 要素バッファ
    int dat_cnt = 0;                      // 要素数

    // リクエストメッセージを要素分解
    char* tok = strtok(src.buf, ":");
    while( tok && dat_cnt < DAT_COUNT ){
      strncpy(dat[dat_cnt], tok, sizeof(dat[0])-1);
      dat[dat_cnt][sizeof(dat[0])-1] = '\0';
      dat_cnt++;
      tok = strtok(nullptr, ":");
    }

    // 空の場合はリターン
    if (dat_cnt == 0) return;

    // ストリームをRAIIガードで実行
    SerialScope scopeSerial(ctx, src.s, src.clientIndex);

    // 各モジュールからコマンドを走査
    for (auto* m: modules){
      // コマンド名が在籍すれば、コマンド実行してリターン
      if (m->owns(dat[0])){ m->handle(dat, dat_cnt); return; }
    }

    // 該当コマンドなしの場合はエラーでリターン
    MMP_SERIAL(ctx).print("#CMD!");
  }

public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  Perser(MmpContext& c): ctx(c) {}

  //━━━━━━━━━━━━━━━━━
  // 対象資源の登録
  //━━━━━━━━━━━━━━━━━
    void Init(){
      // 監視対象のモジュールを登録
      modules.push_back(new ModuleInfo   (ctx, RGB_INFO    ));
      modules.push_back(new ModuleAnalog (ctx, RGB_ANALOG  ));
      modules.push_back(new ModuleDigital(ctx, RGB_DIGITAL ));
      modules.push_back(new ModulePwm    (ctx, RGB_PWM     ));
      modules.push_back(new ModuleI2C    (ctx, RGB_I2C     ));
      modules.push_back(new ModuleDF     (ctx, RGB_MP3     ));

      // 監視対象のクライアントを登録：USB(CDC)用
      addSource(&Serial,  "USB",   0);

      // 監視対象のクライアントを登録：GPIO用
      addSource(&Serial1, "UART0", 1);
    }
    // ───────────────────────
    void addSource(Stream* s, const char* name, int clientIndex){
      Source src;
      src.s           = s;            // ストリーム オブジェクト
      src.name        = name;         // 名称
      src.clientIndex = clientIndex;  // クライアントID：0=USB, 1=UART0
      src.buf[0]      = 0;            // リクエスト受信バッファ            
      src.len         = 0;            // リクエスト長カウンタ
      sources.push_back(src);         // クライアント要素を追加

      // クライアント最大IDをインクリメント
      if (clientIndex > ctx.clientID_max) ctx.clientID_max = clientIndex;
    }

  //━━━━━━━━━━━━━━━━━
  // すべてのクライアントを待ち受ける
  //━━━━━━━━━━━━━━━━━
  void pollAll(){

    // ストリームを順番に処理する
    for (auto& src : sources){

      // ストリームがアクティブの間は繰り返す
      while (src.s && src.s->available()){

        // 1バイト読取り
        char c = (char)src.s->read();

        // リクエスト長カウンタをカウントアップ
        if (src.len < (int)sizeof(src.buf)-1) src.buf[src.len++] = c;

        // 終端に到達
        if (c=='!'){
          src.buf[src.len-1]  = '\0'; // データ末尾をセット
          runCommand(src);            // コマンドを実行
          src.len             = 0;    // リクエスト長カウンタ
        }
      }
    }
  }
};
