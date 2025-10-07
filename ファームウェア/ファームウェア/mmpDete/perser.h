// filename : perser.h
//========================================================
// コマンド パーサーのフロント・対象資源の登録
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.01 (2025/10/07)
// - 関連ファイルを統合
//========================================================
#pragma once
#include <vector>
#include "module.h"

#define MMP_INPUT_LINE_LENGTH 96  // リクエスト全体のバッファ長
#define MMP_INPUT_DAT_COUNT   8   // コマンド＋引数の個数
#define MMP_INPUT_DAT_LENGTH  10  // 上記1個あたりの上限バイト数

//━━━━━━━━━━━━━━━━━
// ストリーム(RAIIガード実行)
//━━━━━━━━━━━━━━━━━
class SerialScope {
  MmpContext& ctx;        // コンテクスト
  Stream*     prev;       //
  int         prevClient; //
public:
  //───── コンストラクタ(対象ストリーム) ─────
  SerialScope(MmpContext& c, Stream* s, int clientIndex) : ctx(c){
    prev                = *ctx.serial;
    prevClient          = *ctx.currentClient;
    *ctx.serial         = s;
    *ctx.currentClient  = clientIndex;
  }
  //───── デストラクタ(元のストリーム) ─────
  ~SerialScope(){
    *ctx.serial         = prev;
    *ctx.currentClient  = prevClient;
  }
};

//━━━━━━━━━━━━━━━━━
// パーサー
//━━━━━━━━━━━━━━━━━
class Perser {

  // ストリーム(クライアント)定義
  struct Source {
    Stream*     s;                            // ストリーム オブジェクト
    const char* name;                         // 名称
    int         clientIndex;                  // クライアントID：0=USB, 1=UART0
    char        buf[ MMP_INPUT_LINE_LENGTH ]; // リクエスト受信バッファ
    int         len = 0;                      // リクエスト長カウンタ 
  };

  MmpContext&               ctx;      //コンテクスト
  std::vector<ModuleBase*>  modules;  // モジュール一覧(動的配列)
  std::vector<Source>       sources;  // ストリーム(クライアント)一覧(動的配列)

  //━━━━━━━━━━━━━━━━━
  // コマンド実行
  //━━━━━━━━━━━━━━━━━
  void runCommand(Source& src){

    char dat[ MMP_INPUT_DAT_COUNT ][ MMP_INPUT_DAT_LENGTH ];  // 要素バッファ
    int dat_cnt=0;                                            // 要素数

    // リクエストメッセージを要素分解
    char* tok = strtok(src.buf, ":");
    while( tok && dat_cnt < MMP_INPUT_DAT_COUNT ){
      strncpy(dat[dat_cnt], tok, sizeof(dat[0])-1);
      dat[dat_cnt][sizeof(dat[0])-1] = '\0';
      dat_cnt++;
      tok = strtok(nullptr, ":");
    }

    // 空の場合はリターン
    if (dat_cnt==0) return;

    // ストリームをRAIIガードで実行
    SerialScope scopeSerial(ctx, src.s, src.clientIndex);

    // 各モジュールからコマンドを走査
    for (auto* m: modules){
      // コマンド名が該当すれば、コマンド実行してリターン
      if (m->owns(dat[0])){ m->handle(dat, dat_cnt); return; }
    }

    // 該当コマンドなしの場合はエラーでリターン
    char resp[6] = "!!!?!";
    strncpy(resp, dat[0], 3);
    MMP_SERIAL(ctx).print(resp);
  }

public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  Perser(MmpContext& c): ctx(c) {}

  //━━━━━━━━━━━━━━━━━
  // 対象資源の登録
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // モジュール登録
    //─────────────────
    void addModule(ModuleBase* m){
      modules.push_back(m);           // 要素を追加
    }

    //─────────────────
    // ストリーム登録
    //─────────────────
    void addSource(Stream* s, const char* name, int clientIndex){
      Source src;
      src.s           = s;            // ストリーム オブジェクト
      src.name        = name;         // 名称
      src.clientIndex = clientIndex;  // クライアントID：0=USB, 1=UART0
      src.buf[0]      = 0;            // リクエスト受信バッファ            
      src.len         = 0;            // リクエスト長カウンタ
      sources.push_back(src);         // 要素を追加
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
