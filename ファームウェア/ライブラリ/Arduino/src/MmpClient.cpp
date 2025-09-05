#include "MmpClient.h"
using namespace Mmp::Core;

// ====================
// ===== 低レベル =====
// ====================
bool MmpClient::Open(uint32_t baud, int32_t /*timeoutIo*/) {

  // ★ ピン設定：コアにより方法が異なるため条件分岐
  #if defined(MMP_UART_USE_SETRXTX)
    // コアが setTX/setRX をサポートする場合のみ有効化
    _uart->setTX(MMP_UART_TX_PIN);   // ★
    _uart->setRX(MMP_UART_RX_PIN);   // ★

  #elif defined(MMP_UART_USE_PICO_GPIOFUNC)
    // pico-sdk のピン多重化で UART 機能を割り当て（非 Mbed コア前提）
    gpio_set_function(MMP_UART_TX_PIN, GPIO_FUNC_UART); // ★
    gpio_set_function(MMP_UART_RX_PIN, GPIO_FUNC_UART); // ★
    // ※ UART0/1 の紐付けはピン番号に依存。GPIO0/1 は UART0 に割付け。
    //   このボードの Serial1 が UART0 である前提（多くの RP2040/2350 非Mbedコアが該当）

  #else
    // ★ 何もしない：ボード定義のデフォルト UART ピンを使用
    //   （WaveShare RP2350 Zero では Serial1 が GPIO0/1 に割当済みのケースが多い）

  #endif

  _uart->begin(baud);   // ★ setRX/setTX を使わず初期化
  delay(10);
  ClearInput();
  _isOpen = true;
  return true;
}

bool MmpClient::Verify(int32_t timeoutVerify) {
  const int32_t t = Resolve(timeoutVerify, Settings.TimeoutVerify);
  String resp = SendCommand("VER!", t);
  if (resp.length() == 0) return false;
  return IsFiveBang(resp);
}

void MmpClient::Close() {
  if (_isOpen) { _uart->flush(); _uart->end(); }
  _isOpen = false;
}

// ================
// ===== 接続 =====
// ================
bool MmpClient::ConnectWithBaud(uint32_t baud, int32_t timeoutIo, int32_t verifyTimeoutMs) {
  (void)Resolve(timeoutIo, Settings.TimeoutIo);
  if (!Open(baud, timeoutIo)) { _lastError = F("Open failed"); return false; }
  if (!Verify(verifyTimeoutMs)) { _lastError = F("Verify failed"); Close(); return false; }
  return true;
}

bool MmpClient::ConnectAutoBaud(const uint32_t* cand, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (ConnectWithBaud(cand[i], 0, 0)) { Settings.BaudRate = (int32_t)cand[i]; return true; }
  }
  _lastError = F("No baud matched");
  return false;
}

// ==================
// ===== 送受信 =====
// ==================
String MmpClient::SendCommand(const String& command, int32_t timeoutMs) {
  if (!_isOpen) { _lastError = F("Port is not open"); return String(); }
  if (command.length() == 0) { _lastError = F("Empty command"); return String(); }

  String cmd = command;
  if (cmd[cmd.length()-1] != '!') cmd += '!';

  ClearInput();
  _uart->print(cmd);
  _uart->flush();
  delayMicroseconds(300);                  // ★ 小ギャップで分割読みに強くする

  String resp;
  resp.reserve(5);                         // ★ 5文字固定のため事前確保
  const uint32_t t = (uint32_t)timeoutMs;
  const uint32_t start = millis();

  // ★ 修正：末尾 '!' を見たら即 return せず、「5文字そろうまで」読む
  while (resp.length() < 5 && (uint32_t)(millis() - start) < t) {   // ★
    if (_uart->available() > 0) {                                    // ★
      resp += (char)_uart->read();                                   // ★
    } else {                                                         // ★
      yield();                                                       // ★
    }                                                                // ★
  }                                                                  // ★

  // ★ 5文字かつ末尾 '!' を厳密チェック
  if (resp.length() == 5 && resp[4] == '!') {                        // ★
    return resp;                                                     // ★
  }                                                                  // ★

  _lastError = F("Timeout or invalid 5-char reply");                 // ★
  return String();
}

// ===================================
// ===== フラットAPI（内部専用） =====
// ===================================
// --------------
// ---- 情報 ----
// --------------
String MmpClient::GetVersion(int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutGeneral);
  return SendCommand("VER!", t);
}
uint16_t MmpClient::GetPwx(int32_t deviceId, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutPwm);
  String resp = SendCommand(String("PWX:") + Hex2(deviceId) + "!", t);
  uint16_t v = 0; if (!TryParseHex4Bang(resp, v)) { _lastError = F("PWX bad response"); } return v;
}
uint16_t MmpClient::GetDpx(int32_t id1to4, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("DPX:") + Hex2(id1to4) + "!", t);
  uint16_t v = 0; if (!TryParseHex4Bang(resp, v)) { _lastError = F("DPX bad response"); } return v;
}

// ------------------
// ---- アナログ ----
// ------------------
bool MmpClient::AnalogConfigure(int32_t players, int32_t switches, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAnalog);
  if (players  < 1 || players  > 16) { _lastError = F("players out of range"); return false; }
  if (switches < 1 || switches >  4) { _lastError = F("switches out of range"); return false; }
  String resp = SendCommand(String("ANS:") + Hex2(players) + ":" + Hex2(switches) + "!", t);
  return (resp == "!!!!!");
}
bool MmpClient::AnalogUpdate(int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAnalog);
  String resp = SendCommand("ANU!", t);
  return (resp == "!!!!!");
}
int32_t MmpClient::AnalogRead(int32_t playerIndex0to15, int32_t switchIndex0to3, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAnalog);
  if (playerIndex0to15 < 0 || playerIndex0to15 > 15) { _lastError = F("playerIndex out of range"); return -1; }
  if (switchIndex0to3  < 0 || switchIndex0to3  >  3) { _lastError = F("switchIndex out of range"); return -1; }
  String resp = SendCommand(String("ANR:") + Hex2(playerIndex0to15) + ":" + Hex2(switchIndex0to3) + "!", t);
  uint16_t v = 0; if (TryParseHex4Bang(resp, v)) return (int32_t)v; return -1;
}

// -------------
// ---- PWM ----
// -------------
String MmpClient::PwmValue(int32_t channel, int32_t value0to4095, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutPwm);
  return SendCommand(String("PWM:") + Hex2(channel) + ":" + Hex4(value0to4095) + "!", t);
}
bool MmpClient::PwmInit(int32_t angleMin, int32_t angleMax, int32_t pwmMin, int32_t pwmMax, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutPwm);
  String resp = SendCommand(String("PWI:") + Hex3(angleMin) + ":" + Hex3(angleMax) + ":" + Hex4(pwmMin) + ":" + Hex4(pwmMax) + "!", t);
  return (resp == "!!!!!");
}
String MmpClient::PwmAngle(int32_t channel, int32_t angle0to180, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutPwm);
  return SendCommand(String("PWA:") + Hex2(channel) + ":" + Hex3(angle0to180) + "!", t);
}

// ------------------
// ---- デジタル ----
// ------------------
int32_t MmpClient::DigitalIn(int32_t portId, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutDigital);
  String resp = SendCommand(String("POR:") + Hex2(portId) + "!", t);
  uint16_t v = 0; if (TryParseHex4Bang(resp, v)) return (int32_t)v; return 0;
}
bool MmpClient::DigitalOut(int32_t portId, int32_t value0or1, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutDigital);
  String resp = SendCommand(String("POW:") + Hex2(portId) + ":" + ((value0or1 & 1) ? "1" : "0") + "!", t);
  return (resp == "!!!!!");
}

// -------------
// ---- I2C ----
// -------------
bool MmpClient::I2cWrite(int32_t addr, int32_t reg, int32_t value, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutI2c);
  String resp = SendCommand(String("I2W:") + Hex2(addr) + ":" + Hex2(reg) + ":" + Hex2(value) + "!", t);
  return (resp == "!!!!!");
}
int32_t MmpClient::I2cRead(int32_t addr, int32_t reg, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutI2c);
  String resp = SendCommand(String("I2R:") + Hex2(addr) + ":" + Hex2(reg) + "!", t);
  uint16_t v = 0; if (TryParseHex4Bang(resp, v)) return (int32_t)v; return 0;
}

// ------------------------
// ---- MP3（DFPlayer）----
// ------------------------
bool MmpClient::DfPlayFolderTrack(int32_t deviceId1to4, int32_t folder1to255, int32_t track1to255, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("DIR:") + Hex2(deviceId1to4) + ":" + Hex2(folder1to255) + ":" + Hex2(track1to255) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::DfSetLoop(int32_t deviceId1to4, int32_t on0or1, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("DLP:") + Hex2(deviceId1to4) + ":" + Hex2(on0or1) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::DfStop(int32_t deviceId1to4, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("DSP:") + Hex2(deviceId1to4) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::DfPause(int32_t deviceId1to4, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("DPA:") + Hex2(deviceId1to4) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::DfResume(int32_t deviceId1to4, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("DPR:") + Hex2(deviceId1to4) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::DfVolume(int32_t deviceId1to4, int32_t vol0to30, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("VOL:") + Hex2(deviceId1to4) + ":" + Hex2(vol0to30) + "!", t);
  return (resp == "!!!!!");
}

bool MmpClient::DfSetEq(int32_t deviceId1to4, int32_t eqMode0to5, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("DEF:") + Hex2(deviceId1to4) + ":" + Hex2(eqMode0to5) + "!", t);
  return (resp == "!!!!!");
}

int32_t MmpClient::DfReadPlayState(int32_t deviceId1to4, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("DST:") + Hex2(deviceId1to4) + ":01!", t);
  uint16_t v = 0; if (TryParseHex4Bang(resp, v)) return (int32_t)v; return -1;
}

int32_t MmpClient::DfReadVolume(int32_t deviceId1to4, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("DST:") + Hex2(deviceId1to4) + ":02!", t);
  uint16_t v = 0; if (TryParseHex4Bang(resp, v)) return (int32_t)v; return -1;
}

int32_t MmpClient::DfReadEq(int32_t deviceId1to4, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("DST:") + Hex2(deviceId1to4) + ":03!", t);
  uint16_t v = 0; if (TryParseHex4Bang(resp, v)) return (int32_t)v; return -1;
}

int32_t MmpClient::DfReadFileCounts(int32_t deviceId1to4, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("DST:") + Hex2(deviceId1to4) + ":04!", t);
  uint16_t v = 0; if (TryParseHex4Bang(resp, v)) return (int32_t)v; return -1;
}

int32_t MmpClient::DfReadCurrentFileNumber(int32_t deviceId1to4, int32_t timeoutMs) {
  const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
  String resp = SendCommand(String("DST:") + Hex2(deviceId1to4) + ":05!", t);
  uint16_t v = 0; if (TryParseHex4Bang(resp, v)) return (int32_t)v; return -1;
}
