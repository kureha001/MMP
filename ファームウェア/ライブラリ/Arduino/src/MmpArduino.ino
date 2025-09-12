//============================================================
// ＭＭＰ ライブラリ：ＡＰＩテスト プログラム
//============================================================
#include <Arduino.h>
#include "MmpClient.h"
using Mmp::Core::MmpClient;
MmpClient mmp;

//============================================================
// 0) 基本情報(バージョン/PCA9685/DFPlayer)
//============================================================
void RunInfo() {
    Serial.println("　・通信ポート  : "   + mmp.ConnectedPort()                 );
    Serial.println("　・接続速度    : "   + String(mmp.ConnectedBaud()) + "bps" );
    Serial.println("　・バージョン  : "   + mmp.Info.Version()                  );
    Serial.println("　・PCA9685 [0] : 0x" + String(mmp.Info.Dev.Pwm(0))         );
    Serial.println("　・DFPlayer[1] : 0x" + String(mmp.Info.Dev.Audio(1))       );
}

//============================================================
// 1) アナログ入力(HC4067)
//============================================================
void RunAnalog() {

    Serial.println("１.アナログ入力（ HC4067：JoyPad1,2 ）");

    bool ok = mmp.Analog.Configure(2, 4);
    Serial.println("　・アクセス範囲指定 → [2,4]  : " + bool2str(ok) );
    if (!ok) { Serial.println("　 <<中断>>\n"); return; }

    ok = mmp.Analog.Update();
    Serial.println("　・アナログ値をバッファに格納 : " + bool2str(ok) );
    if (!ok) { Serial.println(" 　<<中断>>\n"); return; }

    Serial.println("　・バッファを参照");
    for (int x = 0; x <= 1; x += 1) {
        Serial.println  ("　　JoyPad[" + String(x + 1) + "]");
        for (int y = 0; y <= 3; y += 1) {
        //Serial.println("　　　← [" + String(y) + "] = " + String(mmp.Analog.Read         (x, y         ) ) );
        //Serial.println("　　　← [" + String(y) + "] = " + String(mmp.Analog.ReadRound    (x, y, 100) ) );
        //Serial.println("　　　← [" + String(y) + "] = " + String(mmp.Analog.ReadRoundUp  (x, y, 100) ) );
        Serial.println("　　　← [" + String(y) + "] = " + String(mmp.Analog.ReadRoundDown(x, y, 100) ) );
        }
    }

    Serial.println("　[終了]\n");
}

//============================================================
// 2) デジタル入出力
//============================================================
void RunDigital() {

    Serial.println("２.デジタル入出力（ GPIO ）");
    Serial.println("　・入力");
    Serial.println("　　←[2] = " + int2strOnOff(mmp.Digital.In(2)) );
    Serial.println("　　←[6] = " + int2strOnOff(mmp.Digital.In(6)) );
    Serial.println("　　←[7] = " + int2strOnOff(mmp.Digital.In(7)) );

    Serial.println("　・出力[3]" );
    for (int cnt_eq = 0; cnt_eq <= 5; cnt_eq++)
    {
        Serial.println("　　→ HIGH : " + bool2str(mmp.Digital.Out(3, 1)) ); delay(500);
        Serial.println("　　→ LOW  : " + bool2str(mmp.Digital.Out(3, 0)) ); delay(500);
    }

    Serial.println("　[終了]\n");
}

//============================================================
// 3) MP3：フォルダ1のトラック再生,リピート再生
//============================================================
void RunMp3Playlist() {

    Serial.println("３.ＭＰ３再生（ DFPlayer ）");

    Serial.println("　・音量 → 20 : "    + bool2str(mmp.Audio.Volume(1, 20)     ) );
    Serial.println("　・ループ → OFF : " + bool2str(mmp.Audio.Play.SetLoop(1,0) ) );

    Serial.println("　・再生");
    for (int track = 1; track <= 3; ++track) {
        Serial.print("　　→ F=1,T=" + String(track) + " : "
                                    + bool2str(mmp.Audio.Play.Start(1, 1, track) ) );
        Serial.println(" : 状況 = " + String(mmp.Audio.Read.State(1)             ) );
        delay(3000);
    }

    Serial.print  ("　・停止 : " + bool2str(mmp.Audio.Play.Stop(1) ) );
    Serial.println(" : 状況 = "  + String(mmp.Audio.Read.State(1)  ) );

    Serial.println("　・再生 → F=2,T=102 : " + bool2str(mmp.Audio.Play.Start(1, 2, 102) ) );
    Serial.print  ("　・ループ → ON : "      + bool2str(mmp.Audio.Play.SetLoop(1, 1)    ) );
    Serial.println(" : 状況 = "               + String(mmp.Audio.Read.State(1)           ) );
    delay(10000);

    Serial.print  ("　・停止 : " + bool2str(mmp.Audio.Play.Stop(1) ) );
    Serial.println(" : 状況 = "  + String(mmp.Audio.Read.State(1)  ) );

    Serial.println("　[終了]\n");
}

//============================================================
// 4) MP3 制御：状態取得/一時停止/再開/停止/EQ/音量
//============================================================
void RunMp3Control() {

    Serial.println("４.ＭＰ３制御（ DFPlayer ）");
    
    Serial.println("　・音量 → 20 : "      + bool2str(mmp.Audio.Volume(1, 20)       ) );
    Serial.println("　・再生 → F=4,T=1 : " + bool2str(mmp.Audio.Play.Start(1, 4, 1) ) );
    Serial.println("　・ループ → OFF : "   + bool2str(mmp.Audio.Play.SetLoop(1, 0)  ) );
    
    Serial.println("　・参照");
    Serial.println("　　← 状況         = " + String(mmp.Audio.Read.State(1)      ) );
    Serial.println("　　← 音量         = " + String(mmp.Audio.Read.Volume(1)     ) );
    Serial.println("　　← イコライザ   = " + String(mmp.Audio.Read.Eq(1)         ) );
    Serial.println("　　← 総ファイル数 = " + String(mmp.Audio.Read.FileCounts(1) ) );
    Serial.println("　　← 現在ファイル = " + String(mmp.Audio.Read.FileNumber(1) ) );

    Serial.print  ("　・一時停止 : " + bool2str(mmp.Audio.Play.Pause(1) ) );
    Serial.println(" : 状況 = "      + String(mmp.Audio.Read.State(1)   ) );
    delay(2000);
    
    Serial.print  ("　・再開 : " + bool2str(mmp.Audio.Play.Resume(1) ) );
    Serial.println(" : 状況 = "  + String(mmp.Audio.Read.State(1)    ) );

    // イコライザのモードを順次切り替える
    Serial.println("　・イコライザー");
    for (int cnt_eq = 0; cnt_eq <= 5; ++cnt_eq) {
        Serial.println("　　→ " + String(cnt_eq) + " : " + bool2str(mmp.Audio.SetEq(1, cnt_eq) ) );
        delay(3000);
    }

    Serial.println("　・音量");
    for (int cnt_vol = 0; cnt_vol <= 30; cnt_vol += 5) {
        Serial.println("　　→ " + String(cnt_vol) + " : " + bool2str(mmp.Audio.Volume(1, cnt_vol) ) );
        delay(1000);
    }

    Serial.print  ("　・停止 : " + bool2str(mmp.Audio.Play.Stop(1) ) );
    Serial.println(" : 状況 = "  + String(mmp.Audio.Read.State(1)  ) );

    Serial.println("　[終了]\n");
}

//============================================================
// 5) PWM 生値：ch0=180度型、ch15=連続回転型
//============================================================
const int SERVO_MIN     = 150; // CA9685 12bitの生値（例: 150）
const int SERVO_MAX     = 600; // 同上              （例: 600）
const int SERVO_MID     = (SERVO_MIN + SERVO_MAX) / 2;
const int OffsetMax360  = 60;
const int STEPS         = 80;
const int STEP          = 1;
const int STEP_DELAY_MS = 20;
const int CH_180        = 0;
const int CH_360        = 15;
const int PCA_ADDR      = 0x40;

void RunPwm(bool mode) {

    Serial.println(
        String(mode ? "５.ＰＷＭ" : "６.Ｉ２Ｃ")
        + "（ PCA9685：サーボモータ180度型,連続回転型 ）"
    );

    Serial.println("　・初期位置");
    if (mode){
        mmp.Pwm.Out(CH_180, SERVO_MID);
        mmp.Pwm.Out(CH_360, SERVO_MID);
    } else {
        RunI2C(CH_180, SERVO_MID);
        RunI2C(CH_360, SERVO_MID);
    }
    delay(300);

    Serial.println("　・正転,加速");
    for (int i = 0; i <= STEPS; i+=STEP) {
        int pwm180 = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i  / STEPS;
        int pwm360 = SERVO_MID + (OffsetMax360           * i) / STEPS;
        if (mode)
        {
            mmp.Pwm.Out(CH_180, pwm180);
            mmp.Pwm.Out(CH_360, pwm360);
        }
        else
        {
            RunI2C(CH_180, pwm180);
            RunI2C(CH_360, pwm360);
        }
        delay(STEP_DELAY_MS);
    }

    Serial.println("　・逆転,減速");
    for (int i = STEPS; i >= 0; i-=STEP) {
        int pwm180 = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i   / STEPS;
        int pwm360 = SERVO_MID + (OffsetMax360           * i ) / STEPS;
        if (mode){
            mmp.Pwm.Out(CH_180, pwm180);
            mmp.Pwm.Out(CH_360, pwm360);
        } else {
            RunI2C(CH_180, pwm180);
            RunI2C(CH_360, pwm360);
        }
        delay(STEP_DELAY_MS);
    }

    Serial.println("　・初期位置");
    if (mode){
        mmp.Pwm.Out(CH_180, SERVO_MID);
        mmp.Pwm.Out(CH_360, SERVO_MID);
    } else {
        RunI2C(CH_180, SERVO_MID);
        RunI2C(CH_360, SERVO_MID);
    }

    Serial.println("　[終了]\n");
}

void RunI2C(int ch, int ticks){
    int base_reg = 0x06 + 4 * ch;
    mmp.I2c.Write(PCA_ADDR, base_reg + 2, (ticks     ) & 0xFF);
    mmp.I2c.Write(PCA_ADDR, base_reg + 3, (ticks >> 8) & 0x0F);
}

//============================================================
// ヘルパ
//============================================================
static String bool2str(int source){return String(source ? "True" : "False");}
static String int2strOnOff(int source){return String(source == 0 ? "ON" : "OFF");}

//============================================================
// セットアップ
//============================================================
void setup() {

    // 通信開始
    Serial.begin(115200);
    while (!Serial) {}
    delay(200);

    // 通信速度を自動で検出し接続
    Serial.println("<< ＭＭＰライブラリ for Arduino >>\n");
    Serial.print("接続中...");
    if (!mmp.ConnectAutoBaud()) {
        Serial.println("ＭＭＰとの接続に失敗しました...");
        return;
    }

    Serial.println("");
    RunInfo(); // 情報系

    Serial.println("\n＝＝＝ ＭＭＰ ＡＰＩテスト［開始］＝＝＝\n");
    RunAnalog()       ; // アナログ入力
    RunDigital()      ; // デジタル入出力
    RunMp3Playlist()  ; // MP3プレイヤー(基本)
    RunMp3Control()   ; // MP3プレイヤー(制御)
    RunPwm(true)      ; // PWM出力
    RunPwm(false)     ; // I2C→PCA9685 直接制御
    Serial.println("＝＝＝ ＭＭＰ ＡＰＩテスト［終了］＝＝＝");
}

void loop() {}