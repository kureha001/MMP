// filename : mDig.h
//========================================================
// モジュール：デジタル入出力
//-------------------------------------------------------- 
// 変更履歴: Ver0.5.00 (2025/10/13)
// - WEB-API書式対応
//========================================================
#pragma once
#include "module.h"

//━━━━━━━━━━━━━━━━━
// メイン処理
//━━━━━━━━━━━━━━━━━
class ModuleDigital : public ModuleBase {
public:
  using ModuleBase::ModuleBase;

  //━━━━━━━━━━━━━━━━━
  // コマンド在籍確認(実装)
  //━━━━━━━━━━━━━━━━━
  bool owns(const char* cmd) const override {
      return cmd && strncmp(cmd, "DIGITAL", 7) == 0;
  }

  //━━━━━━━━━━━━━━━━━
  // コマンド・パーサー(実装)
  //━━━━━━━━━━━━━━━━━
  void handle(char dat[][ DAT_LENGTH ], int dat_cnt) override {

    //━━━━━━━━━━━━━━━━━
    // 前処理
    //━━━━━━━━━━━━━━━━━
    Stream&     sp = MMP_SERIAL(ctx);     // クライアントのストリーム
    LedScope    scopeLed(ctx, led);       // コマンド色のLED発光
    const char* Cmd = getCommand(dat[0]); // コマンド名を補正

    // ───────────────────────────────
    // 機能 : 信号入力
    // 書式 : DIGITAL/IN
    // 引数 : ① GPIO番号
    // 戻り : ・正常 [X!!!!]
    //        ・異常 [POR!!]
    // ───────────────────────────────
    if (strcmp(dat[0],"INPUT") == 0){

      // １．前処理：
        // 1.1.書式チェック
      if (dat_cnt != 2){ResChkErr(sp); return;}

        // 1.2.単項目チェック
      long port, val;
      if (!strHex2long(dat[1], port, 0, 0xFF)){ResChkErr(sp); return;}

      // ２．デジタル入力：
      pinMode( (int)port, INPUT_PULLUP );
      int16_t res = (int16_t)digitalRead(port);

      // ３．後処理：
      ResHex4(sp, res);
      return;
    }

    // ───────────────────────────────
    // 機能 : 信号出力
    // 書式 : DIGITAL/OUT
    // 引数 : ① GPIO番号
    // 　　　 ② 出力値
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [POW!!]
    // ───────────────────────────────
    if (strcmp(dat[0],"OUTPUT") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt != 3){ResChkErr(sp); return;}

        // 1.2.単項目チェック
      long port, val;
      if  ( !strHex2long(dat[1], port, 0, 0xFF) ||
            !strHex2long(dat[2], val,  0, 1   ) ){ResChkErr(sp); return;}

      // ２．デジタル出力：
      pinMode((int)port, OUTPUT);
      digitalWrite((int)port, (int)val);

      // ３．後処理：
      ResOK(sp);
      return;
    }

  //━━━━━━━━━━━━━━━━━
  // コマンド名エラー
  //━━━━━━━━━━━━━━━━━
  ResNotCmd(sp);
  return;
  }
};
