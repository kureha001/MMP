// filename : modDIG.h
//========================================================
// モジュール：デジタル入出力
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/14) α版
//========================================================
#pragma once
#include "mod.h"      // 機能モジュール：抽象基底クラス

//━━━━━━━━━━━━━━━━━
// メイン処理
//━━━━━━━━━━━━━━━━━
class ModuleDigital : public ModuleBase {
public:
  //━━━━━━━━━━━━━━━━━
  // モジュール(抽象基底クラス)
  //━━━━━━━━━━━━━━━━━
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
    Stream&     sp = ctx.vStream;         // 仮想ストリーム
    LedScope    scopeLed(ctx, led);       // コマンド色のLED発光
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正

    // ───────────────────────────────
    // 機能 : 信号入力
    // 書式 : DIGITAL/IN
    // 引数 : ① GPIO番号
    // 戻り : _ResValue
    // ───────────────────────────────
    if (strcmp(Cmd,"INPUT") == 0){

      // １．前処理：
      // 1.1.書式チェック
      if (dat_cnt != 2){_ResChkErr(sp); return;}

      // 1.2.単項目チェック
      int port, val;
      if (!_Str2Int(dat[1], port, 0, 39)){_ResChkErr(sp); return;}

      // ２．デジタル入力：
      pinMode(port, INPUT_PULLUP );
      int res = digitalRead(port);

      // ３．後処理：
      _ResValue(sp, res);
      return;
    }

    // ───────────────────────────────
    // 機能 : 信号出力
    // 書式 : DIGITAL/OUT
    // 引数 : ① GPIO番号
    // 　　　 ② 出力値
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"OUTPUT") == 0){

      // １．前処理：
      // 1.1.書式チェック
      if (dat_cnt != 3){_ResChkErr(sp); return;}

      // 1.2.単項目チェック
      int port, val;
      if  ( !_Str2Int(dat[1], port, 0, 39) ||
            !_Str2Int(dat[2], val,  0, 1 ) ){_ResChkErr(sp); return;}

      // ２．デジタル出力：
      pinMode(port, OUTPUT);
      digitalWrite(port, val);

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