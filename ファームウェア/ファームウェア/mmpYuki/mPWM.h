// filename : mPWM.h
//========================================================
// モジュール：ＰＷＭ出力
//-------------------------------------------------------- 
// 変更履歴: Ver0.5.00 (2025/10/13)
// - WEB-API書式対応
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
  PCA9685 g_pwm[PWM_COUNT];         // コンテナ

  // 最大ID
  int g_maxDeviceID  = 0; // デバイスID
  int g_maxLogicalCh = 0; // チャンネルID(デバイスID * 16)

//━━━━━━━━━━━━━━━━━
// プリセット情報
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 型：素材
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

  int count = 0;

  Wire.begin(); delay(50);  // ｉ２ｃ通信を開始

  // I2Cアドレス0x40から接続走査
  for (int i = 0; i < PWM_COUNT; i++){
    
    // 接続開始
    uint8_t addr = 0x40 + i;
    Wire.beginTransmission(addr);

    // 接続されている場合
    if(Wire.endTransmission() == 0){
      g_pwm[count] = PCA9685(addr); // オブジェクトを登録
      g_pwm[count].begin();         // 通信開始
      g_pwm[count].setPWMFreq(60);  // 初期設定
      count++;                      // 次を走査
    }
  }

  // 最大IDを求める
  g_maxDeviceID  = count - 1;                     // デバイスID
  g_maxLogicalCh = (g_maxDeviceID + 1) * 16 - 1;  // チャンネルID

  // プリセットを初期化
  InitRotate(0, g_maxLogicalCh);    // 回転サーボ用
  InitAngle (0, g_maxLogicalCh);    // 角度指定用
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
    const char* Cmd = getCommand(dat[0]); // コマンド名を補正

    // ───────────────────────────────
    // 機能 ：機器の状態
    // 書式 : PWX 
    // 機能 : モジュールの接続確認
    // ───────────────────────────────
    // 引数 : なし
    // ───────────────────────────────
    // 戻値 : ・接続数 [XXXXX!]
    // ───────────────────────────────
    if(strcmp(dat[0], "INFO/CONNECT") == 0) {

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 1){ResChkErr(sp); return;}
  
      // ２．値取得：
      int16_t res = (int16_t)g_maxDeviceID;

      // ３．後処理：
      ResHex4(sp, res);
      return;
    }

    // ───────────────────────────────
    // 名称 : OUTPUT 
    // 機能 : ＰＷＭ値を出力する
    // ───────────────────────────────
    // 引数 : ① チャンネルID(開始)；0x00～7F(0～127)PCA9685x8個
    // 　　　 ② ＰＷＭ出力値　　　；0x00～FFF(0～4095)
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [PWM!!]
    // ───────────────────────────────
    if(strcmp(dat[0],"OUTPUT") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt < 3){ResChkErr(sp); return;}

        // 1.2.単項目チェック
        long ch, val;
        if(!Com_Channel(dat[1], ch,  false) ||
           !Com_PWM_val(dat[2], val, false) ){ResChkErr(sp); return;}

      // ２．ＰＷＭ出力：
      g_pwm[int(ch/16)].setPWM((ch%16), 0, (int16_t)val);

      // ３．後処理：
      ResOK(sp);
      return;
    }

  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // 角度指定コマンド
  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 名称 : PAI 
    // 機能 : チャンネル別プリセット登録
    // ───────────────────────────────
    // 引数 : ① チャンネルID(開始)；0x00～7F(0～127)PCA9685x8個
    //        ② チャンネルID(終了)；0x00～7F/0xFF(-1)単一チャンネル
    //        ③ 角度(最大)    ；0x00～168(0～360度)
    //        ④ ＰＷＭ値(最小)；0x00～FFF(0～4095)
    //        ⑤ ＰＷＭ値(最大)；0x00～FFF(0～4095)
    //        ⑥ ＰＷＭ値(中間)；0x00～0FFF(0～4095)/0xFFFF(-1)自動
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
        if(dat_cnt != 7){ResChkErr(sp); return;}

        // 1.2.単項目チェック
        long from, to, re, ps, pe, pn;
        if(!Com_Channel(dat[1], from, false ) ||
           !Com_Channel(dat[2], to,   true  ) ||
           !strHex2long(dat[3], re,   0, 360) ||
           !Com_PWM_val(dat[4], ps,   false ) ||
           !Com_PWM_val(dat[5], pe,   false ) ||
           !Com_PWM_val(dat[6], pn,   true  ) ){ResChkErr(sp); return;}

        // 1.3.データ補正
        if(to == -1) to = from;                         // 単一チャンネル(=From)
        if(pn == -1) pn = (int16_t)((ps + pe + 1) / 2); // 中間を自動計算

        // 1.4.相関チェック
        if(from >  to ||  // ①②チャンネルID
           ps   >= pe ||  // ④⑤ＰＷＭ値
           pn   <= ps ||  // ⑥④ＰＷＭ値(中間)
           pn   >= pe     // ⑥⑤ＰＷＭ値(中間)
        ){ResChkErr(sp); return;}

      // ２．主要データ取得：
      int16_t pwmFront  = (int16_t)(pn - ps)     ; // ＰＷＭ：幅：前半
      int16_t pwmRear   = (int16_t)(pe - pn)     ; // 　　　　　：後半
      int16_t degMiddle = (int16_t)((re + 1) / 2); // 角　度：幅：中間
      int16_t degFront  = degMiddle              ; // 　　　　　：前半
      int16_t degRear   = re - degFront          ; // 　　　　　：後半

      // ３．プリセット登録：
      for (int ch = (int)from; ch <= (int)to; ++ch){
        typePresetAngle &tbl = g_TBL_ANGLE[ch];
        tbl.enable           = 1          ; // 有効性
        tbl.pwm.value.start  = (int16_t)ps; // ＰＷＭ；値；開始
        tbl.pwm.value.end    = (int16_t)pe; // 　　　　　；終了
        tbl.pwm.value.middle = (int16_t)pn; // 　　　　　；中間
        tbl.pwm.width.front  = pwmFront   ; // 　　　；幅；前半
        tbl.pwm.width.rear   = pwmRear    ; // 　　　　　；後半
        tbl.pwm.width.middle = 0          ; // ※未使用
        tbl.deg.front        = degFront   ; // 角　度；幅；前半
        tbl.deg.rear         = degRear    ; // 　　　　　；後半
        tbl.deg.middle       = degMiddle  ; // 　　　　　；中間
      }

      // ４．後処理：
      ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 名称 : PAO 
    // 機能 : ＰＷＭ出力（角度指定）
    // ───────────────────────────────
    // 引数 : ① チャンネルID；0x00～0x7F(0～127)PCA9685x8個
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [PAO!!]
    // ───────────────────────────────
    // 制限 : とくになし
    // ───────────────────────────────
    if (strcmp(Cmd,"ANGLE/OUTPUT") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 3){ResChkErr(sp); return;}

        // 1.2.単項目チェック(チャンネル番号)
        long ch;
        if(!Com_Channel(dat[1], ch, false)){ResChkErr(sp); return;}

        // 1.3.機能チェック
        typePresetAngle &tbl = g_TBL_ANGLE[(int)ch];
        if(!tbl.enable ){ResChkIniErr(sp); return;}

        // 1.4.単項目チェック(角度)
        long arrow;
        int degEnd = tbl.deg.front + tbl.deg.rear;
        if(!strHex2long(dat[2], arrow, 0, degEnd)){ResChkErr(sp); return;}

      // ２．主要データ取得：
        int16_t ground = tbl.deg.middle ; // 中間基準値
        float   rate                    ; // 比率
        if(arrow < ground) rate = (float)arrow / (float)tbl.deg.front;
        else               rate = (float)(arrow - tbl.deg.middle) / (float)tbl.deg.rear;

      // ２．ＰＷＭ出力：
      int16_t res = Com_OutPWM(
        (int16_t)ch         , // チャンネルID
        (int16_t)arrow      , // 指定値
        ground              , // 中間基準値
        rate                , // 比率
        tbl.pwm.width.front , // PWM幅(前半)
        tbl.pwm.width.rear  , // PWM幅(後半)
        tbl.pwm.value.start , // PWM値(開始)
        tbl.pwm.value.middle
      );  // PWM値(中間)

      // ４．後処理：
      if(res < 0) ResChkErr(sp);
      else         ResHex4(sp, (int16_t)res);
      return;
    }

  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // 回転サーボ用コマンド
  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 名称 : PRI 
    // 機能 : チャンネル別プリセット登録
    // ───────────────────────────────
    // 引数 : ① チャンネルID(開始)；0x00～7F(0～127)PCA9685x8個
    //        ② チャンネルID(終了)；0x00～7F/0xFF(-1)単一チャンネル
    //        ③ ＰＷＭ値(逆転最大)；0x00～FFF
    //        ④ ＰＷＭ値(正転最大)；0x00～FFF
    //        ⑤ ＰＷＭ値(中間)　　；0x00～0FFF(0～4095)/0xFFFF(-1)自動
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [PRI!!]
    // ───────────────────────────────
    // 制限 : ・引数①②の大小関係
    //        ・引数③④の大小関係
    //        ・引数③④⑤の大小関係
    // ───────────────────────────────
    if(strcmp(dat[0],"PRI") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 6){ResChkErr(sp); return;}

        // 1.2.単項目チェック
        long from,to,ps,pe,pn;
        if(!Com_Channel(dat[1], from, false) ||
           !Com_Channel(dat[2], to,   true ) ||
           !Com_PWM_val(dat[3], ps,   false) ||
           !Com_PWM_val(dat[4], pe,   false) ||
           !Com_PWM_val(dat[5], pn,   true ) ){ResChkErr(sp); return;}

        // 1.3.データ補正
        if(to == -1) to = from;                         // 単一チャンネル(=From)
        if(pn == -1) pn = (int16_t)((ps + pe + 1) / 2); // 中間を自動計算

        // 1.4.相関チェック
        if(from >  to ||  // ①②チャンネルID
           ps   >= pe ||  // ③④ＰＷＭ値
           pn   <= ps ||  // ⑤③ＰＷＭ値(中間)
           pn   >= pe     // ⑤④ＰＷＭ値(中間)
        ){ResChkErr(sp); return;}

      // ２．主要データ取得：
      int16_t pwmFront = (int16_t)(pn - ps); // ＰＷＭ値(前半)
      int16_t pwmRear  = (int16_t)(pe - pn); // ＰＷＭ値(後半)

      // ３．プリセット登録：
      for (int ch = (int)from; ch <= (int)to; ++ch){
        typePresetPwm &tbl   = g_TBL_ROTATE[ch];
        tbl.enable           = 1          ;
        tbl.pwm.value.start  = (int16_t)ps;
        tbl.pwm.value.end    = (int16_t)pe;
        tbl.pwm.value.middle = (int16_t)pn;
        tbl.pwm.width.front  = pwmFront   ;
        tbl.pwm.width.rear   = pwmRear    ;
        tbl.pwm.width.middle = 0          ; // ※未使用
      }

      // ４．後処理：
      ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 名称 : PRO 
    // 機能 : ＰＷＭ出力（回転率％指定）
    // ───────────────────────────────
    // 引数 : ① チャンネルID；0x00～7F(0～127)PCA9685x8個
    //        ② 回転率　　　；0x00～C7(-100～+100のオフセット[0x63])
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [PRO!!]
    // ───────────────────────────────
    // 制限 : とくになし
    // ───────────────────────────────
    if(strcmp(dat[0],"PRO")==0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 3){ResChkErr(sp); return;}

        // 1.2.単項目チェック
        long ch, arrow;
        if(!Com_Channel(dat[1], ch, false    ) ||
           !strHex2long(dat[2], arrow, 0, 200) ){ResChkErr(sp); return;}

        // 1.3.データ補正
        arrow = arrow - 100; // オフセット値を変換（-100～+100）

        // 1.4.機能チェック
        typePresetPwm &tbl = g_TBL_ROTATE[(int)ch];
        if(!tbl.enable){ResChkIniErr(sp); return;}

      // ２．主要データ取得：
      int16_t ground = 0;                     // 中間基準値
      float   rate   = (float)arrow / 100.0f; //
      if(rate < ground) rate = 1.0f + rate;   // 比率

      // ３．ＰＷＭ出力：
      int16_t res = Com_OutPWM(
        (int16_t)ch         ,   // チャンネルID
        (int16_t)arrow      ,   // 指定値
        ground              ,   // 中間基準値
        rate                ,   // 比率
        tbl.pwm.width.front ,   // PWM幅(前半)
        tbl.pwm.width.rear  ,   // PWM幅(後半)
        tbl.pwm.value.start ,   // PWM値(開始)
        tbl.pwm.value.middle
      );  // PWM値(中間)

      // ４．後処理：
      if(res < 0) ResChkErr(sp);
      else        ResHex4(sp, (int16_t)res);
      return;
    }

  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // 共通コマンド
  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 名称 : DELETE 
    // 機能 : チャンネル別プリセット削除
    // ───────────────────────────────
    // 引数 : ① チャンネルID(開始)；0x00～7F(0～127)PCA9685x8個
    //        ② チャンネルID(終了)；0x00～7F/0xFF(-1)単一チャンネル
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [PAD!!] または [PRD!!]
    // ───────────────────────────────
    // 制限 : ・引数①②の大小関係
    // ───────────────────────────────
    if (strcmp(Cmd,"ANGLE/DELETE") == 0 || strcmp(Cmd,"ROTATE/DELETE") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 3){ResChkErr(sp); return;}

        // 1.2.単項目チェック
        long from, to;
        if(!Com_Channel(dat[1], from, false) ||
           !Com_Channel(dat[2], to,   true ) ){ResChkErr(sp); return;}

        // 1.3.データ補正
        if(to == -1) to = from;

        // 1.4.相関チェック
        if(from > to){ResChkErr(sp); return;}
      
      // ２．プリセット削除：
      if(strcmp(Cmd,"ANGLE/DELETE") == 0) InitAngle ((int)from, (int)to);
      else                                InitRotate((int)from, (int)to);

      // ３．後処理：
      ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 名称 : RRN,PAN 
    // 機能 : ニュートラル（中央位置）
    // ───────────────────────────────
    // 引数 : ① チャンネルID／0x00～0x7F
    // ───────────────────────────────
    // 戻値 : ・正常 [!!!!!]
    //        ・異常 [RRN!!] または [PAN!!]
    // ───────────────────────────────
    // 制限 : なし
    // ───────────────────────────────
    if (strcmp(Cmd,"ANGLE/CENTER") == 0 || strcmp(Cmd,"ROTATE/STOP") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 2){ResChkErr(sp); return;}

        // 1.2.単項目チェック(チャンネル番号)
        long ch;
        if(!Com_Channel(dat[1], ch, false)){ResChkErr(sp); return;}

      // 1.3.機能チェック
      int16_t val;
      if(strcmp(Cmd,"ANGLE/CENTER")==0){
        typePresetAngle &tbl = g_TBL_ANGLE[(int)ch];
        if(!tbl.enable){ResChkIniErr(sp); return;}
        val = tbl.pwm.value.middle;
      } else {
        typePresetPwm &tbl = g_TBL_ROTATE[(int)ch];
        if(!tbl.enable){ResChkIniErr(sp); return;}
        val = tbl.pwm.value.middle;
      }

      // ２．ＰＷＭ出力：
      g_pwm[int(ch/16)].setPWM((ch% 6), 0, val);

      // ３．後処理：
      ResHex4(sp, val);
      return;
    }

    //━━━━━━━━━━━━━━━━━
    // コマンド名エラー
    //━━━━━━━━━━━━━━━━━
    ResNotCmd(sp);
    return;
  }

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 内部ヘルパー
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // ───────────────────────────────
  // 機能 : ＰＷＭ出力
  // ───────────────────────────────
  // 引数 : ① チャンネルID；サーボの接続先
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
  int16_t Com_OutPWM(
    int16_t argCh        ,  // チャンネルID
    int16_t argArrow     ,  // 指定値
    int16_t argGround    ,  // 中間基準値
    float   argRate      ,  // 変化比率
    int16_t argWidthFront,  // PWM幅(前半)
    int16_t argWidthRear ,  // PWM幅(後半)
    int16_t argStart     ,  // PWM値(前半)
    int16_t argMiddle       // PWM値(中間)
  ){
    // １．指定値(中間/前半/後半)に応じて、出力値を求める：
    int16_t val = argMiddle;
    if(argArrow < argGround) val = argStart + argWidthFront * argRate;
    if(argArrow > argGround) val = argStart + argWidthFront + argWidthRear * argRate;

    // ２．ＰＷＭ出力：
    g_pwm[int(argCh/16)].setPWM((argCh%16), 0, val);

    // ３．後処理：
    return val;
  }

  // ───────────────────────────────
  // 機能 : チャンネル番号チェッカー
  // ───────────────────────────────
  // 引数 : ① チャンネルID      ；サーボの接続先(引数値)
  //        ② 結果代入 　     　；変換値の格納先
  //        ③ 単一チャンネル許可；true=有効／false=無効
  // ───────────────────────────────
  // 戻値 : 合否；true=正常／false=違反
  // ───────────────────────────────
  // 制限 : 最大チャンネル番号以内 または 単一チャンネル(0xFF)
  // ───────────────────────────────
  long Com_Channel(
    const char* argVal ,  //① チャンネルID
    long&       argOut ,  //② 結果代入
    bool        argZero   //③ 単一チャンネル許可
  ){
    if(strHex2long(argVal, argOut, 0, g_maxLogicalCh)) return true;
    if(argZero) {
      char* end = nullptr;
      if (strtol(argVal, &end, 16) == 0xFF){argOut = -1; return true;}
    }
    return false;
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
  // 制限 : 最大チャンネル番号以内 または 中間自動計算(0xFFFF)
  // ───────────────────────────────
  long Com_PWM_val(
    const char* argVal ,  // ＰＷＭ値
    long&       argOut ,  // 結果代入
    bool        argZero   // 中間自動計算許可；
  ){
    if(strHex2long(argVal, argOut, 0, 4095)) return true;
    if (argZero) {
      char* end = nullptr;
      if (strtol(argVal, &end, 16) == 0xFFFF){argOut = -1; return true;}
    }
    return false;
  }
};
