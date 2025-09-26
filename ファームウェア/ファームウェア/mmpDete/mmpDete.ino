// mmp_main.ino
const char* g_Version = "0400!";
//========================================================
//  MMP Firmware : Code Name -- Dete -- 
//-------------------------------------------------------- 
// ボード情報：Waveshae RP2040 Zero
// ボード情報：Waveshae RP2350 Zero
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.00 (2025/09/26)
//  - USBシリアルとUART0を同時に受付対応
//  - DFPlayerをSerial2専用に固定
//  - アナログ入力バッファをクライアント別に独立管理
//  - 16進数パースを厳格化、不正時は <CMD>!! 応答
//  - PWM/DFPlayer/I2C/Digital(POW) にエラーチェック追加
//  - I2Cを7bit限定、読取失敗時 I2R!! 応答
//  - スイッチ3bitで8種ボーレート選択（起動時）
//  - RGB LEDを8色マッピング（白/緑/青/水/黄/橙/紫/赤）
//  - リクエストバッファ長を MmpConfig.h に定義化
//  - 機能ごとにモジュール分割し保守性を改善
//========================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <PCA9685.h>
#include <DFRobotDFPlayerMini.h>

#include "MmpConfig.h"
#include "MmpContext.h"
#include "LedScope.h"
#include "LedPalette.h"
#include "SerialScope.h"
#include "CommandRouterMulti.h"
#include "ModuleAnalog.h"
#include "ModulePwm.h"
#include "ModuleI2C.h"
#include "ModuleDigital.h"
#include "ModuleDF.h"
#include "ModuleInfo.h"

// --- Switch pins ---
// Existing pins (kept): 2,6,7. Per spec, assign the 3-bit order by ascending GPIO number.
#define SW_PIN_A 2  // becomes bit-0 (LSB)
#define SW_PIN_B 6  // becomes bit-1
#define SW_PIN_C 7  // becomes bit-2

// --- UART baud presets (8 patterns) ---
// For now, all mapped to 115200 as per spec. Place currently-used one first if needed.
static const int BAUD_PRESETS[8] = {
  921600,57600,38400,19200,9600,4800,2400,300
};

// ボーレート別のLED点灯色
// ID | A(2) B(6) C(7) | 色
// 0  | 0    0    0    | 白
// 1  | 1    0    0    | 緑
// 2  | 0    1    0    | 青
// 3  | 0    0    1    | 水
// 4  | 1    1    0    | 黄
// 5  | 0    1    1    | 橙
// 6  | 1    0    1    | 紫
// 7  | 1    1    1    | 赤
struct RGB { uint8_t r,g,b; };
static const RGB BAUD_COLORS[8] = {
  /*0:白*/ {10,10,10},
  /*1:緑*/ { 0,10, 0},
  /*2:青*/ { 0, 0,10},
  /*3:水*/ { 0,10,10}, // シアン
  /*4:黄*/ {10,10, 0},
  /*5:橙*/ {10, 3, 0},
  /*6:柴*/ {10, 0,10}, // 紫
  /*7:赤*/ {10, 0, 0},
};
static int g_baudIndex = 0;   // 表のID（0..7）を保持
Adafruit_NeoPixel g_pixels(1, 16, NEO_GRB + NEO_KHZ800);

// --- PCA9685 ---
#define PWM_COUNT 16
bool     g_pwmConnected[PWM_COUNT];
PCA9685  g_pwm[PWM_COUNT] = {
  PCA9685(0x40), PCA9685(0x41), PCA9685(0x42), PCA9685(0x43),
  PCA9685(0x44), PCA9685(0x45), PCA9685(0x46), PCA9685(0x47),
  PCA9685(0x48), PCA9685(0x49), PCA9685(0x4A), PCA9685(0x4B),
  PCA9685(0x4C), PCA9685(0x4D), PCA9685(0x4E), PCA9685(0x4F)
};

// --- DFPlayer (Serial2 only) ---
DFRobotDFPlayerMini g_df[2];
bool g_dfConnected[2] = { false, false }; // only [0] used with Serial2

// --- Analog (HC4067) shared hardware ---
const int g_addressBusGPIOs[4] = {10, 11, 12, 13};
const int g_analogGPIOs[4]     = {26, 27, 28, 29};

// Client-scoped snapshots
int g_anaValues0[16*4]; int g_anaValues1[16*4];
int g_anaSwitchCnt0 = 4, g_anaSwitchCnt1 = 4;
int g_anaPlayerCnt0 = 1, g_anaPlayerCnt1 = 1;

// --- Servo rating ---
int g_servoRS = 0;
int g_servoRE = 180;
int g_servoPS = 150;
int g_servoPE = 600;

// --- Current stream + client index ---
Stream* g_serial = nullptr;
int g_currentClient = 0; // 0=USB, 1=UART0

// --- Context ---
MmpContext g_ctx = {
  .serial = &g_serial,
  .pixels = &g_pixels,
  .pwm = g_pwm,
  .pwmConnected = g_pwmConnected,
  .pwmCount = PWM_COUNT,
  .df = g_df,
  .dfConnected = g_dfConnected,
  .addressBusGPIOs = g_addressBusGPIOs,
  .analogGPIOs = g_analogGPIOs,
  .anaValuesClient0 = g_anaValues0,
  .anaValuesClient1 = g_anaValues1,
  .anaSwitchCntClient0 = &g_anaSwitchCnt0,
  .anaSwitchCntClient1 = &g_anaSwitchCnt1,
  .anaPlayerCntClient0 = &g_anaPlayerCnt0,
  .anaPlayerCntClient1 = &g_anaPlayerCnt1,
  .servoRS = &g_servoRS,
  .servoRE = &g_servoRE,
  .servoPS = &g_servoPS,
  .servoPE = &g_servoPE,
  .versionStr = g_Version,
  .currentClient = &g_currentClient
};

CommandRouterMulti g_router(g_ctx);

// mmp_main.ino（readBaudPreset() を置き換え）
static int readBaudPreset(){
  pinMode(SW_PIN_A, INPUT_PULLUP);
  pinMode(SW_PIN_B, INPUT_PULLUP);
  pinMode(SW_PIN_C, INPUT_PULLUP);
  delay(10);

  // pull-up入力のため論理反転（0=HIGH, 1=LOW）
  int A = (digitalRead(SW_PIN_A) == LOW) ? 1 : 0;  // GPIO2
  int B = (digitalRead(SW_PIN_B) == LOW) ? 1 : 0;  // GPIO6
  int C = (digitalRead(SW_PIN_C) == LOW) ? 1 : 0;  // GPIO7

  // 表の順序でID決定
  // ID0: A=0,B=0,C=0
  // ID1: A=1,B=0,C=0
  // ID2: A=0,B=1,C=0
  // ID3: A=0,B=0,C=1
  // ID4: A=1,B=1,C=0
  // ID5: A=0,B=1,C=1
  // ID6: A=1,B=0,C=1
  // ID7: A=1,B=1,C=1
  int id = 0;
  if (A==0 && B==0 && C==0) id = 0;
  else if (A==1 && B==0 && C==0) id = 1;
  else if (A==0 && B==1 && C==0) id = 2;
  else if (A==0 && B==0 && C==1) id = 3;
  else if (A==1 && B==1 && C==0) id = 4;
  else if (A==0 && B==1 && C==1) id = 5;
  else if (A==1 && B==0 && C==1) id = 6;
  else /*(A==1 && B==1 && C==1)*/ id = 7;

  g_baudIndex = id;      // IDを保存
  return BAUD_PRESETS[id];
}


void setup(){
  // Read 3-bit switch at boot (boot-time only)
  int baud = readBaudPreset();

  // Show baud choice by color
  g_pixels.begin();
  g_pixels.clear();
  {
    RGB c = BAUD_COLORS[g_baudIndex];
    g_pixels.setPixelColor(0, g_pixels.Color(c.g, c.r, c.b));
  }
  g_pixels.show();

  // Init command ports
  Serial.begin(baud);
  Serial1.setTX(0); Serial1.setRX(1); Serial1.begin(baud);

  // I2C + PCA9685
  Wire.begin(); delay(50);
  for (int i=0;i<PWM_COUNT;i++){ g_pwm[i].begin(); g_pwm[i].setPWMFreq(60); g_pwmConnected[i]=true; }

  // DFPlayer #1 on Serial2 (9600 fixed)
  Serial2.begin(9600); delay(50);
  if (g_df[0].begin(Serial2)) { g_df[0].volume(20); g_dfConnected[0]=true; }
  g_dfConnected[1] = false; // always unused per spec

  // Clear analog snapshots
  for (int i=0;i<16*4;i++){ g_anaValues0[i]=0; g_anaValues1[i]=0; }

  // Register modules
  g_router.addModule(new ModuleAnalog(g_ctx, LED_PALETTE_ANALOG));
  g_router.addModule(new ModulePwm   (g_ctx, LED_PALETTE_PWM));
  g_router.addModule(new ModuleI2C   (g_ctx, LED_PALETTE_I2C));
  g_router.addModule(new ModuleDigital(g_ctx, LED_PALETTE_DIGITAL));
  g_router.addModule(new ModuleDF    (g_ctx, LED_PALETTE_DF));
  g_router.addModule(new ModuleInfo  (g_ctx, LED_PALETTE_INFO));

  // Register sources (fixed client indices)
  g_router.addSource(&Serial,  "USB",   0);
  g_router.addSource(&Serial1, "UART0", 1);
}

void loop(){
  g_router.pollAll();
}
