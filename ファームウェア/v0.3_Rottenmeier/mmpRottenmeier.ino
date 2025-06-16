//=============================================================================
//  mmpRottenmeier（DFPlayer + PCA9685 + DigitalIO + I2C）
//----------------------------------------------------------------------------- 
// Ver 0.3.02 - 2025/06/12 By Takanari.Kureha
//   ・ボーレート変更の機能を追加
//   ・DFPlayerを2台構成に対応 ※2台目はGPIOシリアルと排他利用
//   ・もろもろリファクタリング
//=============================================================================
#include <Wire.h>
#include <PCA9685.h>
#include <DFRobotDFPlayerMini.h>
const char* Version = "0301!";

//----------------------------------------------------------
// シリアル通信
//----------------------------------------------------------
Stream* serialPort; // インスタンス
int inp_cnt = 0;    // コマンド＋引数の個数

// 切り替えスイッチのGPIO
#define SERIAL_SELECT_PIN 2 // スイッチ１：通信方式切替
#define BAUD_SELECT_PIN1 14 // スイッチ２：ボーレート選択
#define BAUD_SELECT_PIN2 15 // スイッチ３：ボーレート選択

// ボーレートのプリセット
#define BAUD_00 115200      //OFF｜OFF
#define BAUD_10 9600        //ON ｜OFF
#define BAUD_01 230400      //OFF｜ON
#define BAUD_11 921600      //ON ｜ON

//----------------------------------------------------------
// ＰＷＭ機器(PCA9685)
//----------------------------------------------------------
#define SERVOMIN 150  // サーボの最低PWM出力
#define SERVOMAX 600  // サーボの最大PWM出力
#define PWM_COUNT 16  // 扱えるPCA9685の機器の最大個数

// 対応するPCA9685のアドレス(この順番が機器番号)
PCA9685 PWM[PWM_COUNT] = {
  PCA9685(0x40), PCA9685(0x41), PCA9685(0x42), PCA9685(0x43),
  PCA9685(0x44), PCA9685(0x45), PCA9685(0x46), PCA9685(0x47),
  PCA9685(0x48), PCA9685(0x49), PCA9685(0x4A), PCA9685(0x4B),
  PCA9685(0x4C), PCA9685(0x4D), PCA9685(0x4E), PCA9685(0x4F)
};

// 機器ごとの接続済みフラグ(True:接続済み／False:未接続)
bool PWM_Connected[PWM_COUNT];

//----------------------------------------------------------
// mp3機器(DFPlayer)
//----------------------------------------------------------
DFRobotDFPlayerMini dfPlayer[2];        // インスタンス
bool dfConnected[2] = { false, false }; // 接続済みフラグ

//----------------------------------------------------------
// アナログ入力機器(HC4067)
//----------------------------------------------------------
const int analogGPIOs[4] = {26, 27, 28, 29}; //アドレスバス
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
  delay(10);
  bool useUSB = digitalRead(SERIAL_SELECT_PIN);
  if (useUSB) {
    Serial.begin(selectedBaud);
    serialPort = &Serial;
  } else {
    Serial1.setTX(0);
    Serial1.setRX(1);
    Serial1.begin(selectedBaud);
    serialPort = &Serial1;
  }

  // PCA9685 16台を初期化、接続済フラグをONにする
  Wire.begin();
  delay(1000);
  for (int i = 0; i < PWM_COUNT; i++) {
    PWM[i].begin();
    PWM[i].setPWMFreq(60);
    PWM_Connected[i] = true;
  }

  // DFPlayer(1台目)：Serial2に(規定値：GPIO8,9)で接続
  //  Serial2.setTX(6);
  //  Serial2.setRX(7);
  Serial2.begin(9600);      // 規定ボーレートで接続
  if (dfPlayer[0].begin(Serial2)) {
    dfPlayer[0].volume(20); // 音量を20にセット
    dfConnected[0] = true;  // 接続済フラグをONにする
  }

  // DFPlayer(2台目)：：Serial1に(規定値：GPIO0,1)で接続
  if (useUSB) {
    Serial1.begin(9600);      // 規定ボーレートで接続
    if (dfPlayer[1].begin(Serial1)) {
      dfPlayer[1].volume(20); // 音量を20にセット
      dfConnected[1] = true;  // 接続済フラグをONにする
    }
  }

  // アナログ入力値バッファをクリア
  for (int i = 0; i < 16; i++)
    for (int j = 0; j < 4; j++)
      anaValues[i][j] = 0;

  // マルチプレクサのアドレスバス用ピンを設定
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
}

int atoi16(const char *NumberString) {
  char *stopString;
  return strtol(NumberString, &stopString, 16);
}

//===========================================================
// メイン処理
//===========================================================
void loop() {

  // 送受信関連
  static char input[30] = {0};  // 受信データ・バッファ
  char dat[5][10], *div_dat;    // [0]:コマンド，[1～]:引数
  int dat_cnt;                  // コマンド＋引数の個数
  char rets[30];                // リターン値バッファ

  // ＰＣＡ９６８５関連
  int pwmNo;  // 通しチャンネル番号
  int pwmVal; // ＰＷＭ出力値
  int pwmID;  // PCA9685番号
  int pwmCh;  // PCA9685番号別のチャンネル番号

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

        // 引数からHC4067の構成を設定
        int newPl = atoi16(dat[1]);
        int newSw = atoi16(dat[2]);

        if (newPl >= 1 && newPl <= 16 && newSw >= 1 && newSw <= 4) {

          // カレント・アドレスをセット
          anaSwitchCnt = newSw;
          anaPlayerCnt = newPl;

          // カレント・アドレスのアナログ値バッファをクリア
          for (int i = 0; i < 16; i++)
            for (int j = 0; j < 4; j++)
              anaValues[i][j] = 0;

          // 正常返信
          serialPort->print("!!!!!");
        // エラー返信
        } else serialPort->print("ANS!!");
      //----------------------------------------------------------
      // 機能　　　　：値バッファ更新（ANU）
      // コマンド形式：ANU!
      // 返却値　　　："!!!!!" 正常
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "ANU") == 0) {

        for (int ch = 0; ch < anaPlayerCnt; ch++) {

          // アドレス・バスをセット
          digitalWrite(10, (ch >> 0) & 1);
          digitalWrite(11, (ch >> 1) & 1);
          digitalWrite(12, (ch >> 2) & 1);
          digitalWrite(13, (ch >> 3) & 1);
          delayMicroseconds(10);

          // 同じアドレス・バスで全HC4067のアナログ値を読み取り
          // アナログ値バッファに格納
          for (int sw = 0; sw < anaSwitchCnt; sw++) {
            int pin = analogGPIOs[sw];
            anaValues[ch][sw] = analogRead(pin);
          }
        }

        // 正常返信
        serialPort->print("!!!!!");
      //----------------------------------------------------------
      // 機能　　　　：値バッファ参照（ANR）
      // コマンド形式：ANR:<プレイヤー番号1～16>:<スイッチ番号1～4>!
      // 返却値　　　：アナログ値 ／ "ANR!!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "ANR") == 0 && dat_cnt >= 2) {

        // 引数を取得
        int pl = atoi16(dat[1]);
        int sw = atoi16(dat[2]);

        // 正常(値)返信
        if (pl < 16 && sw < 4) {
          sprintf(rets, "%04X!", anaValues[pl][sw]);
          serialPort->print(rets);

        // エラー返信
        } else serialPort->print("ANR!!");

      //===========================================================
      // ＰＷＭ関連(PCA9685)
      //===========================================================
      //----------------------------------------------------------
      // 出力コマンド
      // 機能　　　　：ＰＷＭ出力（PWM）
      // コマンド形式：PWM:<ＰＷＭ値0～4095>!
      // 返却値　　　："!!!!!" 正常 ／ "PWS!!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "PWM") == 0 && dat_cnt >= 3) {
        pwmNo   = atoi16(dat[1]);
        pwmVal  = atoi16(dat[2]);
        pwmID   = pwmNo / 16;
        pwmCh   = pwmNo % 16;
        if (pwmID < PWM_COUNT && PWM_Connected[pwmID]) {
          PWM[pwmID].setPWM(pwmCh, 0, pwmVal);
          serialPort->print("!!!!!");
        } else serialPort->print("PWS!!");

      //===========================================================
      // ｉ２ｃ関連
      //===========================================================
      //----------------------------------------------------------
      // 出力コマンド
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "I2W") == 0 && dat_cnt >= 4) {

        // 引数を取得
        int addr = atoi16(dat[1]);
        int reg = atoi16(dat[2]);
        int val = atoi16(dat[3]);

        // レジスタに値を送信
        Wire.beginTransmission(addr);
        Wire.write(reg);
        Wire.write(val);
        Wire.endTransmission();

        // 正常返信
        serialPort->print("!!!!!");
      //----------------------------------------------------------
      // 入力コマンド
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "I2R") == 0 && dat_cnt >= 3) {

        // 引数を取得
        int addr = atoi16(dat[1]);
        int reg = atoi16(dat[2]);

        // レジスタの値を受信
        Wire.beginTransmission(addr);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(addr, 1);
        int val = Wire.available() ? Wire.read() : 0xFF;

        // 正常(値)返信
        sprintf(rets, "%04X!", val);
        serialPort->print(rets);

      //============================================================
      // デジタル関連
      //============================================================
      //----------------------------------------------------------
      // 出力コマンド：ポート番号：出力値
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "POW") == 0 && dat_cnt >= 3) {

        // 引数を取得
        int port = atoi16(dat[1]);
        int val = atoi16(dat[2]);

        // デジタル・ピンに値を出力
        pinMode(port, OUTPUT);
        digitalWrite(port, val);

        // 正常返信
        serialPort->print("!!!!!");
      //----------------------------------------------------------
      // 入力コマンド：ポート番号
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "POR") == 0 && dat_cnt >= 2) {

        // 引数を取得
        int port = atoi16(dat[1]);

        // デジタル・ピンから値を入力
        pinMode(port, INPUT);
        int val = digitalRead(port);

        // 正常(値)返信
        sprintf(rets, "%04X!", val);
        serialPort->print(rets);
      //----------------------------------------------------------
      // 入出力コマンド：ポート番号：出力値　※出力後に入力する
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "IO") == 0 && dat_cnt >= 3) {

        // 引数を取得
        int port = atoi16(dat[1]);
        int val = atoi16(dat[2]);

        // デジタル・ピンに値を出力
        pinMode(port, OUTPUT);
        digitalWrite(port, val);

        // デジタル・ピンから値を入力
        delayMicroseconds(10);
        pinMode(port, INPUT);
        int result = digitalRead(port);

        // 正常(値)返信
        sprintf(rets, "%04X!", result);
        serialPort->print(rets);

      //===========================================================
      // ｍｐ３関連(DFPlayer)
      //  <コマンド>:<機器番号1または2>:<その他引数>!
      //===========================================================
      //----------------------------------------------------------
      // 再生：指定トラック
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DPL") == 0 && dat_cnt >= 3) {
        int n = atoi16(dat[1]) - 1;
        if (n >= 0 && n < 2 && dfConnected[n]) {
          dfPlayer[n].play(atoi16(dat[2]));
          serialPort->print("!!!!!");
        } else serialPort->print("DPL!!");
      //----------------------------------------------------------
      // 再生：指定トラック（ループ）
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DRP") == 0 && dat_cnt >= 3) {
        int n = atoi16(dat[1]) - 1;
        if (n >= 0 && n < 2 && dfConnected[n]) dfPlayer[n].loop(atoi16(dat[2]));
        serialPort->print("!!!!!");
      //----------------------------------------------------------
      // 再生：指定フォルダ内トラック
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DIR") == 0 && dat_cnt >= 4) {
        int n = atoi16(dat[1]) - 1;
        if (n >= 0 && n < 2 && dfConnected[n]) dfPlayer[n].playFolder(atoi16(dat[2]), atoi16(dat[3]));
        serialPort->print("!!!!!");
      //----------------------------------------------------------
      // 停止
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DSP") == 0 && dat_cnt >= 2) {
        int n = atoi16(dat[1]) - 1;
        if (n >= 0 && n < 2 && dfConnected[n]) {
          dfPlayer[n].stop();
          serialPort->print("!!!!!");
        } else serialPort->print("DSP!!");
      //----------------------------------------------------------
      // 一時停止
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DPA") == 0 && dat_cnt >= 2) {
        int n = atoi16(dat[1]) - 1;
        if (n >= 0 && n < 2 && dfConnected[n]) dfPlayer[n].pause();
        serialPort->print("!!!!!");
      //----------------------------------------------------------
      // 再生再開
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DPR") == 0 && dat_cnt >= 2) {
        int n = atoi16(dat[1]) - 1;
        if (n >= 0 && n < 2 && dfConnected[n]) dfPlayer[n].start();
        serialPort->print("!!!!!");
      //----------------------------------------------------------
      // 再生状態：0停止, 1再生, 2一時停止
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DST") == 0 && dat_cnt >= 2) {
        int n = atoi16(dat[1]) - 1;
        if (n >= 0 && n < 2 && dfConnected[n]) {
          int state = dfPlayer[n].readState();
          sprintf(rets, "%04X!", state);
          serialPort->print(rets);
        } else serialPort->print("----!");
      //----------------------------------------------------------
      // 再生中トラック番号取得（戻り値は16進数）
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DQT") == 0 && dat_cnt >= 2) {
        int n = atoi16(dat[1]) - 1;
        if (n >= 0 && n < 2 && dfConnected[n]) {
          int val = dfPlayer[n].readCurrentFileNumber();
          sprintf(rets, "%04X!", val);
          serialPort->print(rets);
        } else serialPort->print("----!");
      //----------------------------------------------------------
      // 機能　　　　：全トラック数取得（DTC）
      // コマンド形式：DTC:<機器番号0～1>!
      // 返却値　　　："!!!!!" 正常 ／ "----!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DTC") == 0 && dat_cnt >= 2) {
        int n = atoi16(dat[1]) - 1;
        if (n >= 0 && n < 2 && dfConnected[n]) {
          int count = dfPlayer[n].readFileCounts();
          sprintf(rets, "%04X!", count);
          serialPort->print(rets);
        } else serialPort->print("----!");
      //----------------------------------------------------------
      // 機能　　　　：イコライザー設定（DEF）
      // コマンド形式：DEF:<機器番号0～1>:<タイプ0～30>!
      //              0: Normal, 1: Pop, 2: Rock,
      //              3: Jazz, 4: Classic, 5: Bass
      // 返却値　　　："!!!!!" 正常 ／ "----!" エラー
      // イコライザー設定：0〜5
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "DEF") == 0 && dat_cnt >= 3) {
        int n = atoi16(dat[1]) - 1;
        if (n >= 0 && n < 2 && dfConnected[n]) dfPlayer[n].EQ(atoi16(dat[2]));
        serialPort->print("!!!!!");
      //----------------------------------------------------------
      // 機能　　　　：音量設定（VOL）
      // コマンド形式：VOL:<機器番号0～1>:<音量0～30>!
      // 返却値　　　："!!!!!" 正常 ／ "----!" エラー
      //----------------------------------------------------------
      } else if (strcmp(dat[0], "VOL") == 0 && dat_cnt >= 3) {
        int n = atoi16(dat[1]) - 1;
        if (n >= 0 && n < 2 && dfConnected[n]) {
          dfPlayer[n].volume(atoi16(dat[2]));
          serialPort->print("!!!!!");
        // エラー返信
        } else serialPort->print("VOL!!");

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
      // 返却値　　　："0001!" 接続／"0000!" 未接続
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
      // 返却値　　　："0001!" 接続／"0000!" 未接続
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
