//=============================================================================
//  mmpRottenmeier（HC4067/PCA9685/DigitalIO/I2C/DFPlayer）
//----------------------------------------------------------------------------- 
// ボード情報：Waveshae RP2040 Zero
// ボード情報：Waveshae RP2350 Zero
//----------------------------------------------------------------------------- 
// Ver 0.3.06 - 2025/08/27 By Takanari.Kureha
//   ・コメント誤りの修正
//   ・戻り値の一部(異常時)にある誤りを修正
//=============================================================================
#include <Wire.h>
#include <PCA9685.h>
#include <DFRobotDFPlayerMini.h>
const char* Version = "0304!";
int inp_cnt = 0;            // 受信データ・カウンタ

//----------------------------------------------------------
// RGB LED
//----------------------------------------------------------
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(1, 16, NEO_GRB + NEO_KHZ800);

//----------------------------------------------------------
// シリアル通信
//----------------------------------------------------------
Stream* serialPort;         // インスタンス

// 切り替えスイッチのGPIO
#define SERIAL_SELECT_PIN 2 // スイッチ１：通信方式切替
#define BAUD_SELECT_PIN1 14 // スイッチ２：ボーレート選択
#define BAUD_SELECT_PIN2 15 // スイッチ３：ボーレート選択

// ボーレートのプリセット
#define BAUD_00 115200      // 赤：OFF｜OFF
#define BAUD_10 9600        // 緑：ON ｜OFF
#define BAUD_01 230400      // 青：OFF｜ON
#define BAUD_11 921600      // 紫：ON ｜ON

//----------------------------------------------------------
// ＰＷＭ機器(PCA9685)
//----------------------------------------------------------
#define PWM_COUNT 16            // 扱えるPCA9685の機器の最大個数
bool PWM_Connected[PWM_COUNT];  // 機器ごとの接続済みフラグ(True:接続済み／False:未接続)

// 対応するPCA9685のアドレス(この順番が機器番号)
PCA9685 PWM[PWM_COUNT] = {
  PCA9685(0x40), PCA9685(0x41), PCA9685(0x42), PCA9685(0x43),
  PCA9685(0x44), PCA9685(0x45), PCA9685(0x46), PCA9685(0x47),
  PCA9685(0x48), PCA9685(0x49), PCA9685(0x4A), PCA9685(0x4B),
  PCA9685(0x4C), PCA9685(0x4D), PCA9685(0x4E), PCA9685(0x4F)
};

// サーボモータの定格（デフォルト値）
int servoRS = 0;   // 有効開始角度
int servoRE = 180; // 有効終了角度
int servoPS = 150; // 有効開始角度のPWM出力値
int servoPE = 600; // 有効開始角度のPWM出力値

//----------------------------------------------------------
// mp3機器(DFPlayer)
//----------------------------------------------------------
DFRobotDFPlayerMini dfPlayer[2];        // インスタンス
bool dfConnected[2] = { false, false }; // 接続済みフラグ

//----------------------------------------------------------
// アナログ入力機器(HC4067)
//----------------------------------------------------------
const int addressBusGPIOs[4] = {10, 11, 12, 13};  // アドレスバス用GPIO
const int analogGPIOs[4] = {26, 27, 28, 29};      //データバス用GPIO
int anaSwitchCnt = 4; // デフォルト値(マルチプレクサ数)
int anaPlayerCnt = 1; // デフォルト値(プレイヤー数=チャンネル数)
int anaValues[16][4]; // 取得値バッファ

//===========================================================
// 初期化処理
//===========================================================
void setup() {

  // スイッチ２・３によるボーレート選択
  pinMode(BAUD_SELECT_PIN1, INPUT_PULLUP);
  pinMode(BAUD_SELECT_PIN2, INPUT_PULLUP);
  int br1 = digitalRead(BAUD_SELECT_PIN1);
  int br2 = digitalRead(BAUD_SELECT_PIN2);
  int selectedBaud = BAUD_00;
  if      (!br1 && !br2) selectedBaud = BAUD_00; // OFF,OFF
  else if ( br1 && !br2) selectedBaud = BAUD_10; // ON,OFF
  else if (!br1 &&  br2) selectedBaud = BAUD_01; // OFF,ON
  else if ( br1 &&  br2) selectedBaud = BAUD_11; // ON,ON

  // スイッチ１によるシリアル通信方式の選択
  pinMode(SERIAL_SELECT_PIN, INPUT_PULLUP);
  delay(100);
  bool useUSB = digitalRead(SERIAL_SELECT_PIN);
  if (useUSB) {
    Serial.begin(selectedBaud);
    serialPort = &Serial;
  } else {
    //Serial1.setTx(0); //STM32
    //Serial1.setRx(1); //STM32
    Serial1.setTX(0); //RP2350,RP2040
    Serial1.setRX(1); //RP2350,RP2040
    Serial1.begin(selectedBaud);
    serialPort = &Serial1;
  }

  // PCA9685 16台を初期化、接続済フラグをONにする
  Wire.begin();
  delay(100);
  for (int i = 0; i < PWM_COUNT; i++) {
    PWM[i].begin();
    PWM[i].setPWMFreq(60);
    PWM_Connected[i] = true;
  }

  // DFPlayer(1台目)：Serial2に(規定値：GPIO8,9)で接続
  Serial2.begin(9600);      // 規定ボーレートで接続
  delay(100);
  if (dfPlayer[0].begin(Serial2)) {
    dfPlayer[0].volume(20); // 音量を20にセット
    dfConnected[0] = true;  // 接続済フラグをONにする
  }

  // DFPlayer(2台目)：Serial1に(規定値：GPIO0,1)で接続
  if (useUSB) {
    Serial1.begin(9600);      // 規定ボーレートで接続
    delay(100);
    if (dfPlayer[1].begin(Serial1)) {
      dfPlayer[1].volume(20); // 音量を20にセット      
      dfConnected[1] = true;  // 接続済フラグをONにする
    }
  }

  // アナログ入力値バッファをクリア
  for (int i = 0; i < 16; i++)
    for (int j = 0; j < 4; j++)
      anaValues[i][j] = 0;

  // マルチプレクサのアドレスバスのGPIOを設定
  for (int i = 0; i < 4; i++) {
    pinMode(addressBusGPIOs[i], OUTPUT);
  }

  // RGB LED
  pixels.begin();
  pixels.clear();
  if (useUSB) {
    // USBシリアルは白色を点灯
    pixels.setPixelColor(0, pixels.Color(10,10,10));
  } else {
    if      (selectedBaud == BAUD_00) { pixels.setPixelColor(0, pixels.Color(10, 0, 0)); } // 赤
    else if (selectedBaud == BAUD_10) { pixels.setPixelColor(0, pixels.Color( 0,10, 0)); } // 緑
    else if (selectedBaud == BAUD_01) { pixels.setPixelColor(0, pixels.Color( 0, 0,10)); } // 青
    else if (selectedBaud == BAUD_11) { pixels.setPixelColor(0, pixels.Color(10,10, 0)); } // 紫
  }
  pixels.show();
}

//----------------------------------------------------------
// 10進数→16進数変換
//----------------------------------------------------------
int atoi16(const char *NumberString) {
  char *stopString;
  return strtol(NumberString, &stopString, 16);
}

//----------------------------------------------------------
// DFPlayer関連コマンドのバリデーション
//----------------------------------------------------------
bool validateDFPlayer(int index, const char* cmdName) {
  if (index >= 0 && index < 2 && dfConnected[index]) {
    return true;
  } else {
    char err[6];
    strncpy(err, cmdName, 3);
    err[3] = '!';
    err[4] = '!';
    err[5] = '\0';
    serialPort->print(err);
    return false;
  }
}

//===========================================================
// メイン処理
//===========================================================
void loop() {

  // 送受信関連
  static char input[30] = {0};  // 受信データ・バッファ
  char dat[5][10], *div_dat;    // [0]:コマンド，[1～5]:引数
  int dat_cnt;                  // コマンド＋引数の個数
  char rets[30];                // リターン値バッファ

  // ＰＣＡ９６８５関連
  int pwmNo;    // 通しチャンネル番号
  int pwmVal;   // ＰＷＭ出力値
  int pwmID;    // PCA9685番号
  int pwmCh;    // PCA9685番号別のチャンネル番号
  int pwmRDiff; // 角度差
  int pwmPDiff; // PWM値差

  //----------------------------------------------------------
  // 受信したら、１バイトずつ受信データ・バッファに追加格納
  //----------------------------------------------------------
  if (serialPort->available()) {
    char rcv = (char)serialPort->read();
    if (inp_cnt < sizeof(input) - 1) {
      input[inp_cnt++] = rcv;
    }

    //----------------------------------------------------------
    // 終端を検出
    //----------------------------------------------------------
    if (rcv == '!') {

      //----------------------------------------------------------
      // コマンドと引数に分割し、個数を取得
      //----------------------------------------------------------
      input[inp_cnt - 1] = '\0';
      dat_cnt = 0;

      for (div_dat = strtok(input, ":"); div_dat; div_dat = strtok(NULL, ":")) {
        strncpy(dat[dat_cnt], div_dat, sizeof(dat[dat_cnt]) - 1);
        dat[dat_cnt][sizeof(dat[dat_cnt]) - 1] = '\0';
        dat_cnt++;
      }

      inp_cnt = 0;

      //***********************************************************
      // コマンド・パーサ
      //***********************************************************
      //===========================================================
      // アナログ関連(HC4067)
      //===========================================================
      //----------------------------------------------------------
      // 機能　　　　：機器設定（ANS）
      // コマンド形式：ANS:<プレイヤー数1～16>:<スイッチ数1～4>!
      // 返却値　　　："!!!!!" 正常 ／ "ANS!!" エラー
      //----------------------------------------------------------
      if (strcmp(dat[0], "ANS") == 0 && dat_cnt >= 3) {

        pixels.setPixelColor(0, pixels.Color(10,0,10));
        pixels.show();

        int newPl = atoi16(dat[1]);
        int newSw = atoi16(dat[2]);
        if (newPl >= 1 && newPl <= 16 && newSw >= 1 && newSw <= 4) {
          anaSwitchCnt = newSw;
          anaPlayerCnt = newPl;
          for (int i = 0; i < 16; i++)
            for (int j = 0; j < 4; j++)
              anaValues[i][j] = 0;
          serialPort->print("!!!!!");
        } else serialPort->print("ANS!!");

        pixels.clear();
        pixels.show();

      //----------------------------------------------------------
      // 機能　　　　：値バッファ更新（ANU）
      // コマンド形式：ANU!
      // 返却値　　　："!!!!!" 正常
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "ANU") == 0) {

        pixels.setPixelColor(0, pixels.Color(10,0,10));
        pixels.show();

        for (int ch = 0; ch < anaPlayerCnt; ch++) {
          for (int i = 0; i < 4; i++) {
            digitalWrite(addressBusGPIOs[i], (ch >> i) & 1);
          }
          delayMicroseconds(10);
          for (int sw = 0; sw < anaSwitchCnt; sw++) {
            int pin = analogGPIOs[sw];
            anaValues[ch][sw] = analogRead(pin);
          }
        }
        serialPort->print("!!!!!");

        pixels.clear();
        pixels.show();

      //----------------------------------------------------------
      // 機能　　　　：値バッファ参照（ANR）
      // コマンド形式：ANR:<プレイヤー番号0～15>:<スイッチ番号0～3>!
      // 返却値　　　：アナログ値 ／ "ANR!!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "ANR") == 0 && dat_cnt >= 2) {

        pixels.setPixelColor(0, pixels.Color(10,0,10));
        pixels.show();

        int pl = atoi16(dat[1]);
        int sw = atoi16(dat[2]);
        if (pl < 16 && sw < 4) {
          sprintf(rets, "%04X!", anaValues[pl][sw]);
          serialPort->print(rets);
        } else serialPort->print("ANR!!");

        pixels.clear();
        pixels.show();

      //===========================================================
      // ＰＷＭ関連(PCA9685)
      //===========================================================
      //----------------------------------------------------------
      // 出力コマンド
      // 機能　　　　：ＰＷＭ出力（PWM）
      // コマンド形式：PWM:<Ch通番><ＰＷＭ値0～4095>!
      // 返却値　　　："!!!!!" 正常 ／ "PWM!!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "PWM") == 0 && dat_cnt >= 3) {

        pixels.setPixelColor(0, pixels.Color(0,0,50));
        pixels.show();

        pwmNo   = atoi16(dat[1]);
        pwmVal  = atoi16(dat[2]);
        pwmID   = pwmNo / 16;
        pwmCh   = pwmNo % 16;
        if (pwmID < PWM_COUNT && PWM_Connected[pwmID]) {
          PWM[pwmID].setPWM(pwmCh, 0, pwmVal);
          serialPort->print("!!!!!");
        } else serialPort->print("PWM!!");

        pixels.clear();
        pixels.show();

      //----------------------------------------------------------
      // 出力コマンド
      // 機能　　　　：ＰＷＭ出力 角度指定（PWA）
      // コマンド形式：PWM:<Ch通番>:<角度0～180>!
      // 返却値　　　："!!!!!" 正常 ／ "PWA!!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "PWA") == 0 && dat_cnt >= 3) {

        pixels.setPixelColor(0, pixels.Color(0,0,50));
        pixels.show();

        pwmNo   = atoi16(dat[1]);
        pwmID   = pwmNo / 16;
        pwmCh   = pwmNo % 16;
        if (pwmID < PWM_COUNT && PWM_Connected[pwmID]) {
          int pwmAngle = atoi16(dat[2]);  // 指定角度
          pwmRDiff = servoRE - servoRS;   // 角度差(開始～終了)
          pwmPDiff = servoPE - servoPS;   // PWM値差(開始～終了)
          // PWM値 ＝ 開始PWM値 ＋ 角度率 * PWM値差
          pwmVal   = servoPS + int( (pwmAngle - servoRS) / pwmRDiff * pwmPDiff );
          PWM[pwmID].setPWM(pwmCh, 0, pwmVal);
          serialPort->print("!!!!!");
        } else serialPort->print("PWA!!");

        pixels.clear();
        pixels.show();

      //----------------------------------------------------------
      // 出力コマンド
      // 機能　　　　：ＰＷＭ出力 サーボ定格設定（PWI）
      // コマンド形式：PWM:<開始角0~360>:<終了角0～360>:<開始PWM0～4095>:<終了PWM0～4095>!
      // 返却値　　　："!!!!!" 正常／ "PWI!!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "PWI") == 0 && dat_cnt >= 5) {

        pixels.setPixelColor(0, pixels.Color(0,0,50));
        pixels.show();

        servoRS = atoi16(dat[1]); // サーボの有効開始角度
        servoRE = atoi16(dat[2]); // サーボの有効終了角度
        servoPS = atoi16(dat[3]); // サーボの有効開始角度でのPWM値
        servoPE = atoi16(dat[4]); // サーボの有効終了角度でのPWM値
        // 開始・終了の大小チェック
        if (servoRS < servoRE && servoPS < servoPE) {
          serialPort->print("!!!!!");
        } else serialPort->print("PWI!!");

        pixels.clear();
        pixels.show();

      //===========================================================
      // ｉ２ｃ関連
      //===========================================================
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

      //============================================================
      // デジタル関連
      //============================================================
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

      //===========================================================
      // ｍｐ３関連(DFPlayer)
      //  <コマンド>:<機器番号1または2>:<その他引数>!
      //===========================================================
      //----------------------------------------------------------
      // 機能　　　　：指定フォルダ内トラック再生
      // コマンド形式：DIR:<機器番号0～1>!
      // 返却値　　　："!!!!!" 正常 ／ "DIR!!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DIR") == 0 && dat_cnt >= 4) {
        int n = atoi16(dat[1]) - 1;
        if (validateDFPlayer(n, dat[0])) {
          dfPlayer[n].playFolder(atoi16(dat[2]), atoi16(dat[3]));
          serialPort->print("!!!!!");
        }

        pixels.clear();
        pixels.show();

      //----------------------------------------------------------
      // 機能　　　　：状態取得
      // コマンド形式：DST:<機器番号0～1>!
      // 返却値　　　："<状態番号>!" 正常 ／ "DST!!" エラー
      //              状態番号(0停止, 1再生, 2一時停止)
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DST") == 0 && dat_cnt >= 3) {

        pixels.setPixelColor(0, pixels.Color(0,10,0));
        pixels.show();

        int n = atoi16(dat[1]) - 1;
        int s = atoi16(dat[2]);
        int state = 0;

        if (validateDFPlayer(n, dat[0])) {
          switch (s) {
            case 1:
              state = dfPlayer[n].readState();
              if (state<=0) {state=0;}
              break;
            case 2:
              state = dfPlayer[n].readVolume();
              break;
            case 3:
              state = dfPlayer[n].readEQ();
              break;
            case 4:
              state = dfPlayer[n].readFileCounts();
              break;
            case 5:
              state = dfPlayer[n].readCurrentFileNumber();
              break;
            default:
              break;
          }
          if (state<0) {
            serialPort->print("!!!!!");
          } else {
            sprintf(rets, "%04X!", state);
            serialPort->print(rets);
          }
        } else {
          serialPort->print("!!!!!");
        }

        pixels.clear();
        pixels.show();

      //----------------------------------------------------------
      // 機能　　　　：停止
      // コマンド形式：DSP:<機器番号0～1>!
      // 返却値　　　："!!!!!" 正常 ／ "DSP!!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DSP") == 0 && dat_cnt >= 2) {

        pixels.setPixelColor(0, pixels.Color(0,250,0));
        pixels.show();

        int n = atoi16(dat[1]) - 1;
        if (validateDFPlayer(n, dat[0])) {
          dfPlayer[n].stop();
          serialPort->print("!!!!!");
        }

        pixels.clear();
        pixels.show();

      //----------------------------------------------------------
      // 機能　　　　：一時停止
      // コマンド形式：DPA:<機器番号0～1>!
      // 返却値　　　："!!!!!" 正常 ／ "DPA!!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DPA") == 0 && dat_cnt >= 2) {

        pixels.setPixelColor(0, pixels.Color(0,250,0));
        pixels.show();

        int n = atoi16(dat[1]) - 1;
        if (validateDFPlayer(n, dat[0])) {
          dfPlayer[n].pause();
          serialPort->print("!!!!!");
        }

        pixels.clear();
        pixels.show();

      //----------------------------------------------------------
      // 機能　　　　：再生再開
      // コマンド形式：DPR:<機器番号0～1>!
      // 返却値　　　："!!!!!" 正常 ／ "DPR!!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DPR") == 0 && dat_cnt >= 2) {

        pixels.setPixelColor(0, pixels.Color(0,250,0));
        pixels.show();

        int n = atoi16(dat[1]) - 1;
        if (validateDFPlayer(n, dat[0])) {
          dfPlayer[n].start();
          serialPort->print("!!!!!");
        }

        pixels.clear();
        pixels.show();

      //----------------------------------------------------------
      // 機能　　　　：イコライザー設定（DEF）
      // コマンド形式：DEF:<機器番号0～1>:<タイプ0～5>!
      //              0: Normal, 1: Pop, 2: Rock,
      //              3: Jazz, 4: Classic, 5: Bass
      // 返却値　　　："!!!!!" 正常 ／ "DEF!!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DEF") == 0 && dat_cnt >= 3) {

        pixels.setPixelColor(0, pixels.Color(0,250,0));
        pixels.show();

        int n = atoi16(dat[1]) - 1;
        if (validateDFPlayer(n, dat[0])) {
          dfPlayer[n].EQ(atoi16(dat[2]));
          serialPort->print("!!!!!");
        }

        pixels.clear();
        pixels.show();

      //----------------------------------------------------------
      // 機能　　　　：音量設定（VOL）
      // コマンド形式：VOL:<機器番号0～1>:<音量0～30>!
      // 返却値　　　："!!!!!" 正常 ／ "VOL!!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "VOL") == 0 && dat_cnt >= 3) {

        pixels.setPixelColor(0, pixels.Color(0,250,0));
        pixels.show();

        int n = atoi16(dat[1]) - 1;
        if (validateDFPlayer(n, dat[0])) {
          dfPlayer[n].volume(atoi16(dat[2]));
          serialPort->print("!!!!!");
        }

        pixels.clear();
        pixels.show();

      //===========================================================
      // 情報
      //===========================================================
      //----------------------------------------------------------
      // 機能　　　　：ファームウェアのバージョン
      // コマンド形式：DPX:<機器番号1〜4>!
      // 返却値　　　："xyzz!" x:メジャー／y:マイナー／zz:リビジョン
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "VER") == 0) {
        serialPort->print(Version);
      //----------------------------------------------------------
      // 機能　　　　：PWM接続状況確認（PWX）
      // コマンド形式：機器番号 0～15>!
      // 返却値　　　：チャンネルごとのビットマスク
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "PWX") == 0 && dat_cnt >= 2) {
        int id = atoi16(dat[1]);
        int val = 0xFFFF;
        if (id >= 0 && id < PWM_COUNT) {
          val = PWM_Connected[id] ? 1 : 0;
        }
        sprintf(rets, "%04X!", val & 0xFFFF);
        serialPort->print(rets);
      //----------------------------------------------------------
      // 機能　　　　：DFPlayer接続状況確認（DPX）
      // コマンド形式：DPX:<機器番号1〜4>!
      // 返却値　　　：機器ごとのビットマスク
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DPX") == 0 && dat_cnt >= 2) {
        int id = atoi16(dat[1]);
        int val = 0xFFFF;
        if (id >= 1 && id <= 4) {
          val = dfConnected[id - 1] ? 1 : 0;
        }
        sprintf(rets, "%04X!", val & 0xFFFF);
        serialPort->print(rets);

      //===========================================================
      // 機能　　　　：コマンドの該当なしのエラー返却
      // コマンド形式：～!
      // 返却値　　　："<コマンド名>?!"
      //===========================================================
      } else {
        char resp[6] = "!!!?!";
        strncpy(resp, dat[0], 3);
        serialPort->print(resp);
      }
    }
  }
}
