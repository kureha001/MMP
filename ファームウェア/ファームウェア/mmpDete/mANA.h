// filename : mAna.h
//========================================================
// モジュール：アナログ入力
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.00 初版
//========================================================
#pragma once
#include "module.h"

//─────────────────
// GPIO
//─────────────────
const int g_addressBusGPIOs[4] = {10, 11, 12, 13};  // アドレス・バス
const int g_analogGPIOs[4]     = {26, 27, 28, 29};  // データ・バス

//─────────────────
// クライアント別データ
//─────────────────
int g_ana0_Values[16*4], g_ana0_SwitchCnt = 4, g_ana0_PlayerCnt = 1;  // USB用
int g_ana1_Values[16*4], g_ana1_SwitchCnt = 4, g_ana1_PlayerCnt = 1;  // GPIO用


//━━━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━━━
static void InitAnalog(){
  for (int i = 0; i < 16 * 4; i++ ){ // 入力バッファをクリア
    g_ana0_Values[i] = 0;     // USB用
    g_ana1_Values[i] = 0;     // GPIO用
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

  // 入力バッファ、スイッチ数、プレイヤー数
  int* values() { return (*ctx.currentClient==0) ? g_ana0_Values    :  g_ana1_Values    ;}
  int& swCnt()  { return (*ctx.currentClient==0) ? g_ana0_SwitchCnt : g_ana1_SwitchCnt  ;}
  int& plCnt()  { return (*ctx.currentClient==0) ? g_ana0_PlayerCnt : g_ana1_PlayerCnt  ;}


  //━━━━━━━━━━━━━━━━━
  // コマンド・パーサー(実装)
  //━━━━━━━━━━━━━━━━━
  void handle(char dat[][10], int dat_cnt) override {

    LedScope  scopeLed(ctx, led);   // ＬＥＤ点滅をRAIIガードで実行
    Stream&   sp = MMP_SERIAL(ctx); // 対象ストリーム

    // ───────────────────────────────
    // ANS  : 使用範囲設定
    // 引数 : ① プレイヤー数
    // 　　　 ② スイッチ数
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [ANS!!]
    // ───────────────────────────────
    if (strcmp(dat[0],"ANS")==0){

      // １．前処理：
        // 1.1.書式チェック
        if ( dat_cnt != 3 )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.2.単項目チェック
        long newPl, newSw;
        if  ( !strHex2long(dat[1], newPl, 1, 16) ||
              !strHex2long(dat[2], newSw, 1, 4 ) )
        { ResCmdErr(sp, dat[0]); return; }

      plCnt() = (int)newPl;
      swCnt() = (int)newSw;

      for   ( int i = 0; i < 16; i++ ) {
        for ( int j = 0; j < 4 ; j++ ) {
          values()[i*4+j] = 0;
        }
      }

      // ４．後処理：
      sp.print("!!!!!");
    }

    // ───────────────────────────────
    // ANS  : 使用範囲設定
    // 引数 : なし
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [ANS!!]
    // ───────────────────────────────
    if (strcmp(dat[0],"ANU")==0){

      // １．前処理：
        // 1.1.書式チェック
        if ( dat_cnt != 1 )
        { ResCmdErr(sp, dat[0]); return; }

      // ２．前処理：
      for (int ch=0; ch<plCnt(); ch++){

        for (int i=0;i<4;i++){
          pinMode(g_addressBusGPIOs[i], OUTPUT);
          digitalWrite(g_addressBusGPIOs[i], (ch>>i)&1);
        }

        delayMicroseconds(10);

        for ( int sw = 0; sw < swCnt(); sw++ ){
          int pin = g_analogGPIOs[sw];
          values()[ch*4+sw] = analogRead(pin);
        }
      }

      // ４．後処理：
      sp.print("!!!!!");
    }

    // ───────────────────────────────
    // ANR  : 入力バッファ読取り
    // 引数 : ① プレイヤーID
    // 　　　 ② スイッチID
    // 戻り : ・正常 [XXXX!]
    //        ・異常 [ANR!!]
    // ───────────────────────────────
    if (strcmp(dat[0],"ANR")==0){

      // １．前処理：
        // 1.1.書式チェック
        if ( dat_cnt != 3 )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.2.単項目チェック
        long pl, sw;
        if (!strHex2long(dat[1], pl, 0, 15) ||
            !strHex2long(dat[2], sw, 0, 3))
        { ResCmdErr(sp, dat[0]); return; }

      char rets[6];
      snprintf(rets, sizeof(rets), "%04X!", values()[((int)pl)*4 + (int)sw] & 0xFFFF);

      // ４．後処理：
      sp.print(rets);
    }
  }
};
