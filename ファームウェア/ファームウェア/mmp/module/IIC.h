// filename : module/IIC.h
//========================================================
// コマンド・モジュール：ＩＩＣ通信
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/03) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <Wire.h>   // IIC通信を扱うため
  //│
  //■ＭＭＰシステム
  //┴
//┴

//========================================================
// メイン処理
//========================================================
class ModuleIIC : public ModuleBase {
public:
  //━━━━━━━━━━━━━━━━━
  // モジュール(抽象基底クラス)
  //━━━━━━━━━━━━━━━━━
  using ModuleBase::ModuleBase;

  //========================================================
  // コマンド・パーサー(実装)
  //========================================================
  void handle(char dat[][ DAT_LENGTH ], int dat_cnt) override {

    //━━━━━━━━━━━━━━━━━
    // 前処理
    //━━━━━━━━━━━━━━━━━
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正

    // ───────────────────────────────
    // 機能 : レジスタ読取
    // 書式 : READ:<IICアドレス>:<レジスタアドレス>!
    // 引数 : ① IICアドレス
    // 　　　 ② レジスタアドレス
    // 制限 : とくになし
    // 戻値 : _ResValue
    // ───────────────────────────────
    if (strcmp(Cmd,"READ") == 0){
      // １．前処理
        // 1.1. 書式
        if (dat_cnt < 3){_ResChkErr(); return;}

        // 1.2. 単項目チェック
        int addr, reg;
        if (!_Str2Int(dat[1], addr, 0x00, 0x7F) ||
            !_Str2Int(dat[2], reg,  0x00, 0xFF)){_ResChkErr(); return;}

      // ２．コマンド実行
      Wire.beginTransmission(addr);
      Wire.write(reg);
      Wire.endTransmission(false);
      int n = Wire.requestFrom(addr, 1);
      if (n != 1){_ResChkErr(); return;}
      int v = Wire.read();

      // ３．後処理：
      _ResValue(v);
      return;
    }

    // ───────────────────────────────
    // 機能 : レジスタ書込
    // 書式 : WRITE:<IICアドレス>:<レジスタアドレス>:<レジスタ値>!
    // 引数 : ① IICアドレス
    // 　　　 ② レジスタアドレス
    // 　　　 ③ レジスタ値(書込値)
    // 制限 : とくになし
    // 戻値 : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"WRITE") == 0){
      // １．前処理
        // 1.1. 書式
        if (dat_cnt < 4){_ResChkErr(); return;}

        // 1.2. 単項目チェック
      int addr, reg, val;
      if (!_Str2Int(dat[1], addr, 0x00, 0x7F) ||
          !_Str2Int(dat[2], reg,  0x00, 0xFF) ||
          !_Str2Int(dat[3], val,  0x00, 0xFF)
        ){_ResChkErr(); return;}

      // ２．コマンド実行
      Wire.beginTransmission(addr);
      Wire.write(reg);
      Wire.write(val);
      Wire.endTransmission();

      // ３．後処理：
      _ResOK();
      return;
    }

  //━━━━━━━━━━━━━━━━━
  // コマンド名エラー
  //━━━━━━━━━━━━━━━━━
  _ResNotCmd();
  return;
  }

//━━━━━━━━━━━━━━━━━
// 内部ヘルパー
//━━━━━━━━━━━━━━━━━
// なし
};
