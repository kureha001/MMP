// filename : mI2C.h
//========================================================
// モジュール：ＰＷＭ出力
//-------------------------------------------------------- 
// 変更履歴: Ver0.5.00 (2025/10/13)
// - WEB-API書式対応
//========================================================
#pragma once
#include "module.h"
#include <Wire.h>       // i2c通信を扱うため

//━━━━━━━━━━━━━━━━━
// メイン処理
//━━━━━━━━━━━━━━━━━
class ModuleI2C : public ModuleBase {
public:
  using ModuleBase::ModuleBase;

  //━━━━━━━━━━━━━━━━━
  // コマンド在籍確認(実装)
  //━━━━━━━━━━━━━━━━━
  bool owns(const char* cmd) const override {
      return cmd && strncmp(cmd, "I2C", 3) == 0;
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

    //----------------------------------------------------------
    // 入力コマンド
    //----------------------------------------------------------
    if (strcmp(Cmd,"READ") == 0){
      // １．前処理
        // 1.1. 書式
        if (dat_cnt < 3){ResChkErr(sp); return;}

        // 1.2. 単項目チェック
        long addr, reg;
        if (!strHex2long(dat[1], addr, 0x00, 0x7F) ||
            !strHex2long(dat[2], reg,  0x00, 0xFF)){ResChkErr(sp); return;}

      // ２．コマンド実行
      Wire.beginTransmission((int)addr);
      Wire.write((int)reg);
      Wire.endTransmission(false);
      int n = Wire.requestFrom((int)addr, 1);
      if (n != 1){ResChkErr(sp); return;}
      int v = Wire.read();

      // ３．後処理：
      ResHex4(sp, (int16_t)v);
      return;
    }

    //----------------------------------------------------------
    // 出力コマンド
    //----------------------------------------------------------
    if (strcmp(Cmd,"WRITE") == 0){
      // １．前処理
        // 1.1. 書式
        if (dat_cnt < 4){ResChkErr(sp); return;}

        // 1.2. 単項目チェック
      long addr, reg, val;
      if (!strHex2long(dat[1], addr, 0x00, 0x7F) ||
          !strHex2long(dat[2], reg,  0x00, 0xFF) ||
          !strHex2long(dat[3], val,  0x00, 0xFF)){ResChkErr(sp); return;}

      // ２．コマンド実行
      Wire.beginTransmission((int)addr);
      Wire.write((int)reg);
      Wire.write((int)val);
      Wire.endTransmission();

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
