// filename : modANA.h
//========================================================
// モジュール：アナログ入力
//--------------------------------------------------------
// Ver0.6.00 (2025/xx/xx)
//========================================================
#pragma once
#include "mod.h"

//━━━━━━━━━━━━━━━━━
// デバイス情報
//━━━━━━━━━━━━━━━━━
const int g_addressBusGPIOs[4]  = {10, 9, 8, 7}; // アドレス・バス
const int g_dateGPIOs[4]        = { 4, 3, 2, 1}; // データ・バス

//━━━━━━━━━━━━━━━━━
// クライアント情報
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 型
  //─────────────────
  struct AnaClientData {
    int Values[16*4];   // チャンネル別の入力信号
    int SwitchCnt;      // 使用範囲(スイッチ数;デバイス数)
    int PlayerCnt;      // 使用範囲(プレイヤ数;チャンネル数)
  };
  //─────────────────
  // クライアント情報
  //─────────────────
  static AnaClientData* g_ana = nullptr;

//━━━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━━━
void InitAnalog(const MmpContext& ctx) {

  Serial.println(String("---------------------------"));
  Serial.println(String("HC4067バッファを初期化中..."));

  const int count = ctx.clientID_max + 1;         // 要素数＝最大インデックス＋1
  void* p = calloc(count, sizeof(AnaClientData)); // 全要素0で初期化して確保
  if (!p) { return; }
  g_ana = static_cast<AnaClientData*>(p);

  // 既定設定
  for (int i = 0; i < count; ++i) {
    g_ana[i].SwitchCnt = 4;   // 使用範囲(スイッチ数;デバイス数)
    g_ana[i].PlayerCnt = 1;   // 使用範囲(プレイヤ数;チャンネル数)
  }

  Serial.println(String("HC4067バッファを初期化済み"));
}

//━━━━━━━━━━━━━━━━━
// メイン処理
//━━━━━━━━━━━━━━━━━
class ModuleAnalog : public ModuleBase {
public:
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
    Stream&     sp = MMP_REPLY(ctx);      // 返信出力先（経路抽象）
    LedScope    scopeLed(ctx, led);       // コマンド色のLED発光
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正

    // ───────────────────────────────
    // 機能 : セットアップ
    // 書式 : ANALOG/SETUP
    // 引数 : ① プレイヤー数        ※IDより１つ大きい
    // 　　　 ② スイッチ数          ※IDより１つ大きい
    // 　　　 ③ 対象クライアントID  ※未制定はカレント クライアント
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [ANS!!]
    // ───────────────────────────────
    if (strcmp(Cmd,"SETUP") == 0){

      // １．前処理
        // 1.1. 書式
        if (!(dat_cnt == 3 || dat_cnt == 4)){_ResChkErr(sp); return;}

        // 1.2. 単項目チェック
        int plCnt, swCnt;
        if (!_Str2Int(dat[1], plCnt, 1, 16) ||
            !_Str2Int(dat[2], swCnt, 1,  4) ){_ResChkErr(sp); return;}

        // 1.3. 単項目チェック：クライアント指定（任意）
        int ID;
        if (dat_cnt == 3) {
          ID = ctx.clientID;
        } else {
          int id;
          if (!_Str2Int(dat[3], id, 0, ctx.clientID_max))
          {_ResChkErr(sp); return;}
          ID = id;
        }

      // ２．処理
      g_ana[ID].PlayerCnt = plCnt;
      g_ana[ID].SwitchCnt = swCnt;

      // ３．応答
      _ResOK(sp);
    }

    // ───────────────────────────────
    // 機能 : 信号入力（入力バッファに更新）
    // 書式 : ANALOG/IN
    // 引数 : ① 対象クライアントID ※未制定はカレント クライアント
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [ANU!!]
    // ───────────────────────────────
    if (strcmp(Cmd,"INPUT") == 0){

      // １．前処理：
        // 1.1. 書式
        if (!(dat_cnt == 1 || dat_cnt == 2)){_ResChkErr(sp); return;}

        // 1.3. 単項目チェック：クライアント指定（任意）
        int ID;
        if (dat_cnt == 1) {
          ID = ctx.clientID;
        } else {
          if (!_Str2Int(dat[1], ID, 0, ctx.clientID_max))
          {_ResChkErr(sp); return;}
        }

      // ２．処理
      for (int ch = 0; ch < g_ana[ID].PlayerCnt; ch++) {

        // アドレスバスをセット
        for (int i = 0; i < 4; i++) {
          pinMode(g_addressBusGPIOs[i], OUTPUT);
          digitalWrite(g_addressBusGPIOs[i], (ch>>i) & 1);
        }

        delayMicroseconds(10); //時間調整

        // データバスから読取り
        for (int dev = 0; dev < g_ana[ID].SwitchCnt; dev++) {
          const int pin = g_dateGPIOs[dev];
          g_ana[ID].Values[ch*4 + dev] = analogRead(pin);
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
    // 　　　 ③ 対象クライアントID ※未制定はカレント クライアント
    // 戻り : ・正常 [XXXX!]
    //        ・異常 [ANR!!]
    // ───────────────────────────────
    if (strcmp(Cmd,"READ") == 0){

      // １．前処理
        // 1.1. 書式（第3引数は任意）
        if (!(dat_cnt == 3 || dat_cnt == 4)){_ResChkErr(sp); return;}

        // 1.2. 単項目チェック：クライアント指定（任意）
        int ID;
        if (dat_cnt == 3) {
          ID = ctx.clientID;
        } else {
          if (!_Str2Int(dat[3], ID, 0, ctx.clientID_max))
          {_ResChkErr(sp); return;}
        }

        // 1.3. 単項目チェック
        int pl, sw;
        if (!_Str2Int(dat[1], pl, 0, g_ana[ID].PlayerCnt - 1) ||
            !_Str2Int(dat[2], sw, 0, g_ana[ID].SwitchCnt - 1) )
            {_ResChkErr(sp); return;}

        // 1.4.機能チェック
        if (pl >= g_ana[ID].PlayerCnt || sw >= g_ana[ID].SwitchCnt){_ResChkErr(sp); return;}

      // ２．処理
      const int idx = pl * 4 + sw;        // 値のデータ位置
      int res = g_ana[ID].Values[idx];

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
};
