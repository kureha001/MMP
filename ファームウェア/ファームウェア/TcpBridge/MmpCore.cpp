// ============================================================
// MmpCore.cpp — Web-API 用の薄い階層ラッパ実装
//   * MmpClient の階層APIをそのまま公開（名前/階層を一致）
//   * UART0 を使う前提（JSON設定で UARTS[0] が対象機器）
//   * 既にブリッジ側で UART は begin 済み＝原則「常時オープン」
//     → ここでは必要最小限の接続確保のみ（再openは極力避ける）
// ============================================================
#include "MmpCore.h"
#include "bridge.hpp"      // UARTS[0] 設定にアクセス
#include <Arduino.h>

using namespace Mmp::Core;

namespace {
  static MmpClient g_cli;

  // 既存設計：UART は起動時に bridge 側で begin 済み。
  // ここでは「未接続なら最小限で確保」だけ行う（再openは避ける）。
  // ※ MmpClient 現実装は UART begin を内包するため、やむを得ず
  //    同一パラメータでの begin を容認（実機で問題無いことが多い）。
  bool ensure_open_impl() {
    if (g_cli.IsOpen()) return true;

    // UART0 の設定（JSON→bridge.hpp 反映）を参照
    // 既設の UARTS[0].baud を用いて接続
    uint32_t baud = UARTS[0].baud;              // 例: 115200
    if (baud == 0) baud = 115200;

    // 既定タイムアウトは MmpClient.Settings にあるが、明示でもよい
    if (!g_cli.ConnectWithBaud(baud, /*io*/ g_cli.Settings.TimeoutIo,
                                      /*vr*/ g_cli.Settings.TimeoutVerify)) {
      // 必要最小限の失敗情報は LastError() で参照可
      return false;
    }
    return true;
  }
} // namespace

namespace MmpCore {

// ---- 内部公開 ----
bool EnsureOpen() { return ensure_open_impl(); }

// ---- Conn ----
namespace Conn {
  bool    IsOpen()          { return g_cli.IsOpen(); }
  int32_t Baud()            { return g_cli.ConnectedBaud(); }
  String  Port()            { return g_cli.ConnectedPort(); }
  String  LastError()       { return g_cli.LastError(); }
}

// ---- Info ----
namespace Info {
  String   Version(int32_t t){ if(!ensure_open_impl()) return String(""); return g_cli.Info.Version(t); }
  uint16_t DevPwm (int dev, int32_t t){ if(!ensure_open_impl()) return 0; return g_cli.Info.Dev.Pwm  (dev, t); }
  uint16_t DevAudio(int dev, int32_t t){ if(!ensure_open_impl()) return 0; return g_cli.Info.Dev.Audio(dev, t); }
}

// ---- Digital ----
namespace Digital {
  int32_t In (int gpio, int32_t t){ if(!ensure_open_impl()) return -1; return g_cli.Digital.In (gpio, t); }
  bool    Out(int gpio, int v, int32_t t){ if(!ensure_open_impl()) return false; return g_cli.Digital.Out(gpio, v, t); }
}

// ---- Analog ----
namespace Analog {
  bool    Configure(int chTtl, int devTtl, int32_t t){ if(!ensure_open_impl()) return false; return g_cli.Analog.Configure(chTtl, devTtl, t); }
  bool    Update   (int32_t t){ if(!ensure_open_impl()) return false; return g_cli.Analog.Update(t); }
  int32_t Read     (int ch, int dev, int32_t t){ if(!ensure_open_impl()) return -1; return g_cli.Analog.Read(ch, dev, t); }

  int32_t ReadRound    (int ch, int dev, int step, int bits, int32_t t){ if(!ensure_open_impl()) return -1; return g_cli.Analog.ReadRound    (ch, dev, step, bits, t); }
  int32_t ReadRoundUp  (int ch, int dev, int step, int bits, int32_t t){ if(!ensure_open_impl()) return -1; return g_cli.Analog.ReadRoundUp  (ch, dev, step, bits, t); }
  int32_t ReadRoundDown(int ch, int dev, int step, int bits, int32_t t){ if(!ensure_open_impl()) return -1; return g_cli.Analog.ReadRoundDown(ch, dev, step, bits, t); }
}

// ---- Pwm ----
namespace Pwm {
  bool Out      (int ch, int val, int32_t t){ if(!ensure_open_impl()) return false; return g_cli.Pwm.Out(ch, val, t); }
  bool AngleInit(int angleMin, int angleMax, int pwmMin, int pwmMax, int32_t t){
    if(!ensure_open_impl()) return false;
    return g_cli.Pwm.AngleInit(angleMin, angleMax, pwmMin, pwmMax, t);
  }
  bool AngleOut (int ch, int angle, int32_t t){ if(!ensure_open_impl()) return false; return g_cli.Pwm.AngleOut(ch, angle, t); }
}

// ---- Audio ----
namespace Audio {
  bool Volume(int dev, int vol, int32_t t){ if(!ensure_open_impl()) return false; return g_cli.Audio.Volume(dev, vol, t); }
  bool SetEq (int dev, int mode, int32_t t){ if(!ensure_open_impl()) return false; return g_cli.Audio.SetEq (dev, mode, t); }

  namespace Play {
    bool Start  (int dev, int dir, int file, int32_t t){ if(!ensure_open_impl()) return false; return g_cli.Audio.Play.Start(dev, dir, file, t); }
    bool SetLoop(int dev, bool enable, int32_t t){ if(!ensure_open_impl()) return false; return g_cli.Audio.Play.SetLoop(dev, enable, t); }
    bool Stop   (int dev, int32_t t){ if(!ensure_open_impl()) return false; return g_cli.Audio.Play.Stop(dev, t); }
    bool Pause  (int dev, int32_t t){ if(!ensure_open_impl()) return false; return g_cli.Audio.Play.Pause(dev, t); }
    bool Resume (int dev, int32_t t){ if(!ensure_open_impl()) return false; return g_cli.Audio.Play.Resume(dev, t); }
  }

  namespace Read {
    int32_t State     (int dev, int32_t t){ if(!ensure_open_impl()) return -1; return g_cli.Audio.Read.State     (dev, t); }
    int32_t Volume    (int dev, int32_t t){ if(!ensure_open_impl()) return -1; return g_cli.Audio.Read.Volume    (dev, t); }
    int32_t Eq        (int dev, int32_t t){ if(!ensure_open_impl()) return -1; return g_cli.Audio.Read.Eq        (dev, t); }
    int32_t FileCounts(int dev, int32_t t){ if(!ensure_open_impl()) return -1; return g_cli.Audio.Read.FileCounts(dev, t); }
    int32_t FileNumber(int dev, int32_t t){ if(!ensure_open_impl()) return -1; return g_cli.Audio.Read.FileNumber(dev, t); }
  }
}

// ---- I2c ----
namespace I2c {
  bool    Write(int addr, int reg, int val, int32_t t){ if(!ensure_open_impl()) return false; return g_cli.I2c.Write(addr, reg, val, t); }
  int32_t Read (int addr, int reg,             int32_t t){ if(!ensure_open_impl()) return -1; return g_cli.I2c.Read (addr, reg, t); }
}

} // namespace MmpCore
