// filename : openPort.h
//========================================================
// シリアルポートを開く
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.01 (2025/10/07)
// - スケッチから分離
//========================================================
#include <Adafruit_NeoPixel.h>

static int  g_baudIndex     = 0; // 現在のボーレートID

//━━━━━━━━━━━━━━━━━
// 通信速度の定義・設定
//━━━━━━━━━━━━━━━━━

  //─────────────────
  // ボーレートのプリセット
  //─────────────────
  static const int BAUD_PRESETS[8] = {
    921600,57600,38400,19200,9600,4800,2400,300
  };

  //─────────────────
  // ボーレート変更スイッチのGPIO
  //─────────────────
  #define SW_PIN_A 2  // bit-0 (LSB)
  #define SW_PIN_B 6  // bit-1
  #define SW_PIN_C 7  // bit-2

  //─────────────────
  // ボーレート別のRGB-LED点灯色
  //─────────────────
  struct RGB { uint8_t r,g,b; };
  static const RGB BAUD_COLORS[8] = {
    /*0:白*/ {10,10,10},
    /*1:緑*/ { 0,10, 0},
    /*2:青*/ { 0, 0,10},
    /*3:水*/ { 0,10,10},
    /*4:黄*/ {10,10, 0},
    /*5:橙*/ {10, 3, 0},
    /*6:紫*/ {10, 0,10},
    /*7:赤*/ {10, 0, 0}
  };

  //─────────────────
  // ボーレート別のRGB-LED点灯色
  //─────────────────
  Adafruit_NeoPixel g_pixels(1, 16, NEO_GRB + NEO_KHZ800);

  //─────────────────
  // ボーレート設定
  //─────────────────
  static int readBaudPreset(){

    pinMode(SW_PIN_A, INPUT_PULLUP);
    pinMode(SW_PIN_B, INPUT_PULLUP);
    pinMode(SW_PIN_C, INPUT_PULLUP);
    delay(10);

    int A = (digitalRead(SW_PIN_A) == LOW) ? 1 : 0;
    int B = (digitalRead(SW_PIN_B) == LOW) ? 1 : 0;
    int C = (digitalRead(SW_PIN_C) == LOW) ? 1 : 0;

    int id = 0;
    if      (A==0 && B==0 && C==0) id = 0;
    else if (A==1 && B==0 && C==0) id = 1;
    else if (A==0 && B==1 && C==0) id = 2;
    else if (A==0 && B==0 && C==1) id = 3;
    else if (A==1 && B==1 && C==0) id = 4;
    else if (A==0 && B==1 && C==1) id = 5;
    else if (A==1 && B==0 && C==1) id = 6;
    else id = 7;

    g_baudIndex = id;
    return BAUD_PRESETS[id];
  }

//━━━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━━━
static void InitPort(){

  // ボーレート設定
  int baud = readBaudPreset();

  // ボーレートに応じてRGB-LEDを点灯
  RGB c = BAUD_COLORS[g_baudIndex]; // 色パターンを取得
  g_pixels.begin();                 // RGB-LEDを点灯
  g_pixels.clear();
  g_pixels.setPixelColor(0, g_pixels.Color(c.g, c.r, c.b));
  g_pixels.show();

  // シリアル通信を開始
  Serial.begin(baud);   // USB用
  Serial1.setTX(0);     // GPIO用
  Serial1.setRX(1);
  Serial1.begin(baud);
}


