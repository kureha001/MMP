//=============================================================================
//  mmpRottenmeier（DFPlayer + PCA9685 + DigitalIO + I2C）
//----------------------------------------------------------------------------- 
// Ver 0.3.02 - 2025/06/12 By Takanari.Kureha
//   ・シリアル方式をUSB/GPIOに切り替え機能を追加
//=============================================================================
#include <Wire.h>
#include <PCA9685.h>
#include <DFRobotDFPlayerMini.h>

#define SERIAL_SELECT_PIN 2
Stream* serialPort;

const char* Version = "0301!";
int inp_cnt = 0;

#define SERVOMIN 150
#define SERVOMAX 600
#define PWM_COUNT 16

PCA9685 PWM[PWM_COUNT] = {
  PCA9685(0x40), PCA9685(0x41), PCA9685(0x42), PCA9685(0x43),
  PCA9685(0x44), PCA9685(0x45), PCA9685(0x46), PCA9685(0x47),
  PCA9685(0x48), PCA9685(0x49), PCA9685(0x4A), PCA9685(0x4B),
  PCA9685(0x4C), PCA9685(0x4D), PCA9685(0x4E), PCA9685(0x4F)
};
bool PWM_Connected[PWM_COUNT];

DFRobotDFPlayerMini myDFPlayer;
bool DFPlayer_Connected = false;

const int analogGPIOs[4] = {26, 27, 28, 29};
int anaSwitchCnt = 4;
int anaPlayerCnt = 1;
int anaValues[16][4];

void setup() {
  pinMode(SERIAL_SELECT_PIN, INPUT_PULLUP);
  delay(10);
  bool useUSB = digitalRead(SERIAL_SELECT_PIN);

  if (useUSB) {
    Serial.begin(115200);
    serialPort = &Serial;
  } else {
    Serial1.setTX(0);
    Serial1.setRX(1);
    Serial1.begin(115200);
    serialPort = &Serial1;
  }

  Wire.begin();
  for (int i = 0; i < PWM_COUNT; i++) {
    PWM[i].begin();
    PWM[i].setPWMFreq(60);
    PWM_Connected[i] = true;
  }

  Serial2.begin(9600);
  if (myDFPlayer.begin(Serial2)) {
    myDFPlayer.volume(20);
    DFPlayer_Connected = true;
  }

  for (int i = 0; i < 16; i++)
    for (int j = 0; j < 4; j++)
      anaValues[i][j] = 0;

  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
}

int atoi16(const char *NumberString) {
  char *stopString;
  return strtol(NumberString, &stopString, 16);
}

void loop() {
  static char input[30] = {0};
  char dat[5][10], *div_dat;
  int dat_cnt;
  char rets[30];
  int pwmNo, pwmVal, pwmID, pwmCh;

  if (serialPort->available()) {
    char rcv = (char)serialPort->read();
    if (inp_cnt < sizeof(input) - 1) {
      input[inp_cnt++] = rcv;
    }

    if (rcv == '!') {
      input[inp_cnt - 1] = '\0';
      dat_cnt = 0;
      for (div_dat = strtok(input, ":"); div_dat; div_dat = strtok(NULL, ":")) {
        strncpy(dat[dat_cnt], div_dat, sizeof(dat[dat_cnt]) - 1);
        dat[dat_cnt][sizeof(dat[dat_cnt]) - 1] = '\0';
        dat_cnt++;
      }
      inp_cnt = 0;

      // アナログ入力関連
      if (strcmp(dat[0], "ANS") == 0 && dat_cnt >= 3) {
        int newPl = atoi16(dat[1]);
        int newSw = atoi16(dat[2]);
        if (newSw >= 1 && newSw <= 4 && newPl >= 1 && newPl <= 16) {
          anaSwitchCnt = newSw;
          anaPlayerCnt = newPl;
          for (int i = 0; i < 16; i++)
            for (int j = 0; j < 4; j++)
              anaValues[i][j] = 0;
          serialPort->print("!!!!!");
        } else serialPort->print("MUX!!");

      } else if (strcmp(dat[0], "ANU") == 0) {

        for (int ch = 0; ch < anaPlayerCnt; ch++) {

          digitalWrite(10, (ch >> 0) & 1);
          digitalWrite(11, (ch >> 1) & 1);
          digitalWrite(12, (ch >> 2) & 1);
          digitalWrite(13, (ch >> 3) & 1);

          delayMicroseconds(10);

          for (int mux = 0; mux < anaSwitchCnt; mux++) {
            int pin = analogGPIOs[mux];
            anaValues[ch][mux] = analogRead(pin);
          }
        }
        serialPort->print("!!!!!");

      } else if (strcmp(dat[0], "ANR") == 0 && dat_cnt >= 2) {

        int pl = atoi16(dat[1]);
        int sw = atoi16(dat[2]);

        if (pl < 16 && sw < 4) {
          sprintf(rets, "%04X!", anaValues[pl][sw]);
          serialPort->print(rets);
        } else serialPort->print("----!");

      // PWM関連
      } else if (strcmp(dat[0], "PWM") == 0 && dat_cnt >= 3) {
        pwmNo = atoi16(dat[1]);
        pwmVal = atoi16(dat[2]);
        pwmID = pwmNo / 16;
        pwmCh = pwmNo % 16;
        if (pwmID < PWM_COUNT && PWM_Connected[pwmID]) {
          PWM[pwmID].setPWM(pwmCh, 0, pwmVal);
          serialPort->print("!!!!!");
        } else serialPort->print("PWS!!");


      /*===========================================================
        I2C関連
      ============================================================*/
      //----------------------------------------------------------
      // 出力コマンド
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "I2W") == 0 && dat_cnt >= 4) {
        int addr = atoi16(dat[1]);
        int reg = atoi16(dat[2]);
        int val = atoi16(dat[3]);
        Wire.beginTransmission(addr);
        Wire.write(reg);
        Wire.write(val);
        Wire.endTransmission();
        serialPort->print("!!!!!");
      //----------------------------------------------------------
      // 入力コマンド
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "I2R") == 0 && dat_cnt >= 3) {
        int addr = atoi16(dat[1]);
        int reg = atoi16(dat[2]);
        Wire.beginTransmission(addr);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(addr, 1);
        int val = Wire.available() ? Wire.read() : 0xFF;
        sprintf(rets, "%04X!", val);
        serialPort->print(rets);

      /*===========================================================
        デジタルIO関連
      ============================================================*/
      //----------------------------------------------------------
      // 出力コマンド：ポート番号：出力値
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "POW") == 0 && dat_cnt >= 3) {
        int port = atoi16(dat[1]);
        int val = atoi16(dat[2]);
        pinMode(port, OUTPUT);
        digitalWrite(port, val);
        serialPort->print("!!!!!");
      //----------------------------------------------------------
      // 入力コマンド：ポート番号
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "POR") == 0 && dat_cnt >= 2) {
        int port = atoi16(dat[1]);
        pinMode(port, INPUT);
        int val = digitalRead(port);
        sprintf(rets, "%04X!", val);
        serialPort->print(rets);
      //----------------------------------------------------------
      // 入出力コマンド：ポート番号：出力値　※出力後に入力する
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "IO") == 0 && dat_cnt >= 3) {
        int port = atoi16(dat[1]);
        int val = atoi16(dat[2]);
        pinMode(port, OUTPUT);
        digitalWrite(port, val);
        delayMicroseconds(10);
        pinMode(port, INPUT);
        int result = digitalRead(port);
        sprintf(rets, "%04X!", result);
        serialPort->print(rets);

      /*===========================================================
        DFPlayer関連
      ============================================================*/
      //----------------------------------------------------------
      // 再生：指定トラック
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DPL") == 0 && dat_cnt >= 2) {
        if (DFPlayer_Connected) myDFPlayer.play(atoi16(dat[1]));
        serialPort->print("!!!!!");

      //----------------------------------------------------------
      // 再生：指定トラック（ループ）
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DRP") == 0 && dat_cnt >= 2) {
        if (DFPlayer_Connected) myDFPlayer.loop(atoi16(dat[1]));
        serialPort->print("!!!!!");

      //----------------------------------------------------------
      // 再生：指定フォルダ内トラック
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DIR") == 0 && dat_cnt >= 3) {
        if (DFPlayer_Connected) myDFPlayer.playFolder(atoi16(dat[1]), atoi16(dat[2]));
        serialPort->print("!!!!!");

      //----------------------------------------------------------
      // 停止
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DSP") == 0) {
        if (DFPlayer_Connected) myDFPlayer.stop();
        serialPort->print("!!!!!");

      //----------------------------------------------------------
      // 一時停止
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DPA") == 0) {
        if (DFPlayer_Connected) myDFPlayer.pause();
        serialPort->print("!!!!!");

      //----------------------------------------------------------
      // 再生再開
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DPR") == 0) {
        if (DFPlayer_Connected) myDFPlayer.start();
        serialPort->print("!!!!!");

      //----------------------------------------------------------
      // 再生状態：0停止, 1再生, 2一時停止
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DST") == 0) {
        if (DFPlayer_Connected) {
          int state = myDFPlayer.readState();
          sprintf(rets, "%04X!", state);
          serialPort->print(rets);
        } else serialPort->print("----!");

      //----------------------------------------------------------
      // 再生中トラック番号取得（戻り値は16進数）
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DQT") == 0) {
        if (DFPlayer_Connected) {
          int val = myDFPlayer.readCurrentFileNumber();
          sprintf(rets, "%04X!", val);
          serialPort->print(rets);
        } else serialPort->print("----!");

      //----------------------------------------------------------
      // 全トラック数取得
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DTC") == 0) {
        if (DFPlayer_Connected) {
          int count = myDFPlayer.readFileCounts();
          sprintf(rets, "%04X!", count);
          serialPort->print(rets);
        } else serialPort->print("----!");

      //----------------------------------------------------------
      // イコライザー設定：0〜5
      // 0: Normal, 1: Pop, 2: Rock, 3: Jazz, 4: Classic, 5: Bass
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DEF") == 0 && dat_cnt >= 2) {
        if (DFPlayer_Connected) myDFPlayer.EQ(atoi16(dat[1]));
        serialPort->print("!!!!!");

      //----------------------------------------------------------
      // 音量設定（0〜30）
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "VOL") == 0) {
        if (DFPlayer_Connected) myDFPlayer.volume(atoi16(dat[1]));
        serialPort->print("!!!!!");

      /*===========================================================
        バージョン情報
      ============================================================*/
      } else if (strcmp(dat[0], "VER") == 0) {
        serialPort->print(Version);

      /*===========================================================
        コマンド不明（エラー）
      ============================================================*/
      } else {
        char resp[6] = "!!!!!";
        strncpy(resp, dat[0], 3);
        serialPort->print(resp);
      }
    }
  }
}
