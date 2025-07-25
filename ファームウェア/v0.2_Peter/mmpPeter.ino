//=============================================================================
//  Ardipy MMPエディション
//-----------------------------------------------------------------------------
// Ver 0.01.008　2025/06/12 By Takanari.Kureha
//   ・返信が2階になるバグを修正
//   ・DFPlayer制御コマンドを仮追加（再生/停止/音量/EQ/状態確認など）
//=============================================================================
#include <Wire.h>
#include <PCA9685.h>            //PCA9685用ヘッダーファイル（秋月電子通商作成）
//#include <DFRobotDFPlayerMini.h>

int inp_cnt = 0;

//=============================================================================
//PCA9685
//=============================================================================
//□パルス幅
#define SERVOMIN 150  //最小 (標準的なサーボパルスに設定)
#define SERVOMAX 600  //最大 (標準的なサーボパルスに設定)

//□アドレス・パターン数
const int PWM_COUNT = 48;

//□アドレス・パターン
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

//=============================================================================
// DFPlayer関連
//=============================================================================
/*
  HardwareSerial mySerial1(1);
  DFRobotDFPlayerMini myDFPlayer; // Serial1を直接使うため追加のインスタンス化不要
*/

//=============================================================================
// 初期処理
//=============================================================================
void setup(){
    // put your setup code here, to run once:
    Wire.begin();
    Serial.begin(921600);

/*
  //mySerial1.begin(9600, SERIAL_8N1, 8, 9);
    Serial1.setTX(9);        // TX ピン設定（DFPlayerの RXに接続）
    Serial1.setRX(8);        // RX ピン設定（DFPlayerの TXに接続）
    Serial1.begin(9600);     // 通常の UART 初期化
*/

    //★PCA9685
    for (int i = 0; i < PWM_COUNT; i++) {
      PWM[i].begin();         //初期設定
      PWM[i].setPWMFreq(60);  //PWM周期を60Hzに設定
    }
  //★DFPlayer
/*
  if (!myDFPlayer.begin(Serial1)) {
    Serial.println("DFPlayer Init Failed");
  } else {
    myDFPlayer.volume(20);
  }
*/
}

//=============================================================================
// メイン処理
//=============================================================================
void loop() {

  //★PCA9685
  int   pwmNo;        // 総合ポートNo
  int   pwmVal;       // PRM値
  int   pwmAn;        // 角度
  int   pwmID;        // アドレスリスト添え字
  int   pwmCh;        // ボード別ポートNo

  char  input[30];    // 受信文字列
  char  dat[5][10];   // 項目データ

  char  *div_dat;     // 項目分割ワーク
  int   dat_cnt;      // 項目データカウンタ

  char  rets[30];     // 返信用文字列

  //◇
  if (Serial.available()) {
  //├→(シリアル通信が有効な場合)

    //〇シリアルデータを配列変数に格納する
    input[inp_cnt] = Serial.read();

    //◇１通信を受信し終えたら、コマンド処理する。
    if (inp_cnt > 30 || input[inp_cnt] == '!') {
    //├→(30データ以上 または 終端文字を検出)

      //〇末尾データを終端文字で上書きする
      input[inp_cnt] = '\0';

      //〇カウンタをゼロ初期化する
      inp_cnt = 0;
      dat_cnt = 0;

      //◎受信文字列を項目単位に分割する
      for(div_dat = strtok(input, ":"); div_dat; div_dat = strtok(NULL, ":")) {
        //〇項目データに格納する
        strcpy(dat[dat_cnt], div_dat);
        dat_cnt++;
      }

      //=======================================================================
      //【データ】[0]:"PWM"・・・ＰＷＭ出力(PCA9685)
      //         [1]:総合ポート番号
      //         [2]:出力値(角度)
      //【返　信】[正常終了("!!!!!")｜異常終了("----!")]
      //=======================================================================
      if( strcmp( dat[0], "PWM" ) == 0 ){                     // コマンド識別
          pwmNo  = atoi16(dat[1]);                            // 総合ポート番号
          pwmVal = atoi16(dat[2]);                            // PWN値
          pwmID  = pwmNo / 16;                                // ボード特定
          pwmCh  = pwmNo - (16 * pwmID);                      // ポート特定

          if ( pwmID < PWM_COUNT ){
            PWM[pwmID].setPWM( pwmCh, 0, pwmVal );            // ポート出力
            Serial.print( "!!!!!" );                          // 正常を返信
          }else{
            Serial.print( "----!" );                          // 異常を返信
          }
      }
      //=======================================================================
      else if( strcmp( dat[0], "PWA" ) == 0 ){                 // コマンド識別
          pwmNo = atoi16(dat[1]);                             // 総合ポート番号
          pwmAn = atoi16(dat[2]);                             // 角度
          pwmAn = map(pwmAn, 0, 180, SERVOMIN, SERVOMAX);     // PWM値に変換
          pwmID = pwmNo / 16;                                 // ボード特定
          pwmCh = pwmNo - (16 * pwmID);                       // ポート特定

          if ( pwmID < PWM_COUNT ){
            PWM[pwmID].setPWM( pwmCh, 0, pwmAn );             // ポート出力
            Serial.print( "!!!!!" );                          // 正常を返信
          }else{
            Serial.print( "----!" );                          // 異常を返信
          }
      }

      //=======================================================================
      //【データ】[0]:"POW"・・・デジタル出力
      // 　　　　 [1]:ポート番号
      //         [2]:出力値(1or0)
      //【返　信】正常終了("!!!!!")
      //=======================================================================
      else if(strcmp(dat[0], "POW")==0){                      // コマンド識別
         pinMode(atoi16(dat[1]), OUTPUT);                     // ポート割当
         digitalWrite(atoi16(dat[1]), atoi16(dat[2]));        // ポート出力
         Serial.print("!!!!!");                               // 正常を返信
      }
      //=======================================================================
      //【データ】[0]:"POR"・・・デジタル入力
      // 　　　　 [1]:ポート番号
      //【返　信】取得値（16進数1桁＋"!!!!"）
      //=======================================================================
      else if(strcmp(dat[0], "POR")==0){                      // コマンド識別
         pinMode(atoi16(dat[1]), INPUT);                      // ポート割当
         unsigned char retb = digitalRead(atoi16(dat[1]));    // ポート値計測
         sprintf(rets, "%01x!!!!", retb);                     // 計測値を清書
         Serial.print(rets);                                  // 計測値を返信
      }
      //=======================================================================
      //【データ】[0]:"ADR"・・・アナログ入力
      // 　　　　 [1]:ポート番号
      //【返　信】取得値（16進数3桁＋"!!"）
      //=======================================================================
      else if(strcmp(dat[0], "ADR")==0){                      // コマンド識別
         word retw = analogRead(atoi16(dat[1]));              // ポート値計測
         sprintf(rets, "%03x!!", retw);                       // 計測値を清書
         Serial.print(rets);                                  // 計測値を返信
      }
/*
      //=======================================================================
      //【データ】[0]:"DFS"・・・ＤＦプレイヤー：トラック番号を再生
      // 　　　　 [1]:トラック番号
      //【返　信】取得値（16進数3桁＋"!!"）
      //=======================================================================
      else if (strcmp(dat[0], "DFP") == 0) {
        myDFPlayer.play(atoi16(dat[1])); Serial.print("!!!!!");
      }
      //=======================================================================
      //【データ】[0]:"DFLS"・・・ＤＦプレイヤー：トラックをループ再生
      // 　　　　 [1]:トラック番号
      //【返　信】取得値（"!!!!!"）
      //=======================================================================
      else if (strcmp(dat[0], "DFLS") == 0) {
        myDFPlayer.loop(atoi16(dat[1])); Serial.print("!!!!!");
      }
      //=======================================================================
      //【データ】[0]:"DFV"・・・ＤＦプレイヤー：音量を設定（0〜30）
      // 　　　　 [1]:音量
      //【返　信】取得値（!!!!! / ----!）
      //=======================================================================
      else if (strcmp(dat[0], "DFV") == 0) {
        int vol = atoi16(dat[1]);
        if (vol >= 0 && vol <= 30) { myDFPlayer.volume(vol); Serial.print("!!!!!"); }
        else Serial.print("----!");
      }
      //=======================================================================
      //【データ】[0]:"DFS"・・・ＤＦプレイヤー：再開（start）
      //【返　信】取得値（"!!!!!"）
      //=======================================================================
      else if (strcmp(dat[0], "DFS") == 0) {
        myDFPlayer.start(); Serial.print("!!!!!");
      }
      //=======================================================================
      //【データ】[0]:"DFPZ"・・・ＤＦプレイヤー：一時停止（pause）
      //【返　信】取得値（"!!!!!"）
      //=======================================================================
      else if (strcmp(dat[0], "DFPZ") == 0) {
        myDFPlayer.pause(); Serial.print("!!!!!");
      }
      //=======================================================================
      //【データ】[0]:"DFST"・・・ＤＦプレイヤー：再生停止
      //【返　信】取得値（"!!!!!"）
      //=======================================================================
      else if (strcmp(dat[0], "DFST") == 0) {
        myDFPlayer.stop(); Serial.print("!!!!!");
      }
      //=======================================================================
      //【データ】[0]:"DFSTQ"・・・ＤＦプレイヤー：再生状態を確認
      //【返　信】取得値（0!!!! / 1!!!!）
      //=======================================================================
      else if (strcmp(dat[0], "DFSTQ") == 0) {
        sprintf(rets, "%01x!!!!", myDFPlayer.readState()); Serial.print(rets);
      }
      //=======================================================================
      //【データ】[0]:"DFSD"・・・ＤＦプレイヤー：SDカードの検出状況確認
      //【返　信】取得値（0!!!! / 1!!!!）
      //=======================================================================
      else if (strcmp(dat[0], "DFSD") == 0) {
        sprintf(rets, "%01x!!!!", myDFPlayer.readFileCounts() > 0 ? 1 : 0);
        Serial.print(rets);
      }
      //=======================================================================
      //【データ】[0]:"DFSD"・・・ＤＦプレイヤー：EQ（イコライザ）設定
      //【返　信】取得値（!!!!! / ----!）
      //=======================================================================
      else if (strcmp(dat[0], "DFEQ") == 0) {
        int eq = atoi16(dat[1]);
        if (eq >= 0 && eq <= 5) { myDFPlayer.EQ(eq); Serial.print("!!!!!"); }
        else Serial.print("----!");
      }
 */
      //=======================================================================
      // エラー発生
      //【返　信】異常終了("----!")
      //=======================================================================
      else {
        Serial.print( "----!" );                                // 異常を返信
      }
    } else {
      //〇受信項目数を増やす
      inp_cnt++;
    }
  }
}

//=======================================================================
// 文字エンコード
//=======================================================================
int atoi16( const char *NumberString ){
    char *stopString;
    int result = strtol( NumberString, &stopString, 16 );
    return result;
}