// filename : mMP3.h
//========================================================
// モジュール：ＭＰ３
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.01 (2025/10/08)
//  - 初期化処理をスケッチ側から移管
//========================================================
#pragma once
#include "module.h"
#include <DFRobotDFPlayerMini.h>  // デバイス固有のライブラリ

//─────────────────
// コンテナ
//─────────────────
DFRobotDFPlayerMini g_mp3[2];

//─────────────────
// モジュール別の接続状況
//─────────────────
bool g_mp3Connected[2] = { false, false };


//━━━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━━━
static void InitMP3(){

  Serial2.begin(9600); delay(50);   // UART通信を開始

  if (g_mp3[0].begin(Serial2)) {     // 接続済み設定
    g_mp3[0].volume(20);             // プロパティ：音量の規定値
    g_mp3Connected[0] = true;        // Global変数：状況を接続済
  }

  g_mp3Connected[1]   = false;       // Global変数：未使用分は未接続
}

//━━━━━━━━━━━━━━━━━
// メイン処理
//━━━━━━━━━━━━━━━━━
class ModuleDF : public ModuleBase {
public:
  using ModuleBase::ModuleBase;

  //━━━━━━━━━━━━━━━━━
  // コマンド在籍確認(実装)
  //━━━━━━━━━━━━━━━━━
  bool owns(const char* cmd) const override {
    return strcmp(cmd,"DIR")==0 || strcmp(cmd,"DLP")==0 || strcmp(cmd,"DSP")==0 ||
           strcmp(cmd,"DPA")==0 || strcmp(cmd,"DPR")==0 || strcmp(cmd,"VOL")==0 ||
           strcmp(cmd,"DEF")==0 || strcmp(cmd,"DST")==0 || strcmp(cmd,"DPX")==0;
  }

  bool checkDev(const char* cmd, const char* s, int& idx, Stream& sp){
    long dev;
    if (!strHex2long(s, dev, 1, 2)) { ResCmdErr(sp, cmd); return false; }
    idx = (int)dev - 1;
    if (idx<0 || idx>=2 || !g_mp3Connected[idx]){ ResCmdErr(sp, cmd); return false; }
    return true;
  }

  //━━━━━━━━━━━━━━━━━
  // コマンド・パーサー(実装)
  //━━━━━━━━━━━━━━━━━
  void handle(char dat[][10], int dat_cnt) override {

    // スコープ
    LedScope  scopeLed(ctx, led);

    // カレント・クライアント
    Stream&   sp = MMP_SERIAL(ctx);

    //----------------------------------------------------------
    // 機能　　　　：ファームウェアのバージョン
    // コマンド形式：DPX:<機器番号1〜4>!
    // 返却値　　　："xyzz!" x:メジャー／y:マイナー／zz:リビジョン
    //----------------------------------------------------------
    if (strcmp(dat[0],"DPX")==0){
      if (dat_cnt<2){ ResCmdErr(sp,"DPX"); return; }
      long dev;
      if (!strHex2long(dat[1], dev, 1, 2)) { ResCmdErr(sp,"DPX"); return; }
      int idx = (int)dev - 1;
      if (idx<0 || idx>=2) { ResCmdErr(sp,"DPX"); return; }
      if (!g_mp3Connected[idx]){ ResCmdErr(sp,"DPX"); return; } // per spec: invalid/unconnected -> error
      ResHex4(sp, 1);
      return;
    }

    //----------------------------------------------------------
    // 機能　　　　：指定フォルダ内トラック再生
    // コマンド形式：DIR:<機器番号0～1>!
    // 返却値　　　："!!!!!" 正常 ／ "DIR!!" エラー
    //----------------------------------------------------------
    if (strcmp(dat[0],"DIR")==0){
      if (dat_cnt<4){ ResCmdErr(sp,"DIR"); return; }
      int idx; if (!checkDev("DIR", dat[1], idx, sp)) return;
      long folder, track;
      if (!strHex2long(dat[2], folder, 0, 0xFFFF) ||
          !strHex2long(dat[3], track,  0, 0xFFFF)){ ResCmdErr(sp,"DIR"); return; }
      g_mp3[idx].playFolder((int)folder,(int)track);
      sp.print("!!!!!");
      return;
    }
    
    //----------------------------------------------------------
    // 機能　　　　：リピート設定
    // コマンド形式：DLP:<機器番号0～1>:<設定 1(ON),0(OFF)>!
    // 返却値　　　："!!!!!" 正常 ／ "DLP!!" エラー
    //----------------------------------------------------------
    if (strcmp(dat[0],"DLP")==0){
      if (dat_cnt<3){ ResCmdErr(sp,"DLP"); return; }
      int idx; if (!checkDev("DLP", dat[1], idx, sp)) return;
      long loop;
      if (!strHex2long(dat[2], loop, 0, 1)){ ResCmdErr(sp,"DLP"); return; }
      if (loop==1) g_mp3[idx].enableLoop(); else g_mp3[idx].disableLoop();
      sp.print("!!!!!");
      return;
    }
    
    //----------------------------------------------------------
    // 機能　　　　：停止
    // コマンド形式：DSP:<機器番号0～1>!
    // 返却値　　　："!!!!!" 正常 ／ "DSP!!" エラー
    //----------------------------------------------------------
    if (strcmp(dat[0],"DSP")==0){
      if (dat_cnt<2){ ResCmdErr(sp,"DSP"); return; }
      int idx; if (!checkDev("DSP", dat[1], idx, sp)) return;
      g_mp3[idx].stop(); sp.print("!!!!!");
      return;
    }
    
    //----------------------------------------------------------
    // 機能　　　　：一時停止
    // コマンド形式：DPA:<機器番号0～1>!
    // 返却値　　　："!!!!!" 正常 ／ "DPA!!" エラー
    //----------------------------------------------------------
    if (strcmp(dat[0],"DPA")==0){
      if (dat_cnt<2){ ResCmdErr(sp,"DPA"); return; }
      int idx; if (!checkDev("DPA", dat[1], idx, sp)) return;
      g_mp3[idx].pause(); sp.print("!!!!!");
      return;
    }
    
    //----------------------------------------------------------
    // 機能　　　　：再生再開
    // コマンド形式：DPR:<機器番号0～1>!
    // 返却値　　　："!!!!!" 正常 ／ "DPR!!" エラー
    //----------------------------------------------------------
    if (strcmp(dat[0],"DPR")==0){
      if (dat_cnt<2){ ResCmdErr(sp,"DPR"); return; }
      int idx; if (!checkDev("DPR", dat[1], idx, sp)) return;
      g_mp3[idx].start(); sp.print("!!!!!");
      return;
    }
    
    //----------------------------------------------------------
    // 機能　　　　：音量設定（VOL）
    // コマンド形式：VOL:<機器番号0～1>:<音量0～30>!
    // 返却値　　　："!!!!!" 正常 ／ "VOL!!" エラー
    //----------------------------------------------------------
    if (strcmp(dat[0],"VOL")==0){
      if (dat_cnt<3){ ResCmdErr(sp,"VOL"); return; }
      int idx; if (!checkDev("VOL", dat[1], idx, sp)) return;
      long v; if (!strHex2long(dat[2], v, 0, 30)){ ResCmdErr(sp,"VOL"); return; }
      g_mp3[idx].volume((int)v); sp.print("!!!!!");
      return;
    }
    
    //----------------------------------------------------------
    // 機能　　　　：イコライザー設定（DEF）
    // コマンド形式：DEF:<機器番号0～1>:<タイプ0～5>!
    //              0: Normal, 1: Pop, 2: Rock,
    //              3: Jazz, 4: Classic, 5: Bass
    // 返却値　　　："!!!!!" 正常 ／ "DEF!!" エラー
    //----------------------------------------------------------
    if (strcmp(dat[0],"DEF")==0){
      if (dat_cnt<3){ ResCmdErr(sp,"DEF"); return; }
      int idx; if (!checkDev("DEF", dat[1], idx, sp)) return;
      long mode; if (!strHex2long(dat[2], mode, 0, 5)){ ResCmdErr(sp,"DEF"); return; }
      g_mp3[idx].EQ((int)mode); sp.print("!!!!!");
      return;
    }
    
    //----------------------------------------------------------
    // 機能　　　　：状態取得
    // コマンド形式：DST:<機器番号0～1>!
    // 返却値　　　："<状態番号>!" 正常 ／ "DST!!" エラー
    //              状態番号(0停止, 1再生, 2一時停止)
    //----------------------------------------------------------
    if (strcmp(dat[0],"DST")==0){
      if (dat_cnt<3){ ResCmdErr(sp,"DST"); return; }
      int idx; if (!checkDev("DST", dat[1], idx, sp)) return;
      long sel; if (!strHex2long(dat[2], sel, 1, 5)){ ResCmdErr(sp,"DST"); return; }
      int state = -1;
      switch((int)sel){
        case 1:
          state = g_mp3[idx].readState();
          state = g_mp3[idx].readState();
          break;
        case 2:
          state = g_mp3[idx].readVolume();
          state = g_mp3[idx].readVolume();
          break;
        case 3:
          state = g_mp3[idx].readEQ();
          state = g_mp3[idx].readEQ();
          break;
        case 4:
          state = g_mp3[idx].readFileCounts();
          state = g_mp3[idx].readFileCounts();
          break;
        case 5:
          state = g_mp3[idx].readCurrentFileNumber();
          state = g_mp3[idx].readCurrentFileNumber();
          break;
      }
      if (state < 0) { ResCmdErr(sp,"DST"); return; }
      ResHex4(sp, (int16_t)state);
      return;
    }    
  }
};
