// filename : modMP3.h
//========================================================
// モジュール：ＭＰ３
//--------------------------------------------------------
// Ver0.6.00 (2025/xx/xx)
// 未着手：DFPlayer→Serial MP3 Playerへの置換
//========================================================
#pragma once
#include "mod.h"
#include <DFRobotDFPlayerMini.h>  // デバイス固有

//━━━━━━━━━━━━━━━━━
// デバイス情報
//━━━━━━━━━━━━━━━━━
  DFRobotDFPlayerMini g_MP3[2];           // コンテナ
  bool g_MP3STATUS[2] = { false, false }; // 接続状況

//━━━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━━━
static void InitMP3(){

  Serial.println(String("---------------------------"));
  Serial.println(String("MP3プレイヤーを初期化中..."));

  g_MP3STATUS[0]   = false;
  g_MP3STATUS[1]   = false;

  Serial2.begin(9600, SERIAL_8N1, 11, 12);
  delay(50);

  if (g_MP3[0].begin(Serial2)){ // 接続済み設定
    g_MP3[0].volume(20);        // プロパティ：音量の規定値
    g_MP3STATUS[0] = true;      // Global変数：状況を接続済
  }

  Serial.println(String(" デバイスID : 1"));
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
    Stream&     sp = MMP_REPLY(ctx);      // 返信出力先（経路抽象）
    LedScope    scopeLed(ctx, led);       // コマンド色のLED発光
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正

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
        if (dat_cnt < 4) {_ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

        // 1.2. 単項目チェック
        int folder, track;
        if (!_Str2Int(dat[2], folder, 0, 255) ||
            !_Str2Int(dat[3], track,  0, 255) ){_ResChkErr(sp); return;}

      // ２．コマンド実行
      g_MP3[idx].playFolder(folder,track);

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
        if (dat_cnt < 3){_ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

        // 1.3. 単項目チェック
        int loop; if (!_Str2Int(dat[2], loop, 0, 1)){_ResChkErr(sp); return;}

      // ２．コマンド実行
      if (loop==1)  g_MP3[idx].enableLoop();
      else          g_MP3[idx].disableLoop();

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
        if (dat_cnt<2){_ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

      // ２．コマンド実行
      g_MP3[idx].stop();
            
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
        if (dat_cnt < 2){_ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

      // ２．コマンド実行
      g_MP3[idx].pause();
      
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
        if (dat_cnt<2){_ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

      // ２．コマンド実行
      g_MP3[idx].start();

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
        if (dat_cnt<3){_ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

        // 1.3. 単項目チェック
        int v; if (!_Str2Int(dat[2], v, 0, 30)){_ResChkErr(sp); return;}

      // ２．コマンド実行
      g_MP3[idx].volume(v);

      // ３．後処理：
      _ResOK(sp);
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
        if (dat_cnt<3){_ResChkErr(sp); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(sp, dat[1], idx)) return;

        // 1.3. 単項目チェック
        int mode; if (!_Str2Int(dat[2], mode, 0, 5)){_ResChkErr(sp); return;}

      // ２．コマンド実行
      g_MP3[idx].EQ(mode);

      // ３．後処理：
      _ResOK(sp);
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
        if (dat_cnt != 2){_ResChkErr(sp); return;}

        // 1.2. 単項目チェック
        int dev;
        if (!_Str2Int(dat[1], dev, 1, 2)){_ResChkErr(sp); return;}

      // ２．コマンド実行
      int res = g_MP3STATUS[dev - 1];

      // ３．後処理：
      _ResValue(sp, res);
      return;
    }

    // ───────────────────────────────
    // 機能：各種状態取得
    // 書式：MP3/INFO/*:<機器番号0～1>!
    // 戻値：
    // ───────────────────────────────
    if (
      strcmp(Cmd,"INFO/TRACK" ) == 0 ||
      strcmp(Cmd,"INFO/VOLUME") == 0 ||
      strcmp(Cmd,"INFO/EQ"    ) == 0 ||
      strcmp(Cmd,"INFO/FILEID") == 0 ||
      strcmp(Cmd,"INFO/FILES" ) == 0 )
    {
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt != 2){_ResChkErr(sp); return;}

        // 1.3. 対象外チェック
        int idx = 0;
        if (!checkDev(sp, dat[1], idx)) return;

        // ２．コマンド実行 ※エラーならリトライ
        int res = -1;
        for (int tries = 0; tries < 50 && res == -1; ++tries) {
          if      (strcmp(Cmd,"INFO/TRACK" ) == 0){res = g_MP3[idx].readState();res = g_MP3[idx].readState();} 
          else if (strcmp(Cmd,"INFO/VOLUME") == 0){res = g_MP3[idx].readVolume();res = g_MP3[idx].readVolume();}
          else if (strcmp(Cmd,"INFO/EQ"    ) == 0){res = g_MP3[idx].readEQ();res = g_MP3[idx].readEQ();}
          else if (strcmp(Cmd,"INFO/FILES" ) == 0){res = g_MP3[idx].readCurrentFileNumber();res = g_MP3[idx].readCurrentFileNumber();}
          else if (strcmp(Cmd,"INFO/FILEID") == 0){res = g_MP3[idx].readFileCounts();res = g_MP3[idx].readFileCounts();}
          if (res != -1) break;
        }

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
  // ───────────────
  // デバイス違反チェッカー
  // ───────────────
  bool checkDev(Stream& sp, const char* s, int idx){
    // 単項目チェック
    int dev;
    if (!_Str2Int(s, dev, 1, 2)){_ResChkErr(sp); return false;}

    // 対象外チェック
    idx = dev - 1;
    if (!g_MP3STATUS[idx]){_ResDevErr(sp); return false;}

    // 後処理
    return true;
  }
  // ───────────────
  // 対象トラック状況
  // ───────────────
  void reTrackkState(Stream& sp, int idx){
    // ※エラーならリトライ
    int res = -1;
    for (int tries = 0; tries < 50 && res == -1; ++tries) {
      res = g_MP3[idx].readState();
      res = g_MP3[idx].readState();
      if (res != -1) break;          // 成功したら即終了
    }
    _ResValue(sp, res);
  }
};
