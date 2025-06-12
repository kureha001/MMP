//=============================================================================
//  mmpRottenmeier（DFPlayer + PCA9685）
//----------------------------------------------------------------------------- 
// Ver 0.03.001　2025/06/13 By Takanari.Kureha
//   ・Earle Philhowerボード対応：RP2040-Zero用ピン配置
//   ・Generic RP2350 ボード対応：RP2350-Zero用ピン配置
//   ・DFPlayer機能追加
//=============================================================================
#include <Wire.h>
#include <PCA9685.h>
#include <DFRobotDFPlayerMini.h>

const char* Version = "9999!";
int inp_cnt = 0;

#define SERVOMIN 150
#define SERVOMAX 600
const int PWM_COUNT = 48;
PCA9685 PWM[PWM_COUNT] = {
  PCA9685(0x40), PCA9685(0x41), PCA9685(0x42), PCA9685(0x43), PCA9685(0x44),
  PCA9685(0x45), PCA9685(0x46), PCA9685(0x47), PCA9685(0x48), PCA9685(0x49),
  PCA9685(0x4A), PCA9685(0x4B), PCA9685(0x4C), PCA9685(0x4D), PCA9685(0x4E),
  PCA9685(0x4F), PCA9685(0x50), PCA9685(0x51), PCA9685(0x52), PCA9685(0x53), 
  PCA9685(0x54), PCA9685(0x55), PCA9685(0x56), PCA9685(0x57), PCA9685(0x58),
  PCA9685(0x59), PCA9685(0x5A), PCA9685(0x5B), PCA9685(0x5C), PCA9685(0x5D),
  PCA9685(0x5E), PCA9685(0x5F), PCA9685(0x60), PCA9685(0x61), PCA9685(0x62),
  PCA9685(0x63), PCA9685(0x64), PCA9685(0x65), PCA9685(0x66), PCA9685(0x67),
  PCA9685(0x68), PCA9685(0x69), PCA9685(0x6A), PCA9685(0x6B), PCA9685(0x6C),
  PCA9685(0x6D), PCA9685(0x6E)
};

DFRobotDFPlayerMini myDFPlayer;

int analogIndexToGPIO(int idx) {
  if (idx >= 0 && idx <= 3) return 26 + idx;
  return -1;
}

void setup() {
  Wire.begin();
  Serial.begin(115200);

  for (int i = 0; i < PWM_COUNT; i++) {
    PWM[i].begin();
    PWM[i].setPWMFreq(60);
  }

  for (int pin = 10; pin <= 13; pin++) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  Serial2.begin(9600);

  if (!myDFPlayer.begin(Serial2)) {
    while (1);
  }
  myDFPlayer.volume(20);
}

void loop() {
  static char input[30] = {0};
  char dat[5][10], *div_dat;
  int dat_cnt;
  char rets[30];
  int pwmNo, pwmVal, pwmAn, pwmID, pwmCh;

  if (Serial.available()) {
    char rcv = (char)Serial.read();
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

      if (strcmp(dat[0], "PWM") == 0) {
        pwmNo = atoi16(dat[1]);
        pwmVal = atoi16(dat[2]);
        pwmID = pwmNo / 16;
        pwmCh = pwmNo % 16;
        if (pwmID < PWM_COUNT) {
          PWM[pwmID].setPWM(pwmCh, 0, pwmVal);
          Serial.print("!!!!!");
        } else Serial.print("----!");

      } else if (strcmp(dat[0], "PWA") == 0) {
        pwmNo = atoi16(dat[1]);
        pwmAn = atoi16(dat[2]);
        pwmAn = map(pwmAn, 0, 180, SERVOMIN, SERVOMAX);
        pwmID = pwmNo / 16;
        pwmCh = pwmNo % 16;
        if (pwmID < PWM_COUNT) {
          PWM[pwmID].setPWM(pwmCh, 0, pwmAn);
          Serial.print("!!!!!");
        } else Serial.print("----!");

      } else if (strcmp(dat[0], "POW") == 0) {
        pinMode(atoi16(dat[1]), OUTPUT);
        digitalWrite(atoi16(dat[1]), atoi16(dat[2]));
        Serial.print("!!!!!");

      } else if (strcmp(dat[0], "POR") == 0) {
        pinMode(atoi16(dat[1]), INPUT);
        unsigned char retb = digitalRead(atoi16(dat[1]));
        sprintf(rets, "%01x!!!!", retb);
        Serial.print(rets);

      } else if (strcmp(dat[0], "ADR") == 0) {
        int analogIndex = atoi16(dat[1]);
        int gpio = analogIndexToGPIO(analogIndex);
        if (gpio >= 0) {
          word retw = analogRead(gpio);
          if (retw > 0x3FF) retw = 0x3FF;
          sprintf(rets, "%03x!!", retw);
          Serial.print(rets);
        } else {
          Serial.print("----!");
        }

      } else if (strcmp(dat[0], "DPL") == 0) {
        myDFPlayer.play(atoi16(dat[1]));
        Serial.print("!!!!!");

      } else if (strcmp(dat[0], "DSP") == 0) {
        myDFPlayer.stop();
        Serial.print("!!!!!");

      } else if (strcmp(dat[0], "DPA") == 0) {
        myDFPlayer.pause();
        Serial.print("!!!!!");

      } else if (strcmp(dat[0], "DPR") == 0) {
        myDFPlayer.start();
        Serial.print("!!!!!");

      } else if (strcmp(dat[0], "VOL") == 0) {
        myDFPlayer.volume(atoi16(dat[1]));
        Serial.print("!!!!!");

      } else if (strcmp(dat[0], "DQT") == 0) {
        uint16_t num = myDFPlayer.readCurrentFileNumber();
        sprintf(rets, "%03x!!", num);
        Serial.print(rets);

      } else if (strcmp(dat[0], "DST") == 0) {
        uint16_t state = myDFPlayer.readState();
        sprintf(rets, "%01x!!!!", state);
        Serial.print(rets);

      } else if (strcmp(dat[0], "DTC") == 0) {
        uint16_t count = myDFPlayer.readFileCounts();
        sprintf(rets, "%03x!!", count);
        Serial.print(rets);

      } else if (strcmp(dat[0], "DEF") == 0) {
        myDFPlayer.EQ(atoi16(dat[1]));
        Serial.print("!!!!!");

      } else if (strcmp(dat[0], "DIR") == 0) {
        myDFPlayer.playFolder(atoi16(dat[1]), atoi16(dat[2]));
        Serial.print("!!!!!");

      } else if (strcmp(dat[0], "DRP") == 0) {
        myDFPlayer.loop(atoi16(dat[1]));
        Serial.print("!!!!!");

      } else if (strcmp(dat[0], "VER") == 0) {
        Serial.print(Version);

      } else {
        char resp[6] = "!!!!!";
        strncpy(resp, dat[0], 5);
        for (int i = strlen(resp); i < 5; i++) resp[i] = '!';
        resp[5] = '\0';
        Serial.print(resp);
      }
    }
  }
}

int atoi16(const char *NumberString) {
  char *stopString;
  int result = strtol(NumberString, &stopString, 16);
  return result;
}
