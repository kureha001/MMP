// filename : mPWM.h
//========================================================
// モジュール：ＰＷＭ出力
//-------------------------------------------------------- 
// 変更履歴: Ver0.5.01 (2025/10/16)
// - 10進数統一
//========================================================
#pragma once
#include "module.h"
#include <Wire.h>       // デバイスはi2c制御
#include <PCA9685.h>    // デバイス固有

//━━━━━━━━━━━━━━━━━
// デバイス情報
//━━━━━━━━━━━━━━━━━
  // コンテナ
  constexpr uint8_t PWM_COUNT = 8;  // 走査デバイス数
  PCA9685 g_PWM[PWM_COUNT];         // コンテナ

  // 最大ID
  int g_MaxDevID = 0; // デバイスID
  int g_MaxLogCh = 0; // チャンネルID(デバイスID * 16)

//━━━━━━━━━━━━━━━━━
// プリセット情報
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 型：素材
  //─────────────────
  typedef struct {
    int start;              // 値：開始
    int end;                // 　：終了
    int middle;             // 　：中間
  } typePresetValue;
  //─────────────────
  typedef struct {
    int front;              // 幅：前半
    int rear;               // 　：後半
    int middle;             // 　：中間
  } typePresetWidth;
  //─────────────────
  typedef struct {
    typePresetValue value;      // 絶対値
    typePresetWidth width;      // 範囲値
  } typePresetPwm_Base;

  //─────────────────
  // 型：用途別
  //─────────────────
  typedef struct {
    uint8_t             enable; // 有効無効(0=unset, 1=set)
    typePresetPwm_Base  pwm;    // ＰＷＭ値用
    typePresetWidth     deg;    // 角度用
  } typePresetAngle;
  //─────────────────
  typedef struct {
    uint8_t             enable; // 0=unset, 1=set
    typePresetPwm_Base  pwm;    // ＰＷＭ値用
  } typePresetPwm;

  //─────────────────
  // プリセット情報
  //─────────────────
  typePresetAngle g_TBL_ANGLE[ PWM_COUNT * 16]; // 角度指定用
  typePresetPwm   g_TBL_ROTATE[PWM_COUNT * 16]; // 回転サーボ用

//━━━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━━━
static void InitRotate(int argFrom, int argTo){
  for (int idx = argFrom; idx <= argTo; idx++){
    typePresetPwm &T   = g_TBL_ROTATE[idx];
    T.enable           = 0;
    T.pwm.value.start  = 0;
    T.pwm.value.end    = 0;
    T.pwm.value.middle = 0;
    T.pwm.width.front  = 0;
    T.pwm.width.rear   = 0;
    T.pwm.width.middle = 0;
  }
}
//------------------------
static void InitAngle(int argFrom, int argTo){
  for (int idx = argFrom; idx <= argTo; idx++){
    typePresetAngle &T = g_TBL_ANGLE[idx];
    T.enable           = 0;
    T.pwm.value.start  = 0;
    T.pwm.value.end    = 0;
    T.pwm.value.middle = 0;
    T.pwm.width.front  = 0;
    T.pwm.width.rear   = 0;
    T.pwm.width.middle = 0;
  }
}
//------------------------
static void InitPWM(){

  Wire.begin(); delay(50);  // ｉ２ｃ通信を開始

  // I2Cアドレス0x40から接続走査
  int count = 0;
  for (int i = 0; i < PWM_COUNT; i++){
    
    // 接続開始
    uint8_t addr = 0x40 + i;
    Wire.beginTransmission(addr);

    // 接続されている場合
    if(Wire.endTransmission() == 0){
      g_PWM[count] = PCA9685(addr); // オブジェクトを登録
      g_PWM[count].begin();         // 通信開始
      g_PWM[count].setPWMFreq(60);  // 初期設定
      count++;                      // 次を走査
    }
  }

  // 最大IDを求める
  g_MaxDevID  = count - 1;       // デバイスID
  g_MaxLogCh = count * 16 - 1;  // チャンネルID

  // プリセットを初期化
  InitRotate(0, g_MaxLogCh);    // 回転サーボ用
  InitAngle (0, g_MaxLogCh);    // 角度指定用
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
      return cmd && strncmp(cmd, "PWM", 3) == 0;
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
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正

    // ───────────────────────────────
    // 機能 ：機器の状態
    // 書式 : PWX 
    // 機能 : モジュールの接続確認
    // ───────────────────────────────
    // 引数 : なし
    // ───────────────────────────────
    // 戻値 : ・接続数 [XXXXX!]
    // ───────────────────────────────
    if (strcmp(Cmd,"INFO/CONNECT") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 1){_ResChkErr(sp); return;}
  
      // ２．値取得：
      int res = g_MaxDevID;

      // ３．後処理：
      _ResValue(sp, res);
      return;
    }

    // ───────────────────────────────
    // 名称 : OUTPUT 
    // 機能 : ＰＷＭ値を出力する
    // ───────────────────────────────
    // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
    // 　　　 ② ＰＷＭ出力値　　　；0～4095
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [PWM!!]
    // ───────────────────────────────
    if (strcmp(Cmd,"OUTPUT") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt < 3){_ResChkErr(sp); return;}

        // 1.2.単項目チェック
        int ch, val;
        if(!comChannel(dat[1], ch,  false) ||
           !comPWM_val(dat[2], val, false) ){_ResChkErr(sp); return;}

      // ２．ＰＷＭ出力：
      g_PWM[int(ch/16)].setPWM((ch%16), 0, val);

      // ３．後処理：
      _ResOK(sp);
      return;
    }

  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // 角度指定コマンド
  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 名称 : PAI 
    // 機能 : チャンネル別プリセット登録
    // ───────────────────────────────
    // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
    //        ② チャンネルID(終了)；0～127,-1(単一チャンネル)
    //        ③ 角度(最大)    ；0～360度
    //        ④ ＰＷＭ値(最小)；0～4095
    //        ⑤ ＰＷＭ値(最大)；0～4095
    //        ⑥ ＰＷＭ値(中間)；0～4095,-1(自動)
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [PAI!!]
    // ───────────────────────────────
    // 制限 : ・引数①②の大小関係
    //        ・引数④⑤の大小関係
    //        ・引数④⑤⑥の大小関係
    // ───────────────────────────────
    if (strcmp(Cmd,"ANGLE/SETUP") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 7){_ResChkErr(sp); return;}

        // 1.2.単項目チェック
        int from, to, re, ps, pe, pn;
        if(!comChannel(dat[1], from, false ) ||
           !comChannel(dat[2], to,   true  ) ||
           !_Str2Int  (dat[3], re,   0, 360) ||
           !comPWM_val(dat[4], ps,   false ) ||
           !comPWM_val(dat[5], pe,   false ) ||
           !comPWM_val(dat[6], pn,   true  ) ){_ResChkErr(sp); return;}

        // 1.3.データ補正
        if(to == -1) to = from;                         // 単一チャンネル(=From)
        if(pn == -1) pn = int((ps + pe + 1) / 2); // 中間を自動計算

        // 1.4.相関チェック
        if(from >  to ||  // ①②チャンネルID
           ps   >= pe ||  // ④⑤ＰＷＭ値
           pn   <= ps ||  // ⑥④ＰＷＭ値(中間)
           pn   >= pe     // ⑥⑤ＰＷＭ値(中間)
        ){_ResChkErr(sp); return;}

      // ２．主要データ取得：
      int pwmFront  = (pn - ps)     ; // ＰＷＭ：幅：前半
      int pwmRear   = (pe - pn)     ; // 　　　　　：後半
      int degMiddle = ((re + 1) / 2); // 角　度：幅：中間
      int degFront  = degMiddle              ; // 　　　　　：前半
      int degRear   = re - degFront          ; // 　　　　　：後半

      // ３．プリセット登録：
      for (int ch = from; ch <= to; ++ch){
        typePresetAngle &tbl = g_TBL_ANGLE[ch];
        tbl.enable           = 1          ; // 有効性
        tbl.pwm.value.start  = ps; // ＰＷＭ；値；開始
        tbl.pwm.value.end    = pe; // 　　　　　；終了
        tbl.pwm.value.middle = pn; // 　　　　　；中間
        tbl.pwm.width.front  = pwmFront   ; // 　　　；幅；前半
        tbl.pwm.width.rear   = pwmRear    ; // 　　　　　；後半
        tbl.pwm.width.middle = 0          ; // ※未使用
        tbl.deg.front        = degFront   ; // 角　度；幅；前半
        tbl.deg.rear         = degRear    ; // 　　　　　；後半
        tbl.deg.middle       = degMiddle  ; // 　　　　　；中間
      }

      // ４．後処理：
      _ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 名称 : PAO 
    // 機能 : ＰＷＭ出力（角度指定）
    // ───────────────────────────────
    // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [PAO!!]
    // ───────────────────────────────
    // 制限 : とくになし
    // ───────────────────────────────
    if (strcmp(Cmd,"ANGLE/OUTPUT") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 3){_ResChkErr(sp); return;}

        // 1.2.単項目チェック(チャンネル番号)
        int ch;
        if(!comChannel(dat[1], ch, false)){_ResChkErr(sp); return;}

        // 1.3.機能チェック
        typePresetAngle &tbl = g_TBL_ANGLE[ch];
        if(!tbl.enable ){_ResIniErr(sp); return;}

        // 1.4.単項目チェック(角度)
        int arrow;
        int degEnd = tbl.deg.front + tbl.deg.rear;
        if(!_Str2Int(dat[2], arrow, 0, degEnd)){_ResChkErr(sp); return;}

      // ２．主要データ取得：
        int ground = tbl.deg.middle ; // 中間基準値
        float   rate                    ; // 比率
        if(arrow < ground) rate = (float)arrow / (float)tbl.deg.front;
        else               rate = (float)(arrow - tbl.deg.middle) / (float)tbl.deg.rear;

      // ２．ＰＷＭ出力：
      int res = comOutPWM(
        ch         , // チャンネルID
        arrow      , // 指定値
        ground              , // 中間基準値
        rate                , // 比率
        tbl.pwm.width.front , // PWM幅(前半)
        tbl.pwm.width.rear  , // PWM幅(後半)
        tbl.pwm.value.start , // PWM値(開始)
        tbl.pwm.value.middle
      );  // PWM値(中間)

      // ４．後処理：
      if(res < 0) _ResChkErr(sp);
      else        _ResValue(sp, res);
      return;
    }

  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // 回転サーボ用コマンド
  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 名称 : PRI 
    // 機能 : チャンネル別プリセット登録
    // ───────────────────────────────
    // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
    //        ② チャンネルID(終了)；0～127,-1(単一チャンネル)
    //        ③ ＰＷＭ値(逆転最大)；0～4095
    //        ④ ＰＷＭ値(正転最大)；0～4095
    //        ⑤ ＰＷＭ値(中間)　　；0～4095,-1(自動)
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [PRI!!]
    // ───────────────────────────────
    // 制限 : ・引数①②の大小関係
    //        ・引数③④の大小関係
    //        ・引数③④⑤の大小関係
    // ───────────────────────────────
    if (strcmp(Cmd,"ROTATE/SETUP") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 6){_ResChkErr(sp); return;}

        // 1.2.単項目チェック
        int from,to,ps,pe,pn;
        if(!comChannel(dat[1], from, false) ||
           !comChannel(dat[2], to,   true ) ||
           !comPWM_val(dat[3], ps,   false) ||
           !comPWM_val(dat[4], pe,   false) ||
           !comPWM_val(dat[5], pn,   true ) ){_ResChkErr(sp); return;}

        // 1.3.データ補正
        if(to == -1) to = from;                         // 単一チャンネル(=From)
        if(pn == -1) pn = int((ps + pe + 1) / 2); // 中間を自動計算

        // 1.4.相関チェック
        if(from >  to ||  // ①②チャンネルID
           ps   >= pe ||  // ③④ＰＷＭ値
           pn   <= ps ||  // ⑤③ＰＷＭ値(中間)
           pn   >= pe     // ⑤④ＰＷＭ値(中間)
        ){_ResChkErr(sp); return;}

      // ２．主要データ取得：
      int pwmFront = (pn - ps); // ＰＷＭ値(前半)
      int pwmRear  = (pe - pn); // ＰＷＭ値(後半)

      // ３．プリセット登録：
      for (int ch =from; ch <= to; ++ch){
        typePresetPwm &tbl   = g_TBL_ROTATE[ch];
        tbl.enable           = 1          ;
        tbl.pwm.value.start  = ps;
        tbl.pwm.value.end    = pe;
        tbl.pwm.value.middle = pn;
        tbl.pwm.width.front  = pwmFront   ;
        tbl.pwm.width.rear   = pwmRear    ;
        tbl.pwm.width.middle = 0          ; // ※未使用
      }

      // ４．後処理：
      _ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 名称 : PRO 
    // 機能 : ＰＷＭ出力（回転率％指定）
    // ───────────────────────────────
    // 引数 : ① チャンネルID；0～127 PCA9685x8個
    //        ② 回転率　　　；-100～+100%
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [PRO!!]
    // ───────────────────────────────
    // 制限 : とくになし
    // ───────────────────────────────
    if (strcmp(Cmd,"ROTATE/OUTPUT") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 3){_ResChkErr(sp); return;}

        // 1.2.単項目チェック
        int ch, arrow;
        if(!comChannel(dat[1], ch, false       ) ||
           !_Str2Int (dat[2], arrow, -100, 100) ){_ResChkErr(sp); return;}

        // 1.3.機能チェック
        typePresetPwm &tbl = g_TBL_ROTATE[ch];
        if(!tbl.enable){_ResIniErr(sp); return;}

      // ２．主要データ取得：
      int ground = 0;                     // 中間基準値
      float   rate   = (float)arrow / 100.0f; //
      if(rate < ground) rate = 1.0f + rate;   // 比率

      // ３．ＰＷＭ出力：
      int res = comOutPWM(
        ch         ,   // チャンネルID
        arrow      ,   // 指定値
        ground              ,   // 中間基準値
        rate                ,   // 比率
        tbl.pwm.width.front ,   // PWM幅(前半)
        tbl.pwm.width.rear  ,   // PWM幅(後半)
        tbl.pwm.value.start ,   // PWM値(開始)
        tbl.pwm.value.middle
      );  // PWM値(中間)

      // ４．後処理：
      if(res < 0) _ResChkErr(sp);
      else        _ResValue(sp, res);
      return;
    }

  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // 共通コマンド
  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 名称 : DELETE 
    // 機能 : チャンネル別プリセット削除
    // ───────────────────────────────
    // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
    //        ② チャンネルID(終了)；0～127,-1(単一チャンネル)
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [PAD!!] または [PRD!!]
    // ───────────────────────────────
    // 制限 : ・引数①②の大小関係
    // ───────────────────────────────
    if (strcmp(Cmd,"ANGLE/DELETE") == 0 || strcmp(Cmd,"ROTATE/DELETE") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 3){_ResChkErr(sp); return;}

        // 1.2.単項目チェック
        int from, to;
        if(!comChannel(dat[1], from, false) ||
           !comChannel(dat[2], to,   true ) ){_ResChkErr(sp); return;}

        // 1.3.データ補正
        if(to == -1) to = from;

        // 1.4.相関チェック
        if(from > to){_ResChkErr(sp); return;}
      
      // ２．プリセット削除：
      if(strcmp(Cmd,"ANGLE/DELETE") == 0) InitAngle (from, to);
      else                                InitRotate(from, to);

      // ３．後処理：
      _ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 名称 : RRN,PAN 
    // 機能 : ニュートラル（中央位置）
    // ───────────────────────────────
    // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [RRN!!] または [PAN!!]
    // ───────────────────────────────
    // 制限 : なし
    // ───────────────────────────────
    if (strcmp(Cmd,"ANGLE/CENTER") == 0 || strcmp(Cmd,"ROTATE/STOP") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 2){_ResChkErr(sp); return;}

        // 1.2.単項目チェック(チャンネル番号)
        int ch;
        if(!comChannel(dat[1], ch, false)){_ResChkErr(sp); return;}

      // 1.3.機能チェック
      int val;
      if(strcmp(Cmd,"ANGLE/CENTER")==0){
        typePresetAngle &tbl = g_TBL_ANGLE[ch];
        if(!tbl.enable){_ResIniErr(sp); return;}
        val = tbl.pwm.value.middle;
      } else {
        typePresetPwm &tbl = g_TBL_ROTATE[ch];
        if(!tbl.enable){_ResIniErr(sp); return;}
        val = tbl.pwm.value.middle;
      }

      // ２．ＰＷＭ出力：
      g_PWM[int(ch/16)].setPWM((ch% 6), 0, val);

      // ３．後処理：
      _ResValue(sp, val);
      return;
    }

    //━━━━━━━━━━━━━━━━━
    // コマンド名エラー
    //━━━━━━━━━━━━━━━━━
    _ResNotCmd(sp);
    return;
  }

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 内部ヘルパー
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // ───────────────────────────────
  // 機能 : ＰＷＭ出力
  // ───────────────────────────────
  // 引数 : ① チャンネルID；0～127 PCA9685x8個
  //        ② 指定値 　　 ；角度 または パーセンテージ
  //        ③ 中間基準値　；指定値と比較する中点
  //        ④ 変化比率　　；中間から見た変化率
  //        ⑤ PWM幅(前半) ；開始～中間までの幅
  //        ⑥ PWM幅(後半) ；中間～終了までの幅
  //        ⑦ PWM値(前半) ；開始のＰＷＭ値
  //        ⑧ PWM値(中間) ；中間のＰＷＭ値
  // ───────────────────────────────
  // 戻値 : 出力したＰＷＭ値
  // ───────────────────────────────
  // 制限 : なし
  // ───────────────────────────────
  int comOutPWM(
    int argCh        ,  // チャンネルID
    int argArrow     ,  // 指定値
    int argGround    ,  // 中間基準値
    float   argRate      ,  // 変化比率
    int argWidthFront,  // PWM幅(前半)
    int argWidthRear ,  // PWM幅(後半)
    int argStart     ,  // PWM値(前半)
    int argMiddle       // PWM値(中間)
  ){
    // １．指定値(中間/前半/後半)に応じて、出力値を求める：
    int val = argMiddle;
    if(argArrow < argGround) val = argStart + argWidthFront * argRate;
    if(argArrow > argGround) val = argStart + argWidthFront + argWidthRear * argRate;

    // ２．ＰＷＭ出力：
    g_PWM[int(argCh/16)].setPWM((argCh%16), 0, val);

    // ３．後処理：
    return val;
  }

  // ───────────────────────────────
  // 機能 : チャンネル番号チェッカー
  // ───────────────────────────────
  // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
  //        ② 結果代入 　     　；変換値の格納先
  //        ③ 単一チャンネル許可；true=有効／false=無効
  // ───────────────────────────────
  // 戻値 : 合否；true=正常／false=違反
  // ───────────────────────────────
  // 制限 : 最大チャンネル番号以内 または 単一チャンネル(-1)
  // ───────────────────────────────
  int comChannel(
    const char* argVal ,  //① チャンネルID
    int&        argOut ,  //② 結果代入
    bool        argZero   //③ 単一チャンネル許可
  ){
    int min = 0;
    if(argZero) min = -1;
    return _Str2Int(argVal, argOut, min, g_MaxLogCh);
  }

  // ───────────────────────────────
  // 機能 : ＰＷＭ値チェッカー
  // ───────────────────────────────
  // 引数 : ① ＰＷＭ値      　；出力PWM値(引数値)
  //        ② 結果代入      　；変換値の格納先
  //        ③ 中間自動計算許可；true=有効／false=無効
  // ───────────────────────────────
  // 戻値 : 合否；true=正常／false=違反
  // ───────────────────────────────
  // 制限 : 最大チャンネル番号以内 または 中間自動計算(-1)
  // ───────────────────────────────
  int comPWM_val(
    const char* argVal ,  // ＰＷＭ値
    int&        argOut ,  // 結果代入
    bool        argZero   // 中間自動計算許可；
  ){
    int min = 0;
    if(argZero) min = -1;
    return _Str2Int(argVal, argOut, min, 4095);
  }
};
