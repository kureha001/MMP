#include "MmpClient.h"

using namespace Mmp::Core;

//============================================================
// 低レイヤ I/O
//============================================================
String MmpClient::SendCommand(const String& command, int32_t timeoutMs)
{
  _lastError = "";
  if (!_isOpen) { _lastError = "UART not open."; return String(); }

  // 必ず '!' 終端にする
  String cmd = command;
  if (cmd.length() == 0 || cmd.charAt(cmd.length() - 1) != '!') {
    cmd += '!';
  }

  // 送信前に入力をクリア
  ClearInput();

  // 送信
  _uart->print(cmd);

  // 応答待ち（固定 5 文字）。末尾 5 文字だけ保持。
  String buf;
  unsigned long deadline = millis() + (timeoutMs > 0 ? (unsigned long)timeoutMs : 1UL);

  while ((long)(deadline - millis()) > 0) {
    while (_uart->available() > 0) {
      char c = (char)_uart->read();
      if (c == '\r' || c == '\n') {
        // プロトコルは 5 文字固定フレームなので改行は無視
        continue;
      }
      buf += c;
      if (buf.length() > 5) buf.remove(0, buf.length() - 5);

      if (buf.length() == 5 && buf[4] == '!') {
        return buf; // 5 文字そろった
      }
    }
    delay(1);
  }

  _lastError = "Timeout waiting 5-char response.";
  return String();
}

bool MmpClient::Open(uint32_t baud, int32_t /*timeoutIo*/)
{
  _lastError = "";
  // 既に開いている場合は閉じる
  Close();

  _uart = &MMP_UART;
  _uart->begin(baud);
  delay(100);
  ClearInput();

  _isOpen = true;
  return true;
}

bool MmpClient::Verify(int32_t timeoutVerify)
{
  // VER! に対し 5 文字（末尾 '!'）が返れば接続 OK とみなす（内容は数値前提にしない）
  String s = GetVersion(timeoutVerify);
  return (s.length() == 5 && s[4] == '!');
}

//============================================================
// 接続 API
//============================================================
bool MmpClient::ConnectAutoBaud(const uint32_t* cand, size_t n)
{
  _lastError = "";
  if (cand == nullptr || n == 0) {
    cand = MMP_BAUD_CANDIDATES;
    n = MMP_BAUD_CANDIDATES_LEN;
  }

  for (size_t i = 0; i < n; ++i) {
    if (ConnectWithBaud(cand[i], Settings.TimeoutIo, Settings.TimeoutVerify)) {
      return true;
    }
  }
  _lastError = "Auto-baud connect failed.";
  return false;
}

bool MmpClient::ConnectWithBaud(uint32_t baud, int32_t timeoutIo, int32_t verifyTimeoutMs)
{
  _lastError = "";
  if (!Open(baud, timeoutIo)) {
    if (_lastError.length() == 0) _lastError = "Open failed.";
    return false;
  }
  if (!Verify(verifyTimeoutMs)) {
    _lastError = "Verify failed.";
    Close();
    return false;
  }
  return true;
}

void MmpClient::Close()
{
  if (_isOpen) {
    _uart->flush();
    _uart->end();
    _isOpen = false;
  }
}

//============================================================
// 内部フラット API: 情報
//============================================================
String MmpClient::GetVersion(int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutGeneral);
  return SendCommand("VER!", t);
}

uint16_t MmpClient::GetPwx(int32_t devId0to15, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutPwm);
  String resp = SendCommand("PWX:" + Hex2(devId0to15) + "!", t);
  uint16_t v = 0;
  if (!TryParseHex4Bang(resp, v)) {
    _lastError = "PWX bad response.";
    return 0;
  }
  return v;
}

uint16_t MmpClient::GetDpx(int32_t devId1to4, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("DPX:" + Hex2(devId1to4) + "!", t);
  uint16_t v = 0;
  if (!TryParseHex4Bang(resp, v)) {
    _lastError = "DPX bad response.";
    return 0;
  }
  return v;
}

//============================================================
// 内部フラット API: アナログ
//============================================================
bool MmpClient::AnalogConfigure(int32_t hc4067chTtl, int32_t hc4067devTtl, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAnalog);
  if (hc4067chTtl  < 1 || hc4067chTtl  > 16) { _lastError = "AnalogConfigure: hc4067chTtl out of range." ; return false; }
  if (hc4067devTtl < 1 || hc4067devTtl > 4 ) { _lastError = "AnalogConfigure: hc4067devTtl out of range."; return false; }

  String resp = SendCommand("ANS:" + Hex2(hc4067chTtl) + ":" + Hex2(hc4067devTtl) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::AnalogUpdate(int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAnalog);
  String resp = SendCommand("ANU!", t);
  return (resp == "!!!!!");
}

int32_t MmpClient::AnalogRead(int32_t hc4067ch0to15, int32_t hc4067dev0to3, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAnalog);
  if (hc4067ch0to15 < 0 || hc4067ch0to15 > 15) { _lastError = "AnalogRead: hc4067ch0to15 out of range."; return -1; }
  if (hc4067dev0to3 < 0 || hc4067dev0to3 > 3 ) { _lastError = "AnalogRead: hc4067dev0to3 out of range."; return -1; }

  String resp = SendCommand("ANR:" + Hex2(hc4067ch0to15) + ":" + Hex2(hc4067dev0to3) + "!", t);
  uint16_t v = 0;
  if (TryParseHex4Bang(resp, v)) return (int32_t)v;
  _lastError = "AnalogRead: bad response.";
  return -1;
}

//============================================================
// 内部フラット API: PWM
//============================================================
bool MmpClient::PwmValue(int32_t chId0to255, int32_t val0to4095, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutPwm);
  String resp = SendCommand("PWM:" + Hex2(chId0to255) + ":" + Hex4(val0to4095) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::PwmInit(int32_t angleMin, int32_t angleMax, int32_t pwmMin, int32_t pwmMax, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutPwm);
  String resp = SendCommand("PWI:" + Hex3(angleMin) + ":" + Hex3(angleMax) + ":" + Hex4(pwmMin) + ":" + Hex4(pwmMax) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::PwmAngle(int32_t chId0to255, int32_t angle0to180, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutPwm);
  String resp = SendCommand("PWA:" + Hex2(chId0to255) + ":" + Hex3(angle0to180) + "!", t);
  return (resp == "!!!!!");
}

//============================================================
// 内部フラット API: デジタル I/O
//============================================================
int32_t MmpClient::DigitalIn(int32_t gpioId, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutDigital);
  String resp = SendCommand("POR:" + Hex2(gpioId) + "!", t);
  uint16_t v = 0;
  if (TryParseHex4Bang(resp, v)) return (int32_t)v;
  _lastError = "DigitalIn: bad response.";
  return 0;
}

bool MmpClient::DigitalOut(int32_t gpioId, int32_t val0or1, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutDigital);
  String resp = SendCommand("POW:" + Hex2(gpioId) + ":" + ((val0or1 & 1) ? "1" : "0") + "!", t);
  return (resp == "!!!!!");
}

//============================================================
// 内部フラット API: I2C
//============================================================
bool MmpClient::I2cWrite(int32_t addr, int32_t reg, int32_t val, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutI2c);
  String resp = SendCommand("I2W:" + Hex2(addr) + ":" + Hex2(reg) + ":" + Hex2(val) + "!", t);
  return (resp == "!!!!!");
}

int32_t MmpClient::I2cRead (int32_t addr, int32_t reg, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutI2c);
  String resp = SendCommand("I2R:" + Hex2(addr) + ":" + Hex2(reg) + "!", t);
  uint16_t v = 0;
  if (TryParseHex4Bang(resp, v)) return (int32_t)v;
  _lastError = "I2cRead: bad response.";
  return 0;
}

//============================================================
// 内部フラット API: DFPlayer（Audio）
//============================================================
bool MmpClient::DfStart(int32_t devId1to4, int32_t dir1to255, int32_t file1to255, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("DIR:" + Hex2(devId1to4) + ":" + Hex2(dir1to255) + ":" + Hex2(file1to255) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::DfSetLoop(int32_t devId1to4, int32_t on0or1, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("DLP:" + Hex2(devId1to4) + ":" + Hex2(on0or1 ? 1 : 0) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::DfStop(int32_t devId1to4, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("DSP:" + Hex2(devId1to4) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::DfPause(int32_t devId1to4, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("DPA:" + Hex2(devId1to4) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::DfResume(int32_t devId1to4, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("DPR:" + Hex2(devId1to4) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::DfVolume(int32_t devId1to4, int32_t vol0to30, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("VOL:" + Hex2(devId1to4) + ":" + Hex2(vol0to30) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::DfSetEq(int32_t devId1to4, int32_t mode0to5, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("DEF:" + Hex2(devId1to4) + ":" + Hex2(mode0to5) + "!", t);
  return (resp == "!!!!!");
}

int32_t MmpClient::DfReadState(int32_t devId1to4, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("DST:" + Hex2(devId1to4) + ":" + Hex2(1) + "!", t);
  uint16_t v = 0;
  if (TryParseHex4Bang(resp, v)) return (int32_t)v;
  _lastError = "DfReadState: bad response.";
  return -1;
}

int32_t MmpClient::DfReadVolume(int32_t devId1to4, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("DST:" + Hex2(devId1to4) + ":" + Hex2(2) + "!", t);
  uint16_t v = 0;
  if (TryParseHex4Bang(resp, v)) return (int32_t)v;
  _lastError = "DfReadVolume: bad response.";
  return -1;
}

int32_t MmpClient::DfReadEq(int32_t devId1to4, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("DST:" + Hex2(devId1to4) + ":" + Hex2(3) + "!", t);
  uint16_t v = 0;
  if (TryParseHex4Bang(resp, v)) return (int32_t)v;
  _lastError = "DfReadEq: bad response.";
  return -1;
}

int32_t MmpClient::DfReadFileCounts(int32_t devId1to4, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("DST:" + Hex2(devId1to4) + ":" + Hex2(4) + "!", t);
  uint16_t v = 0;
  if (TryParseHex4Bang(resp, v)) return (int32_t)v;
  _lastError = "DfReadFileCounts: bad response.";
  return -1;
}

int32_t MmpClient::DfReadFileNumber(int32_t devId1to4, int32_t timeoutMs)
{
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand("DST:" + Hex2(devId1to4) + ":" + Hex2(5) + "!", t);
  uint16_t v = 0;
  if (TryParseHex4Bang(resp, v)) return (int32_t)v;
  _lastError = "DfReadFileNumber: bad response.";
  return -1;
}
