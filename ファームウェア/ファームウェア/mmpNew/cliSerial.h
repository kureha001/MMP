// filename : cliSerial.h
//========================================================
// クライアント：シリアル通信
//-------------------------------------------------------- 
// Ver0.6.00 (2025/xx/xx)
//========================================================
#include <Adafruit_NeoPixel.h>

// NwoPixel(個数=1) コンテクストのメンバ
#define NEOPIXEL_PIN 38   // Waveshare ESP32-S3-Tiny: WS2812 DIN=GPIO38
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
  #define SW_PIN_A 18 // bit-0
  #define SW_PIN_B 14 // bit-1
  #define SW_PIN_C 13 // bit-2

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
bool InitSerial(){

  Serial.println(String("---------------------------"));
  Serial.println(String("シリアルポートを初期化中..."));

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

  // クライアントのハンドルを作成
  Serial.begin(BAUD_PRESETS[id]);                       // USB(CDC)
  Serial1.begin(BAUD_PRESETS[id], SERIAL_8N1, 44, 43);  // GPIO Serial
 
  // USB CDC の列挙待ち（最大2秒）
  Serial.begin(115200);
  unsigned long dl = millis() + 2000;
  while (!Serial && millis() < dl) delay(10);
 
   Serial.println(String("シリアルポートを初期化済み\n"));
 return true;
}