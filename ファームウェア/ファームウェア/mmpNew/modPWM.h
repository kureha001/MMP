// filename : modPWM.h
//========================================================
// モジュール：ＰＷＭ出力
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/11) 初版
//========================================================
#pragma once
#include "mod.h"
#include <Wire.h>       // デバイスはi2c制御
#include <PCA9685.h>    // デバイス固有

//━━━━━━━━━━━━━━━
// グローバル変数
//━━━━━━━━━━━━━━━
  // デバイスのコンテナ
  constexpr uint8_t PWM_COUNT = 8;  // 走査デバイス数
  PCA9685 g_PWM[PWM_COUNT];         // コンテナ

  // 最大ID
  int g_MAX_DEVICE_ID  = 0; // デバイスID
  int g_MAX_CHANNEL_ID = 0; // チャンネルID(デバイスID * 16)

//━━━━━━━━━━━━━━━━━
// プリセット情報
//━━━━━━━━━━━━━━━━━
  //─────────────────
  typedef struct {
    int min;      // 最小
    int max;      // 最大
  } typePreset_Base;
  //─────────────────
  typedef struct {
    uint8_t         enable; // 有効無効(0=unset, 1=set)
    uint8_t         deg;    // 最大角度
    typePreset_Base pwm;    // ＰＷＭ値用
  } typePresetAngle;
  //─────────────────
  typedef struct {
    uint8_t         enable; // 0=unset, 1=set
    typePreset_Base right ; // 右回り用
    typePreset_Base left  ; // 左回り用
  } typePresetPwm;
  //─────────────────
  // プリセット情報
  //─────────────────
  typePresetAngle g_TBL_ANGLE[ PWM_COUNT * 16]; // 角度指定用
  typePresetPwm   g_TBL_ROTATE[PWM_COUNT * 16]; // 回転サーボ用


//━━━━━━━━━━━━━━━━━
// ユーティリティ
//━━━━━━━━━━━━━━━━━
  //────────────────
  // 初期化：通常サーボ
  //────────────────
  static void InitAngle(int argFrom, int argTo){
    for (int idx = argFrom; idx <= argTo; idx++){
      typePresetAngle &T = g_TBL_ANGLE[idx];
      T.enable    = 0; // 有効性判定
      T.deg       = 0; // 最大角度
      T.pwm.min   = 0; // PWM値：0度
      T.pwm.max   = 0; //　　　：最大角度
    }
  }
  //────────────────
  // 初期化：回転サーボ
  //────────────────
  static void InitRotate(int argFrom, int argTo){
    for (int idx = argFrom; idx <= argTo; idx++){
      typePresetPwm &T   = g_TBL_ROTATE[idx];
      T.enable    = 0; // 有効性判定
      T.right.min = 0; // PWM値：右回り：0%
      T.right.max = 0; //　　　　　　　：100%
      T.left.min  = 0; // PWM値：左回り：0%
      T.left.max  = 0; //　　　　　　　：100%
    }
  }


//━━━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━━━
static void InitPWM(){

  Serial.println("---------------------------");
  Serial.println("[PCA9685 initialize]");

  // ｉ２ｃ通信を開始
  Wire.begin(16, 15);

  // ちょっと待つ
  delay(50);

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
  g_MAX_DEVICE_ID  = count - 1;       // デバイスID
  g_MAX_CHANNEL_ID = count * 16 - 1;  // チャンネルID

  // プリセットを初期化
  InitRotate(0, g_MAX_CHANNEL_ID);    // 回転サーボ用
  InitAngle (0, g_MAX_CHANNEL_ID);    // 角度指定用

  if (g_MAX_DEVICE_ID < 0) Serial.println(String("　Device  ID : Not Found"));
  else                     Serial.println(String("　Device  ID : 0 ～ ") + String(g_MAX_DEVICE_ID));

  if (g_MAX_CHANNEL_ID < 0) Serial.println(String("　Channel ID : Not Found"));
  else                      Serial.println(String("　Channel ID : 0 ～ ") + String(g_MAX_CHANNEL_ID));

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
    Stream&     sp = MMP_REPLY(ctx);      // 返信出力先（経路抽象）
    LedScope    scopeLed(ctx, led);       // コマンド色のLED発光
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正

    // ───────────────────────────────
    // 機能 ：機器の状態
    // 機能 : モジュールの接続確認
    // 引数 : なし
    // 戻値 : 接続数 [XXXXX!]
    // ───────────────────────────────
    if (strcmp(Cmd,"INFO/CONNECT") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 1){_ResChkErr(sp); return;}
  
      // ２．値取得：
      int res = (g_MAX_DEVICE_ID < 0) ? 0 : (g_MAX_DEVICE_ID + 1);

      // ３．後処理：
      _ResValue(sp, res);
      return;
    }

    // ───────────────────────────────
    // 機能 : ＰＷＭ値を出力する
    // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
    // 　　　 ② ＰＷＭ出力値　　　；0～4095
    // 制限 : とくになし
    // 戻値 : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"OUTPUT") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt < 3){_ResChkErr(sp); return;}

        // 1.2.単項目チェック
        int ch, val;
        if(!comChannel(dat[1], ch, false   ) || // チャンネルID
           !_Str2Int  (dat[2], val, 0, 4095)    // PWM値
          ){_ResChkErr(sp); return;}

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
    // 機能 : チャンネル別プリセット登録
    // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
    //        ② チャンネルID(終了)；0～127,-1(単一チャンネル)
    //        ③ 角度(最大)    ；0～360度
    //        ④ ＰＷＭ値(最小)；0～4095
    //        ⑤ ＰＷＭ値(最大)；0～4095
    // 制限 : ・引数①②の大小関係
    //        ・引数④⑤の大小関係
    //        ・引数④⑤⑥の大小関係
    // 戻値 : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"ANGLE/SETUP") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 6){_ResChkErr(sp); return;}

        // 1.2.単項目チェック
        int from,to, deg, ps,pe;
        if(!comChannel(dat[1], from, false  ) ||  // チャンネルID：開始
           !comChannel(dat[2], to,   true   ) ||  //　　　　　　 ：終了
           !_Str2Int  (dat[3], deg,  0, 360 ) ||  // 最大角度
           !_Str2Int  (dat[4], ps, 0, 4095  ) ||  // PWM値：0度
           !_Str2Int  (dat[5], pe, 0, 4095  )     //　　　：最大角度
          ){_ResChkErr(sp); return;}

        // 1.3.データ補正
        if(to == -1) to = from; // 単一チャンネル(=From)

        // 1.4.相関チェック
        if(from > to || ps >= pe){_ResChkErr(sp); return;}

      // ３．プリセット登録：
      for (int ch = from; ch <= to; ++ch){
        typePresetAngle &tbl = g_TBL_ANGLE[ch];
        tbl.enable  = 1   ; // 有効性判定
        tbl.deg     = deg ; // 最大角度
        tbl.pwm.min = ps  ; // PWM値：0度
        tbl.pwm.max = pe  ; //　　　：最大角度
      }

      // ４．後処理：
      _ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 機能 : ＰＷＭ出力（角度指定）
    // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
    // 　　　 ② 角度　　　        ；0～プリセット角度
    // 制限 : とくになし
    // 戻値 : _ResValue
    // ───────────────────────────────
    if (strcmp(Cmd,"ANGLE/OUTPUT") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 3){_ResChkErr(sp); return;}

        // 1.2.単項目チェック
        int ch, deg;
        if(!comChannel(dat[1], ch, false  ) ||  // チャンネルID
           !_Str2Int  (dat[2], deg, 0, 360)     // 角度
          ){_ResChkErr(sp); return;}

        // 1.3.機能チェック
        typePresetAngle &tbl = g_TBL_ANGLE[ch];
        if(!tbl.enable ){_ResIniErr(sp); return;}   // 有効性判定

        // 1.4.相関チェック
        if(deg > tbl.deg){_ResChkErr(sp); return;}  // 最大角度

      // ２．主要データ取得：
      int val = map(deg, 0, tbl.deg, tbl.pwm.min, tbl.pwm.max);

      // ３．ＰＷＭ出力：
      g_PWM[int(ch/16)].setPWM((ch%16), 0, val);

      // ４．後処理：
      _ResValue(sp, val);
      return;
    }

  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // 回転サーボ用コマンド
  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 機能 : チャンネル別プリセット登録
    // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
    //        ② チャンネルID(終了)；0～127,-1(単一チャンネル)
    //        ③ ＰＷＭ値(右回り最小)；3～4095
    //        ④ ＰＷＭ値(右回り最大)；3～4095
    //        ⑤ ＰＷＭ値(左回り最小)；3～4095
    //        ⑥ ＰＷＭ値(左回り最大)；3～4095
    // 制限 : ・引数①②の大小関係
    //        ・引数③④の大小関係
    //        ・引数⑤⑥の大小関係
    //        ・引数③⑤の大小関係
    // 戻値 : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"ROTATE/SETUP") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 7){_ResChkErr(sp); return;}

        // 1.2.単項目チェック
        int from,to, rs,re, ls,le;
        if(!comChannel(dat[1], from, false) ||  // チャンネルID：開始
           !comChannel(dat[2], to,   true ) ||  //　　　　　　 ：終了
           !_Str2Int  (dat[3], rs, 3, 4095) ||  // PWM値：右回り：0%
           !_Str2Int  (dat[4], re, 3, 4095) ||  // 　　　　　　 ：100%
           !_Str2Int  (dat[5], ls, 3, 4095) ||  // 　　 ：左回り：0%
           !_Str2Int  (dat[6], le, 3, 4095)     // 　　　　　　 ：100%
          ){_ResChkErr(sp); return;}

        // 1.3.データ補正
        if(to == -1) to = from; // 単一チャンネル(=From)

        // 1.4.相関チェック
        if(from >  to ||  // ①②チャンネルID
           rs   >= re ||  // ③④ＰＷＭ値
           le   >= ls ||  // ⑤⑥ＰＷＭ値
           ls   >= rs     // ③⑤ＰＷＭ値
        ){_ResChkErr(sp); return;}

      // ２．プリセット登録：
      for (int ch =from; ch <= to; ++ch){
        typePresetPwm &tbl   = g_TBL_ROTATE[ch];
        tbl.enable    = 1 ; // 有効性判定
        tbl.right.min = rs; // PWM値：右回り：0%
        tbl.right.max = re; //　　　　　　　：100%
        tbl.left.min  = ls; // 　　 ：左回り：0%
        tbl.left.max  = le; //　　　　　　　：100%
      }

      // ４．後処理：
      _ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 機能 : ＰＷＭ出力（回転率％指定）
    // 引数 : ① チャンネルID；0～127 PCA9685x8個
    //        ② 回転率　　　；-100～+100%
    // 制限 : とくになし
    // 戻値 : _ResValue
    // ───────────────────────────────
    if (strcmp(Cmd,"ROTATE/OUTPUT") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 3){_ResChkErr(sp); return;}

        // 1.2.単項目チェック
        int ch, rate;
        if(!comChannel(dat[1], ch, false      ) ||  // チャンネルID
           !_Str2Int  (dat[2], rate, -100, 100)     // 出力比率
           ){_ResChkErr(sp); return;}

        // 1.3.機能チェック
        typePresetPwm &tbl = g_TBL_ROTATE[ch];
        if(!tbl.enable){_ResIniErr(sp); return;}    // 有効性判定

      // ２．主要データ取得：
      int val;
      if (rate == 0) {
        // 中立は「左側最大」と「右側最小」の中間を仮のニュートラルとする
        val = (tbl.left.max + tbl.right.min) / 2;
      } else if (rate > 0) {
        // 正転：right.min → right.max に線形補間
        val = tbl.right.min + (tbl.right.max - tbl.right.min) *  rate / 100;
      } else {
        // 逆転：left.min → left.max に線形補間（rate は負なので絶対値）
        val = tbl.left.min  + (tbl.left.max  - tbl.left.min ) * -rate / 100;
      }

      // ３．ＰＷＭ出力：
      g_PWM[int(ch/16)].setPWM((ch%16), 0, val);

      // ４．後処理：
      _ResValue(sp, val);
      return;
    }

  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // 共通コマンド
  // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 機能 : チャンネル別プリセット削除
    // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
    //        ② チャンネルID(終了)；0～127,-1(単一チャンネル)
    // 制限 : ・引数①②の大小関係
    // 戻値 : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"ANGLE/RESET") == 0 || strcmp(Cmd,"ROTATE/RESET") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if(dat_cnt != 3){_ResChkErr(sp); return;}

        // 1.2.単項目チェック
        int from, to;
        if(!comChannel(dat[1], from, false) ||
           !comChannel(dat[2], to,   true )
           ){_ResChkErr(sp); return;}

        // 1.3.データ補正
        if(to == -1) to = from;

        // 1.4.相関チェック
        if(from > to){_ResChkErr(sp); return;}
      
      // ２．プリセット削除：
      if(strcmp(Cmd,"ANGLE/RESET") == 0) InitAngle (from, to);
      else                               InitRotate(from, to);

      // ３．後処理：
      _ResOK(sp);
      return;
    }

    //━━━━━━━━━━━━━━━━━
    // コマンド名エラー
    //━━━━━━━━━━━━━━━━━
    _ResNotCmd(sp);
    return;
  }

//━━━━━━━━━━━━━━━
// 内部ヘルパー
//━━━━━━━━━━━━━━━
  // ──────────────────────────────
  // 機能 : チャンネル番号チェッカー
  // 引数 : ① チャンネルID(開始)；0～127 PCA9685x8個
  //        ② 結果代入 　     　；変換値の格納先
  //        ③ 単一チャンネル許可；true=有効／false=無効
  // 制限 : 最大チャンネル番号以内 または 単一チャンネル(-1)
  // 戻値 : 合否；true=正常／false=違反
  // ───────────────────────────────
  int comChannel(
    const char* argVal ,  //① チャンネルID
    int&        argOut ,  //② 結果代入
    bool        argZero   //③ 単一チャンネル許可
  ){
    int min = 0;
    if(argZero) min = -1;
    return _Str2Int(argVal, argOut, min, g_MAX_CHANNEL_ID);
  }
};
