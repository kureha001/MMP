//============================================================
// ＭＭＰ ライブラリ：ＡＰＩテスト プログラム
//============================================================
#include <Arduino.h>
#include "MmpClient.h"
using Mmp::Core::MmpClient;
MmpClient mmp;

//============================================================
// 自動接続
//============================================================
bool ConnectAuto() {
  Serial.print("接続中...");
  bool ok = mmp.ConnectAutoBaud();
  if (!ok) { Serial.println("★失敗: "); return false; }
  Serial.println("成功");
  return true;
}

//============================================================
// 0) 基本情報(バージョン/PCA9685/DFPlayer)
//============================================================
void RunInfo() {
  Serial.println("０.システム情報");
  Serial.print("　・バージョン : "   ); Serial.println(mmp.Info.Version());
  Serial.print("　・PCA9685 [0]: 0x"); Serial.println(mmp.Info.Dev.Pwm(0), HEX);
  Serial.print("　・DFPlayer[1]: 0x"); Serial.println(mmp.Info.Dev.Audio(1), HEX);
  Serial.println("　[終了]");
}

//============================================================
// 1) アナログ入力(HC4067)
//============================================================
void RunAnalog() {

  Serial.println("１.アナログ入力");

  Serial.print("　・アクセス範囲指定 → [2,4]");
  if (!mmp.Analog.Configure(2, 4)) { Serial.println(": ★失敗"); return; }
  Serial.println("");

  Serial.print("　・アナログ値をバッファに格納");
  if (!mmp.Analog.Update()       ) { Serial.println(": ★失敗"); return; }
  Serial.println("");

  Serial.println("　・バッファを参照(JoyPad1,2)");
  for (int x = 0; x <= 1; x += 1) {
    for (int y = 0; y <= 3; y += 1) {
      int val = mmp.Analog.Read(x, y);
      Serial.print("　　├─ [");
      Serial.print(x); Serial.print(","); Serial.print(y);
      Serial.print("] = ");
      Serial.println(val);
    }
  }
  Serial.println("　　┴");

  Serial.println("　[終了]");
}

//============================================================
// 2) MP3：フォルダ1のトラック1～3を3秒おきに再生
//============================================================
void RunMp3Playlist() {

  Serial.println("２.ＭＰ３再生");

  Serial.println("　・音量 → 20");
  mmp.Audio.Volume(1, 20);

  Serial.println("　・再生");
  for (int track = 1; track <= 3; ++track) {
    Serial.print("　　├→ F=1,T="); Serial.print(track);
    String resp = mmp.Audio.Play.FolderTrack(1, 1, track);
    Serial.print(": "); Serial.println(resp);
    delay(3000);
  }
  Serial.println("　　┴");

  Serial.println("　・停止");
  mmp.Audio.Play.Stop(1);  // ★ Play 配下

  Serial.println("　[終了]");
}

//============================================================
// 3) MP3 制御：状態取得/一時停止/再開/停止/EQ/音量
//============================================================
void RunMp3Control() {

  Serial.println("３.ＭＰ３制御");

  Serial.println("　・音量 → 20");
  mmp.Audio.Volume(1, 20);

  Serial.print("　・フォルダ=1 トラック=1");
  String dir = mmp.Audio.Play.FolderTrack(1, 1, 1);
  Serial.print(": "); Serial.println(dir);
  delay(800);

  Serial.println("　・参照");
  Serial.print("　　├─ 状態 = "       ); Serial.println(mmp.Audio.ReadPlayState(1));
  Serial.print("　　├─ 音量 = "       ); Serial.println(mmp.Audio.ReadVolume(1));
  Serial.print("　　├─ イコライザ = "  ); Serial.println(mmp.Audio.ReadEq(1));
  Serial.print("　　└─ 現在ファイル = "); Serial.println(mmp.Audio.ReadCurrentFileNumber(1));

  Serial.print("　・一時停止: ");
  bool ok = mmp.Audio.Play.Pause(1);
  Serial.println(ok ? "true" : "false");
  delay(500);
  Serial.print("　　└─ 再生状態 = ");
  Serial.println(mmp.Audio.ReadPlayState(1));

  Serial.print("　・再開: ");
  ok = mmp.Audio.Play.Resume(1);
  Serial.println(ok ? "true" : "false");
  delay(500);
  Serial.print("　　└─ 再生状態 = ");
  Serial.println(mmp.Audio.ReadPlayState(1));

  // イコライザのモードを順次切り替える
  Serial.println("　・イコライザ");
  for (int mode = 0; mode <= 5; ++mode) {
    bool okEq = mmp.Audio.SetEq(1, mode);
    Serial.print("　　├→ "); Serial.print(mode); Serial.print(": ");
    Serial.println(okEq ? "true" : "false");
    delay(1000);
  }
  Serial.println("　　┴");

  Serial.println("　・音量");
  for (int v = 15; v <= 25; v += 5) {
    bool okVol = mmp.Audio.Volume(1, v);
    Serial.print("　　├→ "); Serial.print(v); Serial.print(": ");
    Serial.println(okVol ? "true" : "false");
    delay(300);
  }
  Serial.println("　　┴");

  ok = mmp.Audio.Play.Stop(1);
  Serial.print("　・停止: ");
  Serial.println(ok ? "true" : "false");
  delay(300);
  Serial.print("　　└─ 再生状態 = ");
  Serial.println(mmp.Audio.ReadPlayState(1));

  Serial.println("　[終了]");
}

//============================================================
// 4) PWM 生値：ch0=180度型、ch15=連続回転
//============================================================
void RunPwmByValue() {

  Serial.println("４.ＰＷＭ(サーボモータ)");

  const int SERVO_MIN = 150;
  const int SERVO_MAX = 600;
  const int SERVO_MID = (SERVO_MIN + SERVO_MAX) / 2;
  int chAngle = 0;
  int chRot   = 15;

  Serial.println("　・初期位置");
  mmp.Pwm.Out(chAngle, SERVO_MID);
  mmp.Pwm.Out(chRot,   SERVO_MID);
  delay(300);

  int steps = 80, delayMs = 20, rotOffsetMax = 60;

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

  Serial.println("　[終了]");
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
  if (!ConnectAuto()) {
    Serial.println("ＭＭＰとの接続に失敗しました...");
    return;
  }

  Serial.println("＝＝＝ ＭＭＰ ＡＰＩテスト［開始］＝＝＝");
  Serial.println("");
  RunInfo()       ; Serial.println(""); // 情報系
  RunAnalog()     ; Serial.println(""); // アナログ入力
  RunMp3Playlist(); Serial.println(""); // MP3プレイヤー(基本)
  RunMp3Control() ; Serial.println(""); // MP3プレイヤー(制御)
  RunPwmByValue() ; Serial.println(""); // PWM出力
  Serial.println("＝＝＝ ＭＭＰ ＡＰＩテスト［終了］＝＝＝");
}

void loop() {}