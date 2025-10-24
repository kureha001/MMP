// filename : modINF.h
//========================================================
// モジュール：システム情報
//--------------------------------------------------------
// Ver 0.6.0 (2025/xx/xx)
//========================================================
#pragma once
#include "mod.h"

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
      return cmd && strncmp(cmd, "INFO", 4) == 0;
  }

  //━━━━━━━━━━━━━━━━━
  // コマンド・パーサー(実装)
  //━━━━━━━━━━━━━━━━━
  void handle(char dat[][ DAT_LENGTH ], int dat_cnt) override {

    //━━━━━━━━━━━━━━━━━
    // 前処理
    //━━━━━━━━━━━━━━━━━
    Stream&     sp = MMP_REPLY(ctx);      // 返信出力先（経路抽象）
    LedScope    scopeLed(ctx, led);       // コマンド色のLED発光
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正

    // ───────────────────────────────
    // VER  : バージョン
    // 引数 : なし
    // 戻り : ・正常 [NNNN!]：メジャー1桁 マイナー1桁 リビジョン2桁
    //        ・異常 [VER!!]
    // ───────────────────────────────
    if (strcmp(Cmd,"VERSION") == 0){

      // １．前処理：
        // 1.1.書式チェック
      if (dat_cnt != 1){_ResChkErr(sp); return; }

      // ２．バージョン(文字列)を返す
      sp.print(ctx.version);

      // ３．後処理：
      return;
    }

  //━━━━━━━━━━━━━━━━━
  // コマンド名エラー
  //━━━━━━━━━━━━━━━━━
  _ResNotCmd(sp);
  return;
  }

//━━━━━━━━━━━━━━━━━
// 内部ヘルパー
//━━━━━━━━━━━━━━━━━
// なし
};
