// filename : iniSerial.h
//========================================================
// 資源初期化：シリアル系
//--------------------------------------------------------
// シリアル通信が利用可能な状態にする
//・通信アダプタが必要とする資源を準備
//・通信経路の開始を通信アダプタへ移譲
//・対象通信アダプタ：シリアル
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/11) α版 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  //│
  //■ＭＭＰシステム
  #include "adpSerial.h"  // 通信アダプタ：シリアル
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 共通ＬＥＤオブジェクト
  //─────────────────
  extern Adafruit_NeoPixel INO_PIXEL;

  //─────────────────
  // ボーレートのプリセット
  //─────────────────
  static const int BAUD_PRESETS[8] = {
    921600,
    57600,
    38400,
    19200,
    9600,
    4800,
    2400,
    300
  };

  //─────────────────
  // ボーレート変更スイッチのGPIO
  //─────────────────
  #define SW_PIN_A 18 // bit-0
  #define SW_PIN_B 14 // bit-1
  #define SW_PIN_C 13 // bit-2

  //─────────────────
  // ボーレート別のRGB-LED点灯色
  //─────────────────
  struct RGB { uint8_t r,g,b; };
    static const RGB COLOR_LIST[8] = {
    /*0:白*/ {10,10,10},
    /*1:緑*/ { 0,10, 0},
    /*2:青*/ { 0, 0,10},
    /*3:水*/ { 0,10,10},
    /*4:黄*/ {10,10, 0},
    /*5:橙*/ {10, 3, 0},
    /*6:紫*/ {10, 0,10},
    /*7:赤*/ {10, 0, 0}
  };

//━━━━━━━━━━━━━━━━━
// 初期化処理
//----------------------------------
// 実行元：mmp.ino - setup()
//━━━━━━━━━━━━━━━━━
bool InitSerial(){

  // ボーレート設定ボタンのピンを定義
  pinMode(SW_PIN_A, INPUT_PULLUP);
  pinMode(SW_PIN_B, INPUT_PULLUP);
  pinMode(SW_PIN_C, INPUT_PULLUP);
  delay(10);

  // ボタンを読取
  int A = (digitalRead(SW_PIN_A) == LOW) ? 1 : 0;
  int B = (digitalRead(SW_PIN_B) == LOW) ? 1 : 0;
  int C = (digitalRead(SW_PIN_C) == LOW) ? 1 : 0;

  //ボーレートIDを取得
  int id = 7;
  if      (A==0 && B==0 && C==0) id = 0; // □□□
  else if (A==1 && B==0 && C==0) id = 1; // ■□□
  else if (A==0 && B==1 && C==0) id = 2; // □■□
  else if (A==0 && B==0 && C==1) id = 3; // □□■
  else if (A==1 && B==1 && C==0) id = 4; // ■■□
  else if (A==0 && B==1 && C==1) id = 5; // □■■
  else if (A==1 && B==0 && C==1) id = 6; // ■□■
  else if (A==1 && B==1 && C==1) id = 7; // ■■■

  // ボーレートに応じてRGB-LEDを点灯
  RGB c = COLOR_LIST[id]; // 色パターンを取得
  INO_PIXEL.begin();     // RGB-LEDを点灯
  INO_PIXEL.clear();
  INO_PIXEL.setPixelColor(0, INO_PIXEL.Color(c.g, c.r, c.b));
  INO_PIXEL.show();

  // シリアルポートを起動
  Serial.begin(BAUD_PRESETS[id]);                       // USB(CDC)
  Serial.setDebugOutput(false);                         // SDKデバッグ出力を抑止
  Serial1.begin(BAUD_PRESETS[id], SERIAL_8N1, 44, 43);  // GPIO Serial
  delay(100);
  Serial.flush();
  Serial1.flush();
  delay(100);
  
  // 起動メッセージを表示
  Serial.println("[Serial initialize]"  );
  Serial.println("　USB (CDC)      : OK");
  Serial.println("　UART(GPIO 0,1) : OK");

// 通信アダプタの初期処理(シリアル)に通信経路開始処理を移譲
adpSerial::start();

  return true;
} // InitSerial()