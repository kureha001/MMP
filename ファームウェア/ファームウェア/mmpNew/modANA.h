// filename : modANA.h
//========================================================
// モジュール：アナログ入力
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/14) α版
//========================================================
#pragma once
#include "mod.h"  // 機能モジュール：抽象基底クラス
#include "cli.h"  // クライアント：共通ユーティリティ

//━━━━━━━━━━━━━━━
// グローバル変数
//━━━━━━━━━━━━━━━
const int g_ADDR_PINS[4] = {10, 9, 8, 7}; // アドレス・バス
const int g_DATA_PINS[4] = { 4, 3, 2, 1}; // データ・バス

//━━━━━━━━━━━━━━━━━
// クライアント別データ
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 型宣言
  //─────────────────
  struct AnaClientData {
    int Values[16*4];   // チャンネル別の入力信号
    int SwitchCnt;      // 使用範囲(スイッチ数;デバイス数)
    int PlayerCnt;      // 使用範囲(プレイヤ数;チャンネル数)
  };
  //─────────────────
  // 入力バッファ
  //─────────────────
  static AnaClientData* g_ANA_DAT = nullptr;


//━━━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━━━
void InitAnalog(const MmpContext& ctx) {

  Serial.println("---------------------------");
  Serial.println("[HC4067 buffer initialize]");

  // クライアント別データのメモリ確保
  void* p = calloc(MAX_CLIENTS, sizeof(AnaClientData)); // 全要素0で初期化して確保
  if (!p) { return; }
  g_ANA_DAT = static_cast<AnaClientData*>(p);

  // 既定設定
  for (int i = 0; i < MAX_CLIENTS; ++i) {
    g_ANA_DAT[i].SwitchCnt = 4;   // 使用範囲(スイッチ数;デバイス数)
    g_ANA_DAT[i].PlayerCnt = 1;   // 使用範囲(プレイヤ数;チャンネル数)
  }

  Serial.println(String("　Device  ID : 0 ～ 3 "));
  Serial.println(String("　Channel ID : 0 ～ 16"));
}


//━━━━━━━━━━━━━━━━━
// メイン処理
//━━━━━━━━━━━━━━━━━
class ModuleAnalog : public ModuleBase {
public:
  //━━━━━━━━━━━━━━━━━
  // モジュール(抽象基底クラス)
  //━━━━━━━━━━━━━━━━━
  using ModuleBase::ModuleBase;

  //━━━━━━━━━━━━━━━━━
  // コマンド在籍確認(実装)
  //━━━━━━━━━━━━━━━━━
  bool owns(const char* cmd) const override {
      return cmd && strncmp(cmd, "ANALOG", 6) == 0;
  }

  //━━━━━━━━━━━━━━━━━
  // コマンド・パーサー(実装)
  //━━━━━━━━━━━━━━━━━
  void handle(char dat[][ DAT_LENGTH ], int dat_cnt) override {

    //━━━━━━━━━━━━━━━━━
    // 前処理
    //━━━━━━━━━━━━━━━━━
    Stream&     sp = ctx.vStream;         // 仮想ストリーム
    LedScope    scopeLed(ctx, led);       // モジュール別LED色で発光
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正

    // ───────────────────────────────
    // 機能 : セットアップ
    // 書式 : ANALOG/SETUP
    // 引数 : ① プレイヤー数        ※IDより１つ大きい
    // 　　　 ② スイッチ数          ※IDより１つ大きい
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"SETUP") == 0){

      // １．前処理
        // 1.1. 書式
        if (dat_cnt != 3){_ResChkErr(sp); return;}

        // 1.2. 単項目チェック
        int plCnt, swCnt;
        if (!_Str2Int(dat[1], plCnt, 1, 16) ||
            !_Str2Int(dat[2], swCnt, 1,  4) ){_ResChkErr(sp); return;}

      // ２．処理
      int ID = ctx.clientID;
      g_ANA_DAT[ID].PlayerCnt = plCnt;
      g_ANA_DAT[ID].SwitchCnt = swCnt;

      // ３．応答
      _ResOK(sp);
    }

    // ───────────────────────────────
    // 機能 : 信号入力（入力バッファに更新）
    // 書式 : ANALOG/IN
    // 引数 : なし
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"INPUT") == 0){

      // １．前処理：
        // 1.1. 書式
        if (dat_cnt != 1){_ResChkErr(sp); return;}

      // ２．処理
      int ID = ctx.clientID;
      for (int ch = 0; ch < g_ANA_DAT[ID].PlayerCnt; ch++) {

        // アドレスバスをセット
        for (int i = 0; i < 4; i++) {
          pinMode(g_ADDR_PINS[i], OUTPUT);
          digitalWrite(g_ADDR_PINS[i], (ch>>i) & 1);
        }

        delayMicroseconds(10); //時間調整

        // データバスから読取り
        for (int dev = 0; dev < g_ANA_DAT[ID].SwitchCnt; dev++) {
          const int pin = g_DATA_PINS[dev];
          g_ANA_DAT[ID].Values[ch*4 + dev] = analogRead(pin);
        }
      }

      // ３．後処理：
      _ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 機能 : 入力バッファ参照
    // 書式 : ANALOG/READ
    // 引数 : ① プレイヤーID
    // 　　　 ② スイッチID
    // 戻り : _ResValue
    // ───────────────────────────────
    if (strcmp(Cmd,"READ") == 0){

      // １．前処理
        // 1.1. 書式（第3引数は任意）
        if (dat_cnt != 3){_ResChkErr(sp); return;}

        // 1.2. 単項目チェック
        int pl, sw;
        int ID = ctx.clientID;
        if (!_Str2Int(dat[1], pl, 0, g_ANA_DAT[ID].PlayerCnt - 1) ||
            !_Str2Int(dat[2], sw, 0, g_ANA_DAT[ID].SwitchCnt - 1) )
            {_ResChkErr(sp); return;}

        // 1.4.機能チェック
        if (pl >= g_ANA_DAT[ID].PlayerCnt || sw >= g_ANA_DAT[ID].SwitchCnt){_ResChkErr(sp); return;}

      // ２．処理
      const int idx = pl * 4 + sw;        // 値のデータ位置
      int res = g_ANA_DAT[ID].Values[idx];

      // ３．後処理：
      _ResValue(sp, res);
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
