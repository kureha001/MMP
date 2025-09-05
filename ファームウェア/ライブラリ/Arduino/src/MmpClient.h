#pragma once
#include <Arduino.h>
#include "MmpHardware.h"

namespace Mmp { namespace Core {

class MmpClient {
public:
  struct MmpSettings {
    int32_t BaudRate       = 115200;
    int32_t TimeoutIo      = 200;
    int32_t TimeoutVerify  = 400;
    int32_t TimeoutGeneral = 400;
    int32_t TimeoutAnalog  = 400;
    int32_t TimeoutPwm     = 400;
    int32_t TimeoutAudio   = 400;
    int32_t TimeoutDigital = 400;
    int32_t TimeoutI2c     = 400;
  };

private:
  HardwareSerial* _uart = &MMP_UART;
  bool   _isOpen = false;
  String _lastError;

  static inline int32_t Resolve(int32_t v, int32_t fb) { return (v > 0) ? v : fb; }
  static inline bool IsFiveBang(const String& s) { return (s.length() == 5 && s[4] == '!'); }
  static inline int   HexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return -1;
  }
  static inline bool TryParseHex4Bang(const String& s, uint16_t& value) {
    if (s.length() != 5 || s[4] != '!') return false;
    uint16_t v = 0;
    for (int i = 0; i < 4; ++i) {
      int d = HexDigit(s[i]);
      if (d < 0) return false;
      v = (uint16_t)((v << 4) | (uint16_t)d);
    }
    value = v; return true;
  }
  static inline String Hex1(int v) { char b[2+1]; snprintf(b, sizeof(b), "%01X", (unsigned)(v & 0xF));    return String(b); }
  static inline String Hex2(int v) { char b[2+1]; snprintf(b, sizeof(b), "%02X", (unsigned)(v & 0xFF));   return String(b); }
  static inline String Hex3(int v) { char b[3+1]; snprintf(b, sizeof(b), "%03X", (unsigned)(v & 0x3FF));  return String(b); }
  static inline String Hex4(int v) { char b[4+1]; snprintf(b, sizeof(b), "%04X", (unsigned)(v & 0xFFFF)); return String(b); }

  inline void ClearInput() { while (_uart->available() > 0) (void)_uart->read(); }

  // =====（内部専用）フラットAPI=====
  String   GetVersion(int32_t timeoutMs);
  uint16_t GetPwx(int32_t deviceId, int32_t timeoutMs);
  uint16_t GetDpx(int32_t id1to4,  int32_t timeoutMs);

  bool     AnalogConfigure(int32_t players, int32_t switches, int32_t timeoutMs);
  bool     AnalogUpdate(int32_t timeoutMs);
  int32_t  AnalogRead(int32_t playerIndex0to15, int32_t switchIndex0to3, int32_t timeoutMs);

  String   PwmValue(int32_t channel, int32_t value0to4095, int32_t timeoutMs);
  bool     PwmInit(int32_t angleMin, int32_t angleMax, int32_t pwmMin, int32_t pwmMax, int32_t timeoutMs);
  String   PwmAngle(int32_t channel, int32_t angle0to180, int32_t timeoutMs);

  int32_t  DigitalIn(int32_t portId, int32_t timeoutMs);
  bool     DigitalOut(int32_t portId, int32_t value0or1, int32_t timeoutMs);

  bool     I2cWrite(int32_t addr, int32_t reg, int32_t value, int32_t timeoutMs);
  int32_t  I2cRead (int32_t addr, int32_t reg, int32_t timeoutMs);

  bool     DfPlayFolderTrack      (int32_t deviceId1to4, int32_t folder1to255, int32_t track1to255, int32_t timeoutMs);
  bool     DfSetLoop              (int32_t deviceId1to4, int32_t on0or1, int32_t timeoutMs);
  bool     DfStop                 (int32_t deviceId1to4, int32_t timeoutMs);
  bool     DfPause                (int32_t deviceId1to4, int32_t timeoutMs);
  bool     DfResume               (int32_t deviceId1to4, int32_t timeoutMs);
  bool     DfVolume               (int32_t deviceId1to4, int32_t vol0to30, int32_t timeoutMs);
  bool     DfSetEq                (int32_t deviceId1to4, int32_t eqMode0to5, int32_t timeoutMs);
  int32_t  DfReadPlayState        (int32_t deviceId1to4, int32_t timeoutMs);
  int32_t  DfReadVolume           (int32_t deviceId1to4, int32_t timeoutMs);
  int32_t  DfReadEq               (int32_t deviceId1to4, int32_t timeoutMs);
  int32_t  DfReadFileCounts       (int32_t deviceId1to4, int32_t timeoutMs);
  int32_t  DfReadCurrentFileNumber(int32_t deviceId1to4, int32_t timeoutMs);

  // 低レイヤ
  String SendCommand(const String& command, int32_t timeoutMs);
  bool   Open(uint32_t baud, int32_t timeoutIo);
  bool   Verify(int32_t timeoutVerify);

public:
  // ================
  // ===== 公開 =====
  // ================
  MmpSettings Settings;
  bool   IsOpen() const { return _isOpen; }
  String PortName() const { return String("UART1"); }
  String LastError() const { return _lastError; }

  bool ConnectAutoBaud(const uint32_t* cand = MMP_BAUD_CANDIDATES, size_t n = MMP_BAUD_CANDIDATES_LEN);
  bool ConnectWithBaud(uint32_t baud, int32_t timeoutIo = 0, int32_t verifyTimeoutMs = 0);
  void Close();

  // ====================
  // ===== 階層 API =====
  // ====================
  // --------------
  // ---- 情報 ----
  // --------------
  class InfoModule {
    MmpClient* _p;
  public:
    class DevModule {
      MmpClient* _p;
    public:
      explicit DevModule(MmpClient* p): _p(p) {}
      uint16_t Pwm  (int32_t deviceId, int32_t timeoutMs = 0) { return _p->GetPwx(deviceId, timeoutMs); }
      uint16_t Audio(int32_t id1to4,  int32_t timeoutMs = 0) { return _p->GetDpx(id1to4,  timeoutMs); }
    } Dev;
    explicit InfoModule(MmpClient* p): _p(p), Dev(p) {}
    String Version(int32_t timeoutMs = 0) { return _p->GetVersion(timeoutMs); }
  } Info;

  // ------------------
  // ---- アナログ ----
  // ------------------
  class AnalogModule {
    MmpClient* _p;
  public:
    explicit AnalogModule(MmpClient* p): _p(p) {}
    bool    Configure(int32_t players, int32_t switches, int32_t timeoutMs = 0) { return _p->AnalogConfigure(players, switches, timeoutMs); }
    bool    Update   (int32_t timeoutMs = 0) { return _p->AnalogUpdate(timeoutMs); }
    int32_t Read     (int32_t playerIndex0to15, int32_t switchIndex0to3, int32_t timeoutMs = 0) { return _p->AnalogRead(playerIndex0to15, switchIndex0to3, timeoutMs); }
  } Analog;

  // -------------
  // ---- PWM ----
  // -------------
  class PwmModule {
    MmpClient* _p;
  public:
    explicit PwmModule(MmpClient* p): _p(p) {}
    uint16_t Info     (int32_t deviceId, int32_t timeoutMs = 0) { return _p->GetPwx(deviceId, timeoutMs); }
    String   Out      (int32_t ch, int32_t value0to4095, int32_t timeoutMs = 0) { return _p->PwmValue(ch, value0to4095, timeoutMs); }
    bool     AngleInit(int32_t aMin, int32_t aMax, int32_t pMin, int32_t pMax, int32_t timeoutMs = 0) { return _p->PwmInit(aMin,aMax,pMin,pMax,timeoutMs); }
    String   AngleOut (int32_t ch, int32_t angle0to180, int32_t timeoutMs = 0) { return _p->PwmAngle(ch, angle0to180, timeoutMs); }
  } Pwm;

  // ------------------------
  // ---- MP3（DFPlayer）----
  // ------------------------
  class AudioModule {
    MmpClient* _p;
  public:
    // ---------------------
    // ---- MP3：再生系 ----
    // ---------------------
    class PlayModule {
      MmpClient* _p;
    public:
      explicit PlayModule(MmpClient* p): _p(p) {}
      bool FolderTrack(int32_t dev, int32_t folder, int32_t track, int32_t timeoutMs = 0) { return _p->DfPlayFolderTrack(dev, folder, track, timeoutMs  ); }
      bool SetLoop    (int32_t dev, bool enable, int32_t timeoutMs = 0                  ) { return _p->DfSetLoop        (dev, enable ? 1 : 0, timeoutMs ); }
      bool Stop       (int32_t dev, int32_t timeoutMs = 0                               ) { return _p->DfStop           (dev, timeoutMs                 ); }
      bool Pause      (int32_t dev, int32_t timeoutMs = 0                               ) { return _p->DfPause          (dev, timeoutMs                 ); }
      bool Resume     (int32_t dev, int32_t timeoutMs = 0                               ) { return _p->DfResume         (dev, timeoutMs                 ); }
    } Play;

    // ---------------------
    // ---- MP3：参照系 ----
    // ---------------------
    class ReadModule {
      MmpClient* _p;
    public:
      explicit ReadModule(MmpClient* p): _p(p) {}
      int32_t PlayState         (int32_t dev, int32_t timeoutMs = 0) { return _p->DfReadPlayState(dev, timeoutMs        ); }
      int32_t Volume            (int32_t dev, int32_t timeoutMs = 0) { return _p->DfReadVolume(dev, timeoutMs           ); }
      int32_t Eq                (int32_t dev, int32_t timeoutMs = 0) { return _p->DfReadEq(dev, timeoutMs               ); }
      int32_t FileCounts        (int32_t dev, int32_t timeoutMs = 0) { return _p->DfReadFileCounts(dev, timeoutMs       ); }
      int32_t CurrentFileNumber (int32_t dev, int32_t timeoutMs = 0) { return _p->DfReadCurrentFileNumber(dev, timeoutMs); }
    } Read;

    // ---------------------
    // ---- MP3：その他 ----
    // ---------------------
    explicit AudioModule(MmpClient* p): _p(p), Play(p), Read(p) {}
    uint16_t Info (int32_t id1to4, int32_t timeoutMs = 0) { return _p->GetDpx(id1to4, timeoutMs); }
    bool     Volume   (int32_t dev, int32_t vol0to30, int32_t timeoutMs = 0) { return _p->DfVolume(dev, vol0to30, timeoutMs); }
    bool     SetEq    (int32_t dev, int32_t eq0to5,   int32_t timeoutMs = 0) { return _p->DfSetEq (dev, eq0to5,   timeoutMs); }
    // （廃止）直下の Read* は Read.* に移行
  } Audio;

  // ------------------
  // ---- デジタル ----
  // ------------------
  class DigitalModule {
    MmpClient* _p;
  public:
    explicit DigitalModule(MmpClient* p): _p(p) {}
    int32_t In(int32_t portId, int32_t timeoutMs = 0) { return _p->DigitalIn(portId, timeoutMs); }
    bool    Out(int32_t portId, int32_t value0or1, int32_t timeoutMs = 0) { return _p->DigitalOut(portId, value0or1, timeoutMs); }
    // （廃止）int32_t Io(int32_t portId, int32_t value0or1, int32_t timeoutMs = 0)
  } Digital;

  // -------------
  // ---- I2C ----
  // -------------
  class I2cModule {
    MmpClient* _p;
  public:
    explicit I2cModule(MmpClient* p): _p(p) {}
    bool    Write(int32_t addr, int32_t reg, int32_t value, int32_t timeoutMs = 0) { return _p->I2cWrite(addr, reg, value, timeoutMs); }
    int32_t Read (int32_t addr, int32_t reg, int32_t timeoutMs = 0) { return _p->I2cRead (addr, reg, timeoutMs); }
  } I2c;

  MmpClient(): Info(this), Analog(this), Pwm(this), Audio(this), Digital(this), I2c(this) {}
};

}} // namespace
