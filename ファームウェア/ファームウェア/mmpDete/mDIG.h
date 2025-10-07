// filename : mDig.h
//========================================================
// モジュール：デジタル入出力
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.01 (2025/10/03)
// - 誤って削除していたPORコマンドを復活
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
    return strcmp(cmd,"POR")==0;
    return strcmp(cmd,"POW")==0;
  }

  //━━━━━━━━━━━━━━━━━
  // コマンド・パーサー(実装)
  //━━━━━━━━━━━━━━━━━
  void handle(char dat[][10], int dat_cnt) override {

    LedScope  scopeLed(ctx, led);   // ＬＥＤ点滅をRAIIガードで実行
    Stream&   sp = MMP_SERIAL(ctx); // 対象ストリーム

    // ───────────────────────────────
    // POR  : 入力バッファ読取り
    // 引数 : ① GPIO番号
    // 戻り : ・正常 [X!!!!]
    //        ・異常 [POR!!]
    // ───────────────────────────────
    if (strcmp(dat[0],"POR") == 0 ){

      // １．前処理：
        // 1.1.書式チェック
      if ( dat_cnt != 2 )
      { ResCmdErr(sp, dat[0]); return; }

        // 1.2.単項目チェック
      long port, val;
      if ( !strHex2long(dat[1], port, 0, 0xFF) )
      { ResCmdErr(sp, dat[0]); return; }

      // ２．デジタル入力：
      pinMode( (int)port, INPUT_PULLUP );
      ResHex4( sp, (int16_t)digitalRead(port) );

      // ３．後処理：
      return;
    }

    // ───────────────────────────────
    // POW  : 入力バッファ読取り
    // 引数 : ① GPIO番号
    // 　　　 ② スイッチID
    // 戻り : ・正常 [X!!!!]
    //        ・異常 [POW!!]
    // ───────────────────────────────
    if (strcmp(dat[0],"POW")==0){

      // １．前処理：
        // 1.1.書式チェック
      if ( dat_cnt != 3 )
      { ResCmdErr(sp, dat[0]); return; }

        // 1.2.単項目チェック
      long port, val;
      if  ( !strHex2long(dat[1], port, 0, 0xFF)  ||
            !strHex2long(dat[2], val,  0, 1   )  )
      { ResCmdErr(sp, dat[0]); return; }

      // ２．デジタル出力：
      pinMode((int)port, OUTPUT);
      digitalWrite((int)port, (int)val);
      sp.print("!!!!!");

      // ３．後処理：
      return;
    }
  }
};
