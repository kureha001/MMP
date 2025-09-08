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
  Serial.println("０.システム情報");
  Serial.println("　・バージョン  : " + mmp.Info.Version());
  Serial.println("　・PCA9685 [0] : 0x" + String(mmp.Info.Dev.Pwm(0))   );
  Serial.println("　・DFPlayer[1] : 0x" + String(mmp.Info.Dev.Audio(1)) );
  Serial.println("　[終了]\n");
}

//============================================================
// 1) アナログ入力(HC4067)
//============================================================
void RunAnalog() {

  Serial.println("１.アナログ入力（ HC4067：JoyPad1,2 ）");

  bool ok = mmp.Analog.Configure(2, 4);
  Serial.println("　・アクセス範囲指定 → [2,4]  : " + String(ok ? "True" : "False") );
  if (!ok) { Serial.println("　 <<中断>>\n"); return; }

  ok = mmp.Analog.Update();
  Serial.println("　・アナログ値をバッファに格納 : " + String(ok ? "True" : "False") );
  if (!ok) { Serial.println(" 　<<中断>>\n"); return; }

  Serial.println("　・バッファを参照");
  for (int x = 0; x <= 1; x += 1) {
    Serial.println  ("　　JoyPad[" + String(x + 1) + "]");
    for (int y = 0; y <= 3; y += 1) {
      Serial.println("　　　← [" + String(y) + "] = " + String(mmp.Analog.Read(x, y)) );
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
    Serial.println("　　←[2] = " + String((mmp.Digital.In(2) == 0 ? "ON" : "OFF") ));
    Serial.println("　　←[6] = " + String((mmp.Digital.In(6) == 0 ? "ON" : "OFF") ));
    Serial.println("　　←[7] = " + String((mmp.Digital.In(7) == 0 ? "ON" : "OFF") ));

    Serial.println("　・出力[3]" );
    for (int cnt_eq = 0; cnt_eq <= 5; cnt_eq++)
    {
        Serial.println("　　→ HIGH : " + String(mmp.Digital.Out(3, 1) ? "True" : "False" ) ); delay(500);
        Serial.println("　　→ LOW  : " + String(mmp.Digital.Out(3, 0) ? "True" : "False" ) ); delay(500);
    }

    Serial.println("　[終了]\n");
}

//============================================================
// 3) MP3：フォルダ1のトラック再生,リピート再生
//============================================================
void RunMp3Playlist() {

  Serial.println("３.ＭＰ３再生（ DFPlayer ）");

  Serial.println("　・音量 → 20 : "         + String(mmp.Audio.Volume(1, 20)               ? "True" : "False" ) );
  Serial.println("　・ループ → OFF : "      + String(mmp.Audio.Play.SetLoop(1,0)           ? "True" : "False" ) );

  Serial.println("　・再生");
  for (int track = 1; track <= 3; ++track) {
    Serial.print("　　→ F=1,T=" + String(track) + " : "
                                            + String(mmp.Audio.Play.FolderTrack(1, 1, track) ? "True" : "False" ) );
    delay(100); Serial.println(" : 状況 = " + String(mmp.Audio.Read.PlayState(1)                                ) );
    delay(3000);
  }

  Serial.print  ("　・停止 : "              + String(mmp.Audio.Play.Stop(1)                  ? "True" : "False" ) );
  delay(100); Serial.println(" : 状況 = "   + String(mmp.Audio.Read.PlayState(1)                                ) );

  Serial.println("　・再生 → F=2,T=102 : " + String(mmp.Audio.Play.FolderTrack(1, 2, 102)   ? "True" : "False" ) );
  Serial.print  ("　・ループ → ON : "      + String(mmp.Audio.Play.SetLoop(1, 1)            ? "True" : "False" ) );
  delay(100); Serial.println(" : 状況 = "   + String(mmp.Audio.Read.PlayState(1)                                ) );
  delay(10000);

  Serial.print("　・停止 : "                + String(mmp.Audio.Play.Stop(1)                  ? "True" : "False" ) );
  delay(100); Serial.println(" : 状況 = "   + String(mmp.Audio.Read.PlayState(1)                                ) );

  Serial.println("　[終了]\n");
}

//============================================================
// 4) MP3 制御：状態取得/一時停止/再開/停止/EQ/音量
//============================================================
void RunMp3Control() {

  Serial.println("４.ＭＰ３制御（ DFPlayer ）");
 
  Serial.println("　・音量 → 20 : "      + String(mmp.Audio.Volume(1, 20)             ? "True" : "False" ) );
  Serial.println("　・再生 → F=4,T=1 : " + String(mmp.Audio.Play.FolderTrack(1, 4, 1) ? "True" : "False" ) );
  Serial.println("　・ループ → OFF : "   + String(mmp.Audio.Play.SetLoop(1, 0)        ? "True" : "False" ) );
 
  Serial.println("　・参照");
  Serial.println("　　← 状況         = " + String(mmp.Audio.Read.PlayState(1)                            ) );
  Serial.println("　　← 音量         = " + String(mmp.Audio.Read.Volume(1)                               ) );
  Serial.println("　　← イコライザ   = " + String(mmp.Audio.Read.Eq(1)                                   ) );
  Serial.println("　　← 総ファイル数 = " + String(mmp.Audio.Read.FileCounts(1)                           ) );
  Serial.println("　　← 現在ファイル = " + String(mmp.Audio.Read.CurrentFileNumber(1)                    ) );

  Serial.print  ("　・一時停止 : "        + String(mmp.Audio.Play.Pause(1)             ? "True" : "False" ) );
  delay(100); Serial.println(" : 状況 = " + String(mmp.Audio.Read.PlayState(1)                            ) );
  delay(2000);
  
  Serial.print  ("　・再開 : "            + String(mmp.Audio.Play.Resume(1)            ? "True" : "False" ) );
  delay(100); Serial.println(" : 状況 = " + String(mmp.Audio.Read.PlayState(1)                            ) );

  // イコライザのモードを順次切り替える
  Serial.println("　・イコライザー");
  for (int cnt_eq = 0; cnt_eq <= 5; ++cnt_eq) {
    Serial.println("　　→ " + String(cnt_eq) + " : "
                                          + String(mmp.Audio.SetEq(1, cnt_eq)          ? "True" : "False" ) );
    delay(3000);
  }

  Serial.println("　・音量");
  for (int cnt_vol = 0; cnt_vol <= 30; cnt_vol += 5) {
    Serial.println("　　→ " + String(cnt_vol) + " : "
                                          + String(mmp.Audio.Volume(1, cnt_vol)        ? "True" : "False" ) );
    delay(1000);
  }

  Serial.print("　・停止 : "              + String(mmp.Audio.Play.Stop(1)              ? "True" : "False" ) );
  delay(100); Serial.println(" : 状況 = "   + String(mmp.Audio.Read.PlayState(1)                          ) );

  Serial.println("　[終了]\n");
}

//============================================================
// 5) PWM 生値：ch0=180度型、ch15=連続回転型
//============================================================
void RunPwmByValue() {

  Serial.println("５.ＰＷＭ（ PCA9685：サーボモータ180度型,連続回転型 ）");

  const int SERVO_MIN = 150;
  const int SERVO_MAX = 600;
  const int SERVO_MID = (SERVO_MIN + SERVO_MAX) / 2;
  int chAngle = 0;
  int chRot   = 15;

  Serial.println("　・初期位置");
  mmp.Pwm.Out(chAngle, SERVO_MID);
  mmp.Pwm.Out(chRot,   SERVO_MID);
  delay(300);

  int steps        = 80;
  int delayMs      = 20;
  int rotOffsetMax = 60;

  Serial.println("　・正転,加速");
  for (int i = 0; i <= steps; ++i) {
    int pwmAngle = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i / steps;
    int pwmRot   = SERVO_MID + (rotOffsetMax * i) / steps;
    mmp.Pwm.Out(chAngle, pwmAngle);
    mmp.Pwm.Out(chRot,   pwmRot);
    delay(delayMs);
  }

  Serial.println("　・逆転,減速");
  for (int i = steps; i >= 0; --i) {
    int pwmAngle = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i / steps;
    int pwmRot   = SERVO_MID + (rotOffsetMax * i) / steps;
    mmp.Pwm.Out(chAngle, pwmAngle);
    mmp.Pwm.Out(chRot,   pwmRot);
    delay(delayMs);
  }

  Serial.println("　・初期位置");
  mmp.Pwm.Out(chRot,   SERVO_MID);
  mmp.Pwm.Out(chAngle, SERVO_MID);

  Serial.println("　[終了]\n");
}

//============================================================
// 6) I2C：PCA9685 を直接叩いてサーボスイープ（追加）
//============================================================
void RunI2cServoSweep() {

  Serial.println("６.I2C（ PCA9685：サーボスイープ ）");

  const int PCA_ADDR = 0x40;     // 一般的な PCA9685 の I2C アドレス
  const int CH       = 0;        // CH0 を使用
  const int baseReg  = 0x06 + 4 * CH;  // LEDn_ON_L(0x06) 以降 4byte/CH

  // ON を 0 に固定
  mmp.I2c.Write(PCA_ADDR, baseReg + 0, 0x00); // ON_L
  mmp.I2c.Write(PCA_ADDR, baseReg + 1, 0x00); // ON_H

  const int SERVO_MIN = 150;
  const int SERVO_MAX = 600;
  const int SERVO_MID = (SERVO_MIN + SERVO_MAX) / 2;

  // 初期位置
  mmp.I2c.Write(PCA_ADDR, baseReg + 2, (SERVO_MID & 0xFF));        // OFF_L
  mmp.I2c.Write(PCA_ADDR, baseReg + 3, ((SERVO_MID >> 8) & 0x0F)); // OFF_H(12bit)
  delay(300);

  const int steps   = 80;
  const int delayMs = 20;

  Serial.println("　・スイープ(往路)");
  for (int i = 0; i <= steps; ++i) {
    int v = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i / steps;
    mmp.I2c.Write(PCA_ADDR, baseReg + 2, (v & 0xFF));
    mmp.I2c.Write(PCA_ADDR, baseReg + 3, ((v >> 8) & 0x0F));
    delay(delayMs);
  }

  Serial.println("　・スイープ(復路)");
  for (int i = steps; i >= 0; --i) {
    int v = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i / steps;
    mmp.I2c.Write(PCA_ADDR, baseReg + 2, (v & 0xFF));
    mmp.I2c.Write(PCA_ADDR, baseReg + 3, ((v >> 8) & 0x0F));
    delay(delayMs);
  }

  Serial.println("　・初期位置");
  mmp.I2c.Write(PCA_ADDR, baseReg + 2, (SERVO_MID & 0xFF));
  mmp.I2c.Write(PCA_ADDR, baseReg + 3, ((SERVO_MID >> 8) & 0x0F));

  Serial.println("　[終了]\n");
}


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

  Serial.println("自動検出 " + String(mmp.Settings.BaudRate) + " bps\n");

  Serial.println("＝＝＝ ＭＭＰ ＡＰＩテスト［開始］＝＝＝\n");
  //RunInfo()         ; // 情報系
  RunAnalog()       ; // アナログ入力
  //RunDigital()      ; // デジタル入出力
  //RunMp3Playlist()  ; // MP3プレイヤー(基本)
  //RunMp3Control()   ; // MP3プレイヤー(制御)
  //RunPwmByValue()   ; // PWM出力
  //RunI2cServoSweep(); // I2C→PCA9685 直接制御
  Serial.println("＝＝＝ ＭＭＰ ＡＰＩテスト［終了］＝＝＝");
}

void loop() {}