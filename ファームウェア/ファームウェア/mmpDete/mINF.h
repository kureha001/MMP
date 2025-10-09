// filename : mInf.h
//========================================================
// モジュール：システム情報
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.01 (2025/10/07)
// - VERコマンドを名称でパースする方式に修正
//========================================================
#pragma once
#include "module.h"

//━━━━━━━━━━━━━━━━━
// メイン処理
//━━━━━━━━━━━━━━━━━
class ModuleInfo : public ModuleBase {
public:
  using ModuleBase::ModuleBase;

  //━━━━━━━━━━━━━━━━━
  // コマンド在籍確認(実装)
  //━━━━━━━━━━━━━━━━━
  bool owns(const char* cmd) const override {
    return strcmp(cmd,"VER")==0;
  }

  //━━━━━━━━━━━━━━━━━
  // コマンド・パーサー(実装)
  //━━━━━━━━━━━━━━━━━
  void handle(char dat[][10], int dat_cnt) override {

    // コンテクストの依存性注入
    Stream&   sp = MMP_SERIAL(ctx); // クライアントのストリーム

    // スコープ
    LedScope  scopeLed(ctx, led);   // コマンド色のLED発光

    // ───────────────────────────────
    // VER  : バージョン
    // 引数 : なし
    // 戻り : ・正常 [NNNN!]：メジャー1桁 マイナー1桁 リビジョン2桁
    //        ・異常 [VER!!]
    // ───────────────────────────────
    if (strcmp(dat[0],"VER") == 0 ){

      // １．前処理：
        // 1.1.書式チェック
      if ( dat_cnt != 1 )
      { ResCmdErr(sp, dat[0]); return; }

      // ２．バージョン(文字列)を返す
      sp.print(ctx.version);
      //MMP_SERIAL(ctx).print(ctx.version);
    }
  }
};
