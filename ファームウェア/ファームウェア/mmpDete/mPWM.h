// filename : mPWM.h
//========================================================
// モジュール：ＰＷＭ出力
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.01 (2025/10/08)
//  - 初期化処理をスケッチ側から移管
//  - 角度指定用のコマンドを新設
//  - サーボモータ用のコマンドを新設
//========================================================
#pragma once
#include "module.h"
#include <Wire.h>       // デバイスはi2c制御
#include <PCA9685.h>    // デバイス固有のライブラリ

constexpr uint8_t PWM_COUNT = 8;
PCA9685 g_pwm[PWM_COUNT];
int     g_maxDeviceID   = 0;
int     g_maxLogicalCh  = 0; // g_maxDevice * 16

//─────────────────
// プリセット型(素材)
//─────────────────
typedef struct {
  int16_t start;              // 値：開始
  int16_t end;                // 　：終了
  int16_t middle;             // 　：中間
} typePresetValue;
//─────────────────
typedef struct {
  int16_t front;              // 幅：前半
  int16_t rear;               // 　：後半
  int16_t middle;             // 　：中間
} typePresetWidth;
//─────────────────
typedef struct {
  typePresetValue value;      // 絶対値
  typePresetWidth width;      // 範囲値
} typePresetPwm_Base;

//─────────────────
// プリセット型(角度用)
//─────────────────
typedef struct {
  uint8_t             enable; // 有効無効(0=unset, 1=set)
  typePresetPwm_Base  pwm;    // ＰＷＭ値用
  typePresetWidth     deg;    // 角度用
} typePresetAngle;


//─────────────────
// プリセット型(ローテーション用)
//─────────────────
typedef struct {
  uint8_t             enable; // 0=unset, 1=set
  typePresetPwm_Base  pwm;    // ＰＷＭ値用
} typePresetPwm;
//─────────────────
typePresetAngle g_angleTbl[256];
typePresetPwm   g_TBL_ROT[256];


//━━━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━━━
static void InitRotation(int argFrom, int argTo){
  for (int idx = argFrom; idx <= argTo; idx++){
    typePresetPwm   &T = g_TBL_ROT[idx];
    T.enable            = 0;
    T.pwm.value.start   = 0;
    T.pwm.value.end     = 0;
    T.pwm.value.middle  = 0;
    T.pwm.width.front   = 0;
    T.pwm.width.rear    = 0;
    T.pwm.width.middle  = 0;
  }
}
//------------------------
static void InitAngle(int argFrom, int argTo){
  for (int idx = argFrom; idx <= argTo; idx++){
    typePresetAngle &T = g_angleTbl[idx];
    T.enable            = 0;
    T.pwm.value.start   = 0;
    T.pwm.value.end     = 0;
    T.pwm.value.middle  = 0;
    T.pwm.width.front   = 0;
    T.pwm.width.rear    = 0;
    T.pwm.width.middle  = 0;
  }
}
//------------------------
static void InitPWM(){

  int count = 0;

  Wire.begin(); delay(50);  // ｉ２ｃ通信を開始

  for ( int i = 0; i < PWM_COUNT; i++ ){

    uint8_t addr = 0x40 + i;
    Wire.beginTransmission(addr);

    if (Wire.endTransmission()==0){
      g_pwm[count] = PCA9685(addr);
      g_pwm[count].begin();
      g_pwm[count].setPWMFreq(60);
      count++;
    }
  }

  g_maxDeviceID  = count - 1;                     // 最大デバイス番号
  g_maxLogicalCh = (g_maxDeviceID + 1) * 16 - 1;  // 最大チャンネル番号

  InitRotation( 0, g_maxLogicalCh );    // プリセット初期化
  InitAngle   ( 0, g_maxLogicalCh );    // プリセット初期化
}


//━━━━━━━━━━━━━━━━━
// メイン処理
//━━━━━━━━━━━━━━━━━
class ModulePwm : public ModuleBase {
public:
  using ModuleBase::ModuleBase;

  //━━━━━━━━━━━━━━━━━
  // コマンド在籍確認(実装)
  //━━━━━━━━━━━━━━━━━
  bool owns(const char* cmd) const override {
    return strcmp(cmd,"PWX")==0 || strcmp(cmd,"PWM")==0 ||
           strcmp(cmd,"PAI")==0 || strcmp(cmd,"PAD")==0 || strcmp(cmd,"PAO")==0 || strcmp(cmd,"PAN")==0 || 
           strcmp(cmd,"PRI")==0 || strcmp(cmd,"PRD")==0 || strcmp(cmd,"PRO")==0 ;
  }

  //━━━━━━━━━━━━━━━━━
  // コマンド・パーサー(実装)
  //━━━━━━━━━━━━━━━━━
  void handle(char dat[][10], int dat_cnt) override {

    // コンテクストの依存性注入
    Stream&   sp = MMP_SERIAL(ctx); // クライアントのストリーム

    // スコープ
    LedScope  scopeLed(ctx, led);   // コマンド色のLED発光

    // ───────────────────────────────
    // PWX  : モジュールの接続確認
    // 引数 : なし
    // 戻り : ・接続数 [XXXXX!]
    // ───────────────────────────────
    if ( strcmp(dat[0], "PWX") == 0 ) {

      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt!=2)
        { ResCmdErr(sp, dat[0]); return; }
  
      // ２．主要データ取得：
        // ※ 該当処理なし

      // ３．接続状態を1桁HEXで返す(Boolean相当)：
      ResHex4(sp, (int16_t)g_maxDeviceID);

      // ４．後処理：
      return;
    }

    // ───────────────────────────────
    // PWM  : 生ＰＷＭ値を出力する
    // 引数 : ① チャンネルID(開始)／0x00～0x7F
    // 　　　 ② 出力値／0x00～0xFFF
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [PWM!!]
    // ───────────────────────────────
    if ( strcmp(dat[0],"PWM") == 0 ){

      // １．前処理：
        // 1.1.書式チェック
        if ( dat_cnt < 3 )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.2.単項目チェック
        long ch, val;
        if (!strHex2long(dat[1], ch,  0, g_maxLogicalCh) ||
            !strHex2long(dat[2], val, 0, 4095          ) )
        { ResCmdErr(sp, dat[0]); return; }

      // ２．ＰＷＭ出力：
      g_pwm[ (ch / 16) ].setPWM( (ch % 16), 0, (int16_t)val);

      // ３．後処理：
      sp.print("!!!!!");
      return;
    }

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 角度指定コマンド
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // PAI  : チャンネル別プリセット登録
    // 引数 : ① チャンネルID(開始)／0x00～0x7F
    //        ② チャンネルID(終了)／0x00～0x7F、★0xFF(単一指定)
    //        ③ 角度(最大)        ／0x00～0xFFF
    //        ④ ＰＷＭ値(最小)    ／0x00～0xFFF
    //        ⑤ ＰＷＭ値(最大)    ／0x00～0xFFF
    //        ⑥ ＰＷＭ値(中間)    ／0x00～0x0FFF、★0xFFFF(自動設定)
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [PAI!!]
    // 制限 : ・引数①②の大小関係
    //        ・引数③④の大小関係
    //        ・引数⑤⑥の大小関係
    //        ・引数⑤⑥⑦の大小関係
    // ───────────────────────────────
    if ( strcmp(dat[0],"PAI") == 0 ){

      // １．前処理：
        // 1.1.書式チェック
        if ( dat_cnt != 7 )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.2.単項目チェック
        long from,to,rs,re,ps,pe,pn;
        if  ( !strHex2long(dat[1], from, 0, g_maxLogicalCh                   ) ||
            ( !strHex2long(dat[2], to,   0, g_maxLogicalCh ) && to != 0xFF   ) ||
              !strHex2long(dat[3], re,   0, 360                              ) ||
              !strHex2long(dat[4], ps,   0, 4095                             ) ||
              !strHex2long(dat[5], pe,   0, 4095                             ) ||
            ( !strHex2long(dat[6], pn,   0, 4095           ) && pn != 0xFFFF ) )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.3.データ補正
        if (to==0xFF) to=from;

        // 1.4.相関チェック
        if  ( ( (long)from >= (long)to )  ||  // ①②チャンネルID(開始終了)
            (rs >= re)                    ||  // ③④角度(開始終了)
            (ps >= pe)                    ||  // ⑤⑥ＰＷＭ値(開始終了)
            (pn <= ps) || (pe <= pn)      )   // ⑤⑥⑦ＰＷＭ値(中間)
        { ResCmdErr(sp, dat[0]); return; }

      // ２．主要データ取得：
      if ( pn == 0xFFFF ) { pn = (int16_t)((ps + pe + 1) / 2); }
      int16_t pwmFront  = (int16_t)(pn - ps);                   // ＰＷＭ：幅：前半
      int16_t pwmRear   = (int16_t)(pe - pn);                   // 　　　　　：後半
      int16_t redMiddle = (int16_t)((pe + 1) / 2);              // 角　度：幅：中間
      int16_t radFront  = redMiddle;                            // 　　　　　：前半
      int16_t redRear   = re - radFront;                        // 　　　　　：後半

      // ３．プリセット登録：
      for ( int ch = (int)from; ch <= (int)to; ++ch ){
        typePresetAngle &tbl = g_angleTbl[ch];
        tbl.enable    = 1           ; // 有効性
        tbl.pwm.value.start   = (int16_t)ps ; // ＰＷＭ；値；開始
        tbl.pwm.value.end     = (int16_t)pe ; // 　　　　　；終了
        tbl.pwm.value.middle  = (int16_t)pn ; // 　　　　　；中間
        tbl.pwm.width.front   = pwmFront    ; // 　　　；幅；前半
        tbl.pwm.width.rear    = pwmRear     ; // 　　　　　；後半
        tbl.pwm.width.middle  = 0           ; // ※未使用
        tbl.deg.front         = radFront    ; // 角　度；幅；前半
        tbl.deg.rear          = redRear     ; // 　　　　　；後半
        tbl.deg.middle        = redMiddle   ; // 　　　　　；中間
      }

      // ４．後処理：
      sp.print("!!!!!");
      return;
    }

    // ───────────────────────────────
    // PAD  : チャンネル別プリセット削除
    // 引数 : ① チャンネルID(開始)／0x00～0x7F
    //        ② チャンネルID(終了)／0x00～0x7F、★0xFF(単一指定)
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [PAD!!]
    // 制限 : ・引数①②の大小関係
    // ───────────────────────────────
    if ( strcmp(dat[0],"PAD") == 0 ){

      // １．前処理：
        // 1.1.書式チェック
        if ( dat_cnt != 3 )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.2.単項目チェック
        long from,to;
        if  ( !strHex2long(dat[1], from, 0, g_maxLogicalCh)                 ||
            ( !strHex2long(dat[2], to,   0, g_maxLogicalCh) && to != 0xFF)  )
        { ResCmdErr(sp, dat[0]); return; }

        // ★データ補正
        if ( to == 0xFF ) to=from;

        // ★相関チェック
        if ( (long)from >= (long)to )
        { ResCmdErr(sp, dat[0]); return; }
      
      // ２．主要データ取得：
        // ※ 該当処理なし

      // ３．プリセット削除：
      InitAngle( (int)from, (int)to );

      // ４．後処理：
      sp.print("!!!!!");
      return;
    }

    // ───────────────────────────────
    // PAO  : ＰＷＭ出力（角度指定）
    // 引数 : ① チャンネルID／0x00～0x7F
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [PAO!!]
    // 制限 : とくになし
    // ───────────────────────────────
    if ( strcmp(dat[0],"PAO") == 0 ){

      // １．前処理：
        // 1.1.書式チェック
        if ( dat_cnt != 3 )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.2.単項目チェック(チャンネル番号)
        long ch;
        if ( !strHex2long(dat[1], ch,  0, g_maxLogicalCh) )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.3.プリセットを特定
        typePresetAngle &tbl = g_angleTbl[(int)ch];

        // 1.4.機能チェック
        if ( !tbl.enable )  
        { ResCmdErr(sp, dat[0]); return; }

        // 1.5.単項目チェック(角度)
        long deg;
        int degEnd = tbl.deg.front + tbl.deg.rear;
        if ( !strHex2long(dat[2], deg, 0, degEnd) )
        { ResCmdErr(sp, dat[0]); return; }

      // ２．主要データ取得：
        // 2.1.準備
        float   rate                          ;  // 100分率
        int16_t pwmVal = tbl.pwm.value.middle ; // 出力ＰＷＭ値　※初期値：ユーザ定義済の中間値

        // 2.2.中間前後の算出
          // 2.2.1.指定の角度が中間以下の場合
          if (deg < tbl.deg.middle){
            rate   = deg / tbl.deg.front;                     // 比率を求める
            pwmVal = (int16_t)((tbl.pwm.width.front) * rate); // 中間の幅(比率)を求める
            pwmVal = pwmVal + tbl.pwm.value.start;            // ＰＷＭ値(開始)を加える
          }

          // 2.2.2.指定の角度が中間超えの場合
          if(deg > tbl.deg.middle){
            rate   = (deg - tbl.deg.front) / tbl.deg.rear;    // 比率を求める
            pwmVal = (int16_t)((tbl.pwm.width.rear) * rate);  // 後半の幅(比率)を求める
            pwmVal = pwmVal + tbl.pwm.value.middle;           // ＰＷＭ値(中間)を加える
          }

      // ３．ＰＷＭ出力：
      g_pwm[ (ch / 16) ].setPWM( (ch % 16), 0, pwmVal);

      // ４．後処理：
        sp.print("!!!!!");
        return;
    }

    // ───────────────────────────────
    // PAN  : ニュートラル（中央位置）
    // 引数 : ① チャンネルID／0x00～0x7F
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [PAO!!]
    // 制限 : とくになし
    // ───────────────────────────────
    if (strcmp(dat[0],"PAN")==0){

      // １．前処理：
        // 1.1.書式チェック
        if ( dat_cnt != 2 )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.2.単項目チェック(チャンネル番号)
        long ch;
        if ( !strHex2long(dat[1], ch, 0, g_maxLogicalCh) )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.3.プリセットを特定
        typePresetAngle &tbl = g_angleTbl[(int)ch];

        // 1.4.機能チェック
        if ( !tbl.enable )
        { ResCmdErr(sp, dat[0]); return; }

      // ２．ＰＷＭ出力：
      g_pwm[ (ch / 16) ].setPWM( (ch % 16), 0, tbl.pwm.value.middle);

      // ３．後処理：
        sp.print("!!!!!");
        return;
    }


    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ローテーションサーボ用コマンド
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // PRI  : チャンネル別プリセット登録
    // 引数 : ① チャンネルID(開始)  ／0x00～0x7F
    //        ② チャンネルID(終了)  ／0x00～0x7F、★0xFF(単一指定)
    //        ③ ＰＷＭ値(左周り最大)／0x00～0xFFF
    //        ④ ＰＷＭ値(右回り最大)／0x00～0xFFF
    //        ⑤ ＰＷＭ値(中間)      ／0x00～0x0FFF、★0xFFFF(自動設定)
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [PRI!!]
    // 制限 : ・引数①②の大小関係
    //        ・引数③④の大小関係
    //        ・引数③④⑤の大小関係
    // ───────────────────────────────
    if ( strcmp(dat[0],"PRI") == 0 ){

      // １．前処理：
        // 1.1.書式チェック
        if ( dat_cnt != 6 )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.2.単項目チェック
        long from,to,ps,pe,pn;
        if  ( !strHex2long(dat[1], from, 0, g_maxLogicalCh                   ) ||
            ( !strHex2long(dat[2], to,   0, g_maxLogicalCh ) && to != 0xFF   ) ||
              !strHex2long(dat[3], ps,   0, 4095                             ) ||
              !strHex2long(dat[4], pe,   0, 4095                             ) ||
            ( !strHex2long(dat[5], pn,   0, 4095           ) && pn != 0xFFFF ) )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.3.データ補正
        if ( to == 0xFF ) to=from;
        if ( (int)pn == 0xFFFF ) {
          long mid = ((long)ps + (long)pe + 1) / 2;
          pn = mid;
        }

        // 1.4.相関チェック
        if ( ( (long)from >= (long)to ) ||  // ①②チャンネルID(開始終了)
          (ps >= pe)                    ||  // ③④ＰＷＭ値(左右周り最大値)
          (pn <= ps) || (pe <= pn)      )   // ③④⑤ＰＷＭ値(中央値)
        { ResCmdErr(sp, dat[0]); return; }

      // ２．主要データ取得：
      if (pn == 0xFFFF) { pn = (int16_t)((ps + pe + 1) / 2); }  // ＰＷＭ：値：中間
      int16_t pwmFront = (int16_t)(pn - ps);                    // 　　　　　：前半
      int16_t pwmRear  = (int16_t)(pe - pn);                    // 　　　　　：後半

      // ３．プリセット登録：
      for (int ch = (int)from; ch <= (int)to; ++ch){
        typePresetPwm &tbl = g_TBL_ROT[ch];
        tbl.enable            = 1           ;
        tbl.pwm.value.start   = (int16_t)ps ;
        tbl.pwm.value.end     = (int16_t)pe ;
        tbl.pwm.value.middle  = (int16_t)pn ;
        tbl.pwm.width.front   = pwmFront    ;
        tbl.pwm.width.rear    = pwmRear     ;
        tbl.pwm.width.middle  = 0           ; // ※未使用
      }

      // ４．後処理：
      sp.print("!!!!!");
      return;
    }

    // ───────────────────────────────
    // PRD  : チャンネル別プリセット削除
    // 引数 : ① チャンネルID(開始)／0x00～0x7F
    //        ② チャンネルID(終了)／0x00～0x7F、★0xFF(単一指定)
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [PRD!!]
    // 制限 : ・引数①②の大小関係
    // ───────────────────────────────
    if ( strcmp(dat[0],"PRD") == 0 ){

      // １．前処理：
        // 1.1.書式チェック
        if ( dat_cnt != 3 )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.2.単項目チェック
        long from,to;
        if  ( !strHex2long(dat[1], from, 0, g_maxLogicalCh                 ) ||
            ( !strHex2long(dat[2], to,   0, g_maxLogicalCh) && to != 0xFF  ) )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.3.データ補正
        if (to==0xFF) to=from;

        // 1.4.相関チェック
        if ( (long)from >= (long)to )
        { ResCmdErr(sp, dat[0]); return; }
      
      // ２．主要データ取得：
        // ※ 該当処理なし

      // ３．プリセット削除：
      InitRotation( (int)from, (int)to );

      // ４．後処理：
        // 4.1.正常にリターン
        sp.print("!!!!!");
        return;
    }

    // ───────────────────────────────
    // PRO  : ＰＷＭ出力（回転率％指定）
    // 引数 : ① チャンネルID／0x00～0x7F
    // 戻り : ・正常 [!!!!!]
    //        ・異常 [PRO!!]
    // 制限 : とくになし
    // ───────────────────────────────
    if ( strcmp(dat[0],"PRO")==0 ){

      // １．前処理：
        // 1.1.書式チェック
        if ( dat_cnt != 3 )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.2.単項目チェック
        long ch,per;
        if  ( !strHex2long(dat[1], ch,  0, g_maxLogicalCh  ) ||
              !strHex2long(dat[2], per, 0, 0x6e            ) )
        { ResCmdErr(sp, dat[0]); return; }

        // 1.3.プリセットを特定
        typePresetPwm &tbl = g_TBL_ROT[(int)ch];

        // 1.4.機能チェック
        if ( !tbl.enable )
        { ResCmdErr(sp, dat[0]); return; }

      // ２．主要データ取得：
        // 2.1.準備
        float   rate      ; // 100分率
        int16_t pwmVal = 0; // 出力ＰＷＭ値 ※初期値：ゼロ値(必ず停止)

        // 2.2.中間前後の算出
          // 2.2.1.指定の比率が中間未満の場合
          if (per < 0){
            rate   = 1 + (per / 100);                         // 比率を求める
            pwmVal = (int16_t)((tbl.pwm.width.front) * rate); // 中間の幅(比率)を求める
            pwmVal = pwmVal + tbl.pwm.value.start;            // ＰＷＭ値(開始)を加える
          }

          // 2.2.2.指定の比率が中間超えの場合
          if(per > 0){
            rate   = abs(per / 100);                          // 比率を求める
            pwmVal = (int16_t)((tbl.pwm.width.rear) * rate);  // 後半の幅(比率)を求める
            pwmVal = pwmVal + tbl.pwm.value.middle;           // ＰＷＭ値(中間)を加える
          }

      // ３．ＰＷＭ出力：
      g_pwm[ (ch / 16) ].setPWM( (ch % 16), 0, pwmVal);

      // ４．後処理：
      sp.print("!!!!!");
      return;
    }
  }
};