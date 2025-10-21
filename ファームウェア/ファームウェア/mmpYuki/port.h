// filename : openPort.h
//========================================================
// シリアルポートを開く
//-------------------------------------------------------- 
// Ver0.5.02 (2025/10/21)
//   - ESP32-S3-tiny対応：ボーレートGPIOを区別
//   - ESP32-S3-tiny対応：シリアル起動を区別
//========================================================
#include <Adafruit_NeoPixel.h>

// NwoPixel(個数=1) コンテクストのメンバ
#if defined(ARDUINO_ARCH_RP2040)
  #define NEOPIXEL_PIN 16   // Waveshare RP2040 ZERO: WS2812 DIN=GPIO16
#else
  #define NEOPIXEL_PIN 38   // Waveshare ESP32-S3-Tiny: WS2812 DIN=GPIO38
#endif
Adafruit_NeoPixel g_pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

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
  #if defined(ARDUINO_ARCH_RP2040)
    #define SW_PIN_A 2  // bit-0
    #define SW_PIN_B 6  // bit-1
    #define SW_PIN_C 7  // bit-2
  #else
    // RP2040のレイアウトに合わせる
    #define SW_PIN_A 18 // bit-0
    #define SW_PIN_B 14 // bit-1
    #define SW_PIN_C 13 // bit-2
  #endif

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

//━━━━━━━━━━━━━━━━━
// 初期化処理
//━━━━━━━━━━━━━━━━━
void InitPort(){

  pinMode(SW_PIN_A, INPUT_PULLUP);
  pinMode(SW_PIN_B, INPUT_PULLUP);
  pinMode(SW_PIN_C, INPUT_PULLUP);
  delay(10);

  int A = (digitalRead(SW_PIN_A) == LOW) ? 1 : 0;
  int B = (digitalRead(SW_PIN_B) == LOW) ? 1 : 0;
  int C = (digitalRead(SW_PIN_C) == LOW) ? 1 : 0;

  int id = 7;
  if      (A==0 && B==0 && C==0) id = 0;
  else if (A==1 && B==0 && C==0) id = 1;
  else if (A==0 && B==1 && C==0) id = 2;
  else if (A==0 && B==0 && C==1) id = 3;
  else if (A==1 && B==1 && C==0) id = 4;
  else if (A==0 && B==1 && C==1) id = 5;
  else if (A==1 && B==0 && C==1) id = 6;

  // ボーレートに応じてRGB-LEDを点灯
  RGB c = BAUD_COLORS[id]; // 色パターンを取得
  g_pixels.begin();        // RGB-LEDを点灯
  g_pixels.clear();
  g_pixels.setPixelColor(0, g_pixels.Color(c.g, c.r, c.b));
  g_pixels.show();

  // シリアル通信を開始：USB(CDC)用
  Serial.begin(BAUD_PRESETS[id]);

  // シリアル通信を開始：GPIO用
  #if defined(ARDUINO_ARCH_RP2040)
    Serial1.setTX(0);
    Serial1.setRX(1);
    Serial1.begin(BAUD_PRESETS[id]);
  #else
    Serial1.begin(BAUD_PRESETS[id], SERIAL_8N1, 44, 43);
  #endif
}


