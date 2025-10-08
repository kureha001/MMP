// filename : mAna.h
//========================================================
// モジュール：アナログ入力
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.01 (2025/10/08)
//  - 初期化処理をスケッチ側から移管
//  - クライアント別で使い分けが可能
//  - 別クライアントの制御が可能
//========================================================
#pragma once
#include "module.h"

//─────────────────
// GPIO
//─────────────────
const int g_addressBusGPIOs[4]  = {10, 11, 12, 13};  // アドレス・バス
const int g_dateGPIOs[4]        = {26, 27, 28, 29};  // データ・バス

//─────────────────
// クライアント別データ
//─────────────────
struct AnaClientData {
  int Values[16*4];
  int SwitchCnt;
  int PlayerCnt;
};
static AnaClientData* g_ana = nullptr;  // InitAnalog() で確保


//━━━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━━━
void InitAnalog(const MmpContext& ctx) {

  const int count = ctx.clientID_max + 1;         // 要素数＝最大インデックス＋1
  void* p = calloc(count, sizeof(AnaClientData)); // 0 初期化で確保
  if (!p) { return; }
  g_ana = static_cast<AnaClientData*>(p);

  // 既定設定（従来どおり）
  for (int i = 0; i < count; ++i) {
    // Values は calloc により 0 初期化済み
    g_ana[i].SwitchCnt = 4;
    g_ana[i].PlayerCnt = 1;
  }
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
    return  strcmp(cmd,"ANS")==0  ||
            strcmp(cmd,"ANU")==0  ||
            strcmp(cmd,"ANR")==0  ;
  }

  //━━━━━━━━━━━━━━━━━
  // コマンド・パーサー(実装)
  //━━━━━━━━━━━━━━━━━
  void handle(char dat[][10], int dat_cnt) override {

    // スコープ
    LedScope  scopeLed(ctx, led);

    // カレント・クライアント
    Stream&   sp = MMP_SERIAL(ctx);

    // ───────────────────────────────
    // ANS  : 使用範囲設定
    // 引数 : ① プレイヤー数
    // 　　　 ② スイッチ数
    // 　　　 ③ 対象クライアントID ※未制定はカレント クライアント
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [ANS!!]
    // ───────────────────────────────
    if (strcmp(dat[0],"ANS")==0){

      // １．前処理
        // 1.1. 書式
        if (!(dat_cnt==3 || dat_cnt==4))
        { ResCmdErr(sp, dat[0]); return; }

        // 1.2. 単項目チェック
        long plCnt, swCnt;
        if (!strHex2long(dat[1], plCnt, 0, 15) ||
            !strHex2long(dat[2], swCnt, 0,  3)  )
            { ResCmdErr(sp, dat[0]); return; }

        // 1.3. 単項目チェック：クライアント指定（任意）
        long ID;
        if (dat_cnt == 3) {
          ID = ctx.clientID;
        } else {
          long id;
          if (!strHex2long(dat[3], id, 0, (long)ctx.clientID_max))
          { ResCmdErr(sp, dat[0]); return; }
          ID = id;
        }

      // ２．処理
      if (g_ana == nullptr) { ResCmdErr(sp, dat[0]); return; }
      g_ana[ID].PlayerCnt = (int)plCnt;
      g_ana[ID].SwitchCnt = (int)swCnt;

      // ３．応答
      sp.print("!!!!!");
    }

    // ───────────────────────────────
    // ANU  : 入力バッファ更新(使用範囲)
    // 引数 : ① 対象クライアントID ※未制定はカレント クライアント
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [ANU!!]
    // ───────────────────────────────
    if (strcmp(dat[0],"ANU")==0){

      // １．前処理：
        // 1.1. 書式
        if (!(dat_cnt==1 || dat_cnt==2))
        { ResCmdErr(sp, dat[0]); return; }

        // 1.3. 単項目チェック：クライアント指定（任意）
        long ID;
        if (dat_cnt == 1) {
          ID = ctx.clientID;
        } else {
          if (!strHex2long(dat[1], ID, 0, (long)ctx.clientID_max))
          { ResCmdErr(sp, dat[0]); return; }
        }

      // ２．処理
      for (int ch = 0; ch < g_ana[ID].PlayerCnt; ch++) {

        // アドレスバスをセット
        for (int i = 0; i < 4; i++) {
          pinMode(g_addressBusGPIOs[i], OUTPUT);
          digitalWrite(g_addressBusGPIOs[i], (ch>>i) & 1);
        }

        //delayMicroseconds(10); //時間調整

        // データバスから読取り
        for (int dev = 0; dev < g_ana[ID].SwitchCnt; dev++) {
          const int pin = g_dateGPIOs[dev];
          g_ana[ID].Values[ch*4 + dev] = analogRead(pin);
        }
      }

      // ３．後処理：
      sp.print("!!!!!");
    }

    // ───────────────────────────────
    // ANR  : 入力バッファ読取り
    // 引数 : ① プレイヤーID
    // 　　　 ② スイッチID
    // 　　　 ③ 対象クライアントID ※未制定はカレント クライアント
    // 戻り : ・正常 [XXXX!]
    //        ・異常 [ANR!!]
    // ───────────────────────────────
    if (strcmp(dat[0],"ANR")==0){

      // １．前処理
        // 1.1. 書式（第3引数は任意）
        if (!(dat_cnt==3 || dat_cnt==4))
        { ResCmdErr(sp, dat[0]); return; }

        // 1.2. 単項目チェック
        long pl, sw;
        if (!strHex2long(dat[1], pl, 0, 15) ||
            !strHex2long(dat[2], sw, 0,  3)  )
            { ResCmdErr(sp, dat[0]); return; }

        // 1.3. 単項目チェック：クライアント指定（任意）
        long ID;
        if (dat_cnt == 3) {
          ID = ctx.clientID;
        } else {
          if (!strHex2long(dat[3], ID, 0, (long)ctx.clientID_max))
          { ResCmdErr(sp, dat[0]); return; }
        }

        // 1.4.機能チェック
        if (pl >= g_ana[ID].PlayerCnt || sw >= g_ana[ID].SwitchCnt)
        { ResCmdErr(sp, dat[0]); return;}

      // ２．処理
      const int idx = (int)pl * 4 + (int)sw;        // 値のデータ位置
      ResHex4( sp, (int16_t)g_ana[ID].Values[idx] );
    }
  }
};