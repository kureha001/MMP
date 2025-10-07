// filename : mI2C.h
//========================================================
// モジュール：ＰＷＭ出力
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.00 初版
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
    return strcmp(cmd,"I2W")==0 || strcmp(cmd,"I2R")==0;
  }

  //━━━━━━━━━━━━━━━━━
  // コマンド・パーサー(実装)
  //━━━━━━━━━━━━━━━━━
  void handle(char dat[][10], int dat_cnt) override {

    LedScope  scopeLed(ctx, led);   // ＬＥＤ点滅をRAIIガードで実行
    Stream&   sp = MMP_SERIAL(ctx); // 対象ストリーム

    //----------------------------------------------------------
    // 出力コマンド
    //----------------------------------------------------------
    if (strcmp(dat[0],"I2W")==0){
      if (dat_cnt < 4) { ResCmdErr(sp, "I2W"); return; }
      long addr, reg, val;
      if (!strHex2long(dat[1], addr, 0x00, 0x7F) ||
          !strHex2long(dat[2], reg,  0x00, 0xFF) ||
          !strHex2long(dat[3], val,  0x00, 0xFF)){ ResCmdErr(sp, "I2W"); return; }
      Wire.beginTransmission((int)addr);
      Wire.write((int)reg);
      Wire.write((int)val);
      Wire.endTransmission();
      sp.print("!!!!!");
      return;
    }

    //----------------------------------------------------------
    // 入力コマンド
    //----------------------------------------------------------
    if (strcmp(dat[0],"I2R")==0){
      if (dat_cnt < 3) { ResCmdErr(sp, "I2R"); return; }
      long addr, reg;
      if (!strHex2long(dat[1], addr, 0x00, 0x7F) ||
          !strHex2long(dat[2], reg,  0x00, 0xFF)){ ResCmdErr(sp, "I2R"); return; }
      Wire.beginTransmission((int)addr);
      Wire.write((int)reg);
      Wire.endTransmission(false);
      int n = Wire.requestFrom((int)addr, 1);
      if (n != 1) { ResCmdErr(sp, "I2R"); return; }
      int v = Wire.read();
      ResHex4(sp, (int16_t)v);
      return;
    }
  }
};
