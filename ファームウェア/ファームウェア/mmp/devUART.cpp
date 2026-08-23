// filename : devUART.cpp
//========================================================
// 通信デバイス初期化：ＵＡＲＴポート
//--------------------------------------------------------
// 2026/08/21 : 大幅リファクタリング 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <Adafruit_NeoPixel.h>
  //│
  //■ＭＭＰシステム
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // RGB-LED
  //─────────────────
  extern Adafruit_NeoPixel INO_PIXEL;

//########################################################
//# 専用名の前空間
//########################################################
namespace devUART {
//========================================================
// 基本情報
//========================================================
  bool ENABLED = false; // 有効判定：有効：true、無効：false

//========================================================
// ハードウェア
//========================================================

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

//========================================================
// メイン処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //----------------------------------
  // 戻り値：処理結果（論理値）
  // ・true ：失敗
  // ・false：成功
  //━━━━━━━━━━━━━━━━━
  void START(){

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

    // UARTポートを起動
    Serial.begin(BAUD_PRESETS[id]);                      // USB(CDC)
    Serial.setDebugOutput(false);                        // SDKデバッグ出力を抑止
    Serial1.begin(BAUD_PRESETS[id], SERIAL_8N1, 44, 43); // GPIO Serial
    delay(3000); // 安定するまで待つ
    
    // 起動メッセージを表示
    Serial.println("---------------------------");
    Serial.println("<<通信デバイスの初期化>>");
    Serial.println(" [Serial device]"  );
    Serial.println("　USB (CDC)      : OK");
    Serial.println("　UART(#01)      : OK");
    Serial.println("  [OK] 初期化が完了");
    Serial.println("");

    //○有効性セット
    ENABLED = true;
  } /* START() */
} /* namespace devUART */