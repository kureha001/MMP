// filename : modI2C.h
//========================================================
// モジュール：ＰＷＭ出力
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/11) 初版
//========================================================
#pragma once
#include "mod.h"
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
    Stream&     sp = MMP_REPLY(ctx);      // 返信出力先（経路抽象）
    LedScope    scopeLed(ctx, led);       // コマンド色のLED発光
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正

    // ───────────────────────────────
    // 機能 : レジスタ読取
    // 書式 : READ:<I2Cアドレス>:<レジスタアドレス>!
    // 引数 : ① I2Cアドレス
    // 　　　 ② レジスタアドレス
    // 制限 : とくになし
    // 戻値 : _ResValue
    // ───────────────────────────────
    if (strcmp(Cmd,"READ") == 0){
      // １．前処理
        // 1.1. 書式
        if (dat_cnt < 3){_ResChkErr(sp); return;}

        // 1.2. 単項目チェック
        int addr, reg;
        if (!_Str2Int(dat[1], addr, 0x00, 0x7F) ||
            !_Str2Int(dat[2], reg,  0x00, 0xFF)){_ResChkErr(sp); return;}

      // ２．コマンド実行
      Wire.beginTransmission(addr);
      Wire.write(reg);
      Wire.endTransmission(false);
      int n = Wire.requestFrom(addr, 1);
      if (n != 1){_ResChkErr(sp); return;}
      int v = Wire.read();

      // ３．後処理：
      _ResValue(sp, v);
      return;
    }

    // ───────────────────────────────
    // 機能 : レジスタ書込
    // 書式 : WRITE:<I2Cアドレス>:<レジスタアドレス>:<レジスタ値>!
    // 引数 : ① I2Cアドレス
    // 　　　 ② レジスタアドレス
    // 　　　 ③ レジスタ値(書込値)
    // 制限 : とくになし
    // 戻値 : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"WRITE") == 0){
      // １．前処理
        // 1.1. 書式
        if (dat_cnt < 4){_ResChkErr(sp); return;}

        // 1.2. 単項目チェック
      int addr, reg, val;
      if (!_Str2Int(dat[1], addr, 0x00, 0x7F) ||
          !_Str2Int(dat[2], reg,  0x00, 0xFF) ||
          !_Str2Int(dat[3], val,  0x00, 0xFF)
        ){_ResChkErr(sp); return;}

      // ２．コマンド実行
      Wire.beginTransmission(addr);
      Wire.write(reg);
      Wire.write(val);
      Wire.endTransmission();

      // ３．後処理：
      _ResOK(sp);
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
