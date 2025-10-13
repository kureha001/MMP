// filename : mMP3.h
//========================================================
// モジュール：ＭＰ３
//-------------------------------------------------------- 
// 変更履歴: Ver0.5.00 (2025/10/13)
// - WEB-API書式対応
//========================================================
#pragma once
#include "module.h"
#include <DFRobotDFPlayerMini.h>  // デバイス固有

//━━━━━━━━━━━━━━━━━
// デバイス情報
//━━━━━━━━━━━━━━━━━
  DFRobotDFPlayerMini g_mp3[2];           // コンテナ
  bool g_mp3status[2] = { false, false }; // 接続状況

//━━━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━━━
static void InitMP3(){

  Serial2.begin(9600); delay(50); // UART通信を開始

  if (g_mp3[0].begin(Serial2)) {  // 接続済み設定
    g_mp3[0].volume(20);          // プロパティ：音量の規定値
    g_mp3status[0] = true;        // Global変数：状況を接続済
  }

  g_mp3status[1]   = false;       // Global変数：未使用分は未接続
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
      return cmd && strncmp(cmd, "MP3", 3) == 0;
  }

  //━━━━━━━━━━━━━━━━━
  // コマンド・パーサー(実装)
  //━━━━━━━━━━━━━━━━━
  void handle(char dat[][ DAT_LENGTH ], int dat_cnt) override {

    //━━━━━━━━━━━━━━━━━
    // 前処理
    //━━━━━━━━━━━━━━━━━
    Stream&     sp = MMP_SERIAL(ctx);     // クライアントのストリーム
    LedScope    scopeLed(ctx, led);       // コマンド色のLED発光
    const char* Cmd = getCommand(dat[0]); // コマンド名を補正

  //━━━━━━━━━━━━━━━━━
  // 対象トラックの制御
  //━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 機能：指定フォルダ内トラックを再生開始
    // 書式：MP3/TRACK/PLAY:<機器番号0～1>!
    // 戻値："!!!!!" 正常 ／ "ERR!!" エラー
    // ───────────────────────────────
    if (strcmp(Cmd,"TRACK/PLAY") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt<4) {ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

        // 1.2. 単項目チェック
        long folder, track;
        if (!strHex2long(dat[2], folder, 0, 0xFFFF) ||
            !strHex2long(dat[3], track,  0, 0xFFFF) ){ResChkErr(sp); return;}

      // ２．コマンド実行
      g_mp3[idx].playFolder((int)folder,(int)track);

      // ３．後処理：
      reTrackkState(sp,idx);
      return;
    }
    
    // ───────────────────────────────
    // 機能：ループ再生を設定
    // 書式：MP3/LOOP:<機器番号0～1>:<設定 1(ON),0(OFF)>!
    // 戻値："!!!!!" 正常 ／ "ERR!!" エラー
    // ───────────────────────────────
    if (strcmp(Cmd,"TRACK/LOOP") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt < 3){ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

        // 1.3. 単項目チェック
        long loop; if (!strHex2long(dat[2], loop, 0, 1)){ResChkErr(sp); return;}

      // ２．コマンド実行
      if (loop==1)  g_mp3[idx].enableLoop();
      else          g_mp3[idx].disableLoop();

      // ３．後処理：
      reTrackkState(sp,idx);
      return;
    }
    
    // ───────────────────────────────
    // 機能：停止
    // 書式：MP3/STOP:<機器番号0～1>!
    // 戻値："!!!!!" 正常 ／ "ERR!!" エラー
    // ───────────────────────────────
    if (strcmp(Cmd,"TRACK/STOP") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt<2){ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

      // ２．コマンド実行
      g_mp3[idx].stop();
            
      // ３．後処理：
      reTrackkState(sp,idx);
      return;
    }
    
    // ───────────────────────────────
    // 機能：一時停止
    // 書式：MP3/PAUSE:<機器番号0～1>!
    // 戻値："!!!!!" 正常 ／ "ERR!!" エラー
    // ───────────────────────────────
    if (strcmp(Cmd,"TRACK/PAUSE") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt < 2){ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

      // ２．コマンド実行
      g_mp3[idx].pause();
      
      // ３．後処理：
      reTrackkState(sp,idx);
      return;
    }
    
    // ───────────────────────────────
    // 機能：開始
    // 書式：MP3/START:<機器番号0～1>!
    // 戻値："!!!!!" 正常 ／ "ERR!!" エラー
    // ───────────────────────────────
    if (strcmp(Cmd,"TRACK/START") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt<2){ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

      // ２．コマンド実行
      g_mp3[idx].start();

      // ３．後処理：
      reTrackkState(sp,idx);
      return;
    }
    
  //━━━━━━━━━━━━━━━━━
  // デバイスの設定
  //━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 機能：音量を設定
    // 書式：MP3/VOLUME:<機器番号0～1>:<音量0～30>!
    // 戻値："!!!!!" 正常 ／ "ERR!!" エラー
    // ───────────────────────────────
    if (strcmp(Cmd,"SET/VOLUME") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt<3){ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

        // 1.3. 単項目チェック
        long v; if (!strHex2long(dat[2], v, 0, 30)){ResChkErr(sp); return;}

      // ２．コマンド実行
      g_mp3[idx].volume((int)v);

      // ３．後処理：
      ResOK(sp);
      return;
    }
    
    // ───────────────────────────────
    // 機能：イコライザーを設定
    // 書式：MP3/EQ:<機器番号0～1>:<タイプ0～5>!
    //        0: Normal, 1: Pop, 2: Rock,3: Jazz, 4: Classic, 5: Bass
    // 戻値："!!!!!" 正常 ／ "ERR!!" エラー
    // ───────────────────────────────
    if (strcmp(Cmd,"SET/EQ") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt<3){ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

        // 1.3. 単項目チェック
        long mode; if (!strHex2long(dat[2], mode, 0, 5)){ResChkErr(sp); return;}

      // ２．コマンド実行
      g_mp3[idx].EQ((int)mode);

      // ３．後処理：
      ResOK(sp);
      return;
    }
    
  //━━━━━━━━━━━━━━━━━
  // インフォメーション
  //━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 機能：機器の状態
    // 書式：X:<機器番号1〜4>!
    // 戻値："xyzz!" x:メジャー／y:マイナー／zz:リビジョン
    // ───────────────────────────────
    if (strcmp(Cmd,"INFO/CONNECT") == 0){
      // １．前処理
        // 1.1. 書式
        if (dat_cnt != 2){ResChkErr(sp); return;}

        // 1.2. 単項目チェック
        long dev;
        if (!strHex2long(dat[1], dev, 1, 2)){ResChkErr(sp); return;}

        // 1.3. 相関チェック
        int idx = (int)dev - 1;
        if (idx < 0 || idx >= 2){ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        if (!g_mp3status[idx]){ResChkErr(sp); return;}

      // ２．コマンド実行
      int16_t res = g_mp3status[idx];

      // ３．後処理：
      ResHex4(sp, res);
      return;
    }

    // ───────────────────────────────
    // 機能：各種状態取得
    // 書式：MP3/INFO/*:<機器番号0～1>!
    // 戻値：
    // ───────────────────────────────
    if (
      strcmp(Cmd,"INFO/TRACK"  ) == 0 ||
      strcmp(Cmd,"INFO/VOLUME" ) == 0 ||
      strcmp(Cmd,"INFO/EQ"     ) == 0 ||
      strcmp(Cmd,"INFO/FILEID" ) == 0 ||
      strcmp(Cmd,"INFO/FILES"  ) == 0 )
    {
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt != 2){ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

        // 1.3. 単項目チェック
        long sel; if (!strHex2long(dat[2], sel, 1, 5)){ResChkErr(sp); return;}

      // ２．コマンド実行
      int res = -1;
        // 再生状況
        if (strcmp(Cmd,"INFO/TRACK") == 0){
          reTrackkState(sp,idx);
          return;
        }
        // 音量
        if (strcmp(Cmd,"INFO/VOLUME") == 0){
          res = g_mp3[idx].readVolume();
          res = g_mp3[idx].readVolume();
        }
        // イコライザーのモード
        if (strcmp(Cmd,"INFO/EQ") == 0){
          res = g_mp3[idx].readEQ();
          res = g_mp3[idx].readEQ();
        }
        // ファイル番号
        if (strcmp(Cmd,"INFO/FILEID") == 0){
          res = g_mp3[idx].readFileCounts();
          res = g_mp3[idx].readFileCounts();
        }
        // ファイル総数
        if (strcmp(Cmd,"INFO/FILES") == 0){
          res = g_mp3[idx].readCurrentFileNumber();
          res = g_mp3[idx].readCurrentFileNumber();
        }

        // ３．後処理：
        ResHex4(sp, (int16_t)res);
        return;
      }    

  //━━━━━━━━━━━━━━━━━
  // コマンド名エラー
  //━━━━━━━━━━━━━━━━━
  ResNotCmd(sp);
  return;
  }

//━━━━━━━━━━━━━━━━━
// 内部ヘルパー
//━━━━━━━━━━━━━━━━━
  // ───────────────
  // デバイス違反チェッカー
  // ───────────────
  bool checkDev(Stream& sp, const char* s, int idx){
    // 単項目チェック
    long dev;
    if (!strHex2long(s, dev, 1, 2)){ResChkErr(sp); return false;}

    // 対象外チェック
    idx = (int)dev - 1;
    if (!g_mp3status[idx]){ResDevErr(sp); return false;}

    // 後処理
    return true;
  }
  // ───────────────
  // 対象トラック状況
  // ───────────────
  void reTrackkState(Stream& sp, int idx){
      int res;
      res = g_mp3[idx].readState();
      res = g_mp3[idx].readState();
      ResHex4(sp, (int16_t)res);
  }
};
