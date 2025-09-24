// ============================================================
// MmpCore.h  — Web-API 用の薄い階層ラッパ
//   * UART0 直下の機器を MmpClient 階層APIで操作するための橋渡し
//   * 関数名/階層は MmpClient の【外部公開・階層API】に一致
//   * ここでは宣言のみ。実装は MmpCore.cpp
// ============================================================
#pragma once
#include <Arduino.h>
#include "MmpClient.h"

// 必要に応じて UART0 設定へアクセス（UARTS[0] など）
#include "bridge.hpp"   // UARTS[], NUM_UARTS, etc.

namespace MmpCore {

// 内部: 接続（必要なら確立）。常時オープン方針だが安全のため存在。
bool EnsureOpen();

// ---- 接続プロパティ（Conn） ----
namespace Conn {
  bool    IsOpen();
  int32_t Baud();
  String  Port();
  String  LastError();
}

// ---- 情報（Info） ----
namespace Info {
  String   Version(int32_t timeoutMs = 0);
  uint16_t DevPwm (int devId0to15, int32_t timeoutMs = 0);
  uint16_t DevAudio(int devId1to4, int32_t timeoutMs = 0);
}

// ---- デジタル（Digital） ----
namespace Digital {
  int32_t In (int gpioId,                  int32_t timeoutMs = 0);
  bool    Out(int gpioId, int val0or1,     int32_t timeoutMs = 0);
}

// ---- アナログ（Analog） ----
namespace Analog {
  bool    Configure(int hc4067chTtl, int hc4067devTtl, int32_t timeoutMs = 0);
  bool    Update   (                                  int32_t timeoutMs = 0);
  int32_t Read     (int hc4067ch0to15, int hc4067dev0to3, int32_t timeoutMs = 0);
  // 丸め系（必要なら）
  int32_t ReadRound    (int ch, int dev, int step, int bits = 0, int32_t timeoutMs = 0);
  int32_t ReadRoundUp  (int ch, int dev, int step, int bits = 0, int32_t timeoutMs = 0);
  int32_t ReadRoundDown(int ch, int dev, int step, int bits = 0, int32_t timeoutMs = 0);
}

// ---- PWM（Pwm） ----
namespace Pwm {
  bool Out      (int chId0to255, int val0to4095,                  int32_t timeoutMs = 0);
  bool AngleInit(int angleMin,   int angleMax,  int pwmMin, int pwmMax, int32_t timeoutMs = 0);
  bool AngleOut (int chId0to255, int angle0to180,                 int32_t timeoutMs = 0);
}

// ---- Audio（DFPlayer） ----
namespace Audio {
  // 単独コマンド
  bool Volume(int devId1to4, int vol0to30, int32_t timeoutMs = 0);
  bool SetEq (int devId1to4, int mode0to5, int32_t timeoutMs = 0);

  // 再生
  namespace Play {
    bool Start  (int devId1to4, int dir1to255, int file1to255, int32_t timeoutMs = 0);
    bool SetLoop(int devId1to4, bool enable,                  int32_t timeoutMs = 0);
    bool Stop   (int devId1to4,                               int32_t timeoutMs = 0);
    bool Pause  (int devId1to4,                               int32_t timeoutMs = 0);
    bool Resume (int devId1to4,                               int32_t timeoutMs = 0);
  }

  // 参照
  namespace Read {
    int32_t State     (int devId1to4, int32_t timeoutMs = 0);
    int32_t Volume    (int devId1to4, int32_t timeoutMs = 0);
    int32_t Eq        (int devId1to4, int32_t timeoutMs = 0);
    int32_t FileCounts(int devId1to4, int32_t timeoutMs = 0);
    int32_t FileNumber(int devId1to4, int32_t timeoutMs = 0);
  }
}

// ---- I2C ----
namespace I2c {
  bool    Write(int addr, int reg, int val, int32_t timeoutMs = 0);
  int32_t Read (int addr, int reg,           int32_t timeoutMs = 0);
}

} // namespace MmpCore
