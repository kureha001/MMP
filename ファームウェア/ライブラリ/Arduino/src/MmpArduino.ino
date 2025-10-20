// MmpArduino.ino
//============================================================
// ＭＭＰコマンド テスト（ＡＰＩ経由で実行）
//============================================================
#include <Arduino.h>
#include "MMP.h"
#include "webui.hpp"
#include "MmpClient.h"

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
using Mmp::Core::MmpClient;
MmpClient mmp;
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

//============================================================
// Wifi設定情報（各自の環境に合わせる）
//============================================================
const char*     WIFI_SSID = "Buffalo-G-C0F0";
const char*     WIFI_PASS = "6shkfa53dk8ks";

//============================================================
// テスト構成
// `USE_*` の ON/OFF で切替る
// `USE_*` 意外は 各自の環境に合わせる
//============================================================
//(1) 直結：シリアル接続（プラットフォーム自動）
const bool      USE_SERIAL_AUTO = false; // true | false

//(2) TCPブリッジ：ser2net
const bool      USE_TCP   = true;      // true | false
//const char*     TCP_HOST  = "192.168.2.124";
const char*     TCP_HOST  = "192.168.2.108";
const uint16_t  TCP_PORT  = 5331;

//(3) TCPブリッジ：usb4a(Pydroid)
// ※未使用
const bool      USE_USB4A_DIRECT = false;    // true | false
const uint16_t  USB4A_INDEX      = 0;

//(4) TCP共通：read_one_char の待ち秒（?timeout=） 
const uint32_t  IO_MS     = 1000;
const uint32_t  VERIFY_MS = 2000;

//============================================================
// 0) 基本情報(バージョン/PCA9685/DFPlayer)
//============================================================
void RunInfo() {
    Serial.println("　・通信ポート  : " + mmp.ConnectedPort()                 );
    Serial.println("　・接続速度    : " + String(mmp.ConnectedBaud()) + "bps" );
    Serial.println("　・バージョン  : " + String(mmp.info.version())          );
    Serial.println("　・PCA9685 [0] : " + String(mmp.pwm.info.connect(0))     );
    Serial.println("　・DFPlayer[1] : " + String(mmp.mp3.info.connect(1))   );
}

//============================================================
// 1) アナログ入力(HC4067)
//============================================================
void RunAnalog() {

    Serial.println("１.アナログ入力（ HC4067：JoyPad1,2 ）");

    bool ok = mmp.analog.setup(2, 4);
    Serial.println("　・アクセス範囲指定 → [2,4]  : " + bool2str(ok) );
    if (!ok) { Serial.println("　 <<中断>>\n"); return; }

    ok = mmp.analog.input();
    Serial.println("　・アナログ値をバッファに格納 : " + bool2str(ok) );
    if (!ok) { Serial.println(" 　<<中断>>\n"); return; }

    Serial.println("　・バッファを参照");
    for (int x = 0; x <= 1; x += 1) {
        Serial.println  ("　　JoyPad[" + String(x + 1) + "]");
        for (int y = 0; y <= 3; y += 1) {
        //Serial.println("　　　[" + String(y) + "] " + String(mmp.analog.READ  (x, y       ) ) );
        //Serial.println("　　　[" + String(y) + "] " + String(mmp.analog.round (x, y, 100) ) );
        //Serial.println("　　　[" + String(y) + "] " + String(mmp.analog.roundU(x, y, 100) ) );
        Serial.println("　　　[" + String(y) + "] " + String(mmp.analog.roundD(x, y, 100) ) );
        }
    }

    Serial.println("　[終了]\n");
}

//============================================================
// 2) デジタル入出力
//============================================================
void RunDigital() {

    Serial.println("２.デジタル入出力（ GPIO ）");
    Serial.println("　・入力");
    Serial.println("　　[2] " + int2strOnOff(mmp.digital.input(2)) );
    Serial.println("　　[6] " + int2strOnOff(mmp.digital.input(6)) );
    Serial.println("　　[7] " + int2strOnOff(mmp.digital.input(7)) );

    Serial.println("　・出力[3]" );
    for (int cnt_eq = 0; cnt_eq <= 5; cnt_eq++)
    {
        Serial.println("　　[HIGH] " + bool2str(mmp.digital.output(3, 1)) ); delay(500);
        Serial.println("　　[LOW ] " + bool2str(mmp.digital.output(3, 0)) ); delay(500);
    }

    Serial.println("　[終了]\n");
}

//============================================================
// 3) MP3：フォルダ1のトラック再生,リピート再生
//============================================================
void RunMp3Playlist() {

    Serial.println("３.ＭＰ３再生（ DFPlayer ）");

    Serial.println("　・音量   [20]  " + bool2str(mmp.mp3.set.volume(1, 20) ) );
    Serial.println("　・ループ [OFF] " + String  (mmp.mp3.track.loop(1,0)   ) );

    Serial.println("　・再生");
    for (int track = 1; track <= 3; ++track) {
        Serial.println("　　[F=1,T=" + String(track) + "]    "
                                             + String(mmp.mp3.track.play(1, 1, track) ) );
        delay(3000);
    }

    Serial.println("　・一時停止           " + String(mmp.mp3.track.stop(1)           ) );
    Serial.println("　・再生   [F=2,T=102] " + String(mmp.mp3.track.play(1, 2, 102)   ) );
    Serial.println("　・ループ [ON]        " + String(mmp.mp3.track.loop(1, 1)        ) );
    delay(10000);

    Serial.print  ("　・停止 : " + String(mmp.mp3.track.stop(1) ) );

    Serial.println("　[終了]\n");
}

//============================================================
// 4) MP3 制御：状態取得/一時停止/再開/停止/EQ/音量
//============================================================
void RunMp3Control() {

    Serial.println("４.ＭＰ３制御（ DFPlayer ）");
    
    Serial.println("　・音量 → 20      : " + bool2str(mmp.mp3.set.volume(1, 20)   ) );
    Serial.println("　・再生 → F=4,T=1 : " + String  (mmp.mp3.track.play(1, 4, 1) ) );
    Serial.println("　・ループ → OFF   : " + String  (mmp.mp3.track.loop(1, 0)    ) );
    
    Serial.println("　・参照");
    Serial.println("　　・トラック状況  : " + String(mmp.mp3.info.track(1)  ) );
    Serial.println("　　・音量          : " + String(mmp.mp3.info.volume(1) ) );
    Serial.println("　　・イコライザ    : " + String(mmp.mp3.info.eq(1)     ) );
    Serial.println("　　・総ファイル数  : " + String(mmp.mp3.info.files(1)  ) );
    Serial.println("　　・現在ファイル  : " + String(mmp.mp3.info.fileID(1) ) );

    Serial.println("　・一時停止        : " + String(mmp.mp3.track.pause(1) ) );
    delay(2000);
    
    Serial.println("　・再開            : " + String(mmp.mp3.track.start(1) ) );

    // イコライザのモードを順次切り替える
    Serial.println("　・イコライザー");
    for (int cnt_eq = 0; cnt_eq <= 5; ++cnt_eq) {
        Serial.println("　　[" + String(cnt_eq) + "] " + bool2str(mmp.mp3.set.eq(1, cnt_eq) ) );
        delay(3000);
    }

    Serial.println("　・音量");
    for (int cnt_vol = 0; cnt_vol <= 30; cnt_vol += 5) {
        Serial.println("　　[" + String(cnt_vol) + "] " + bool2str(mmp.mp3.set.volume(1, cnt_vol) ) );
        delay(1000);
    }

    Serial.println("　・停止 : " + String(mmp.mp3.track.stop(1) ) );

    Serial.println("　[終了]\n");
}

//============================================================
// 5) PWM 生値：ch0=180度型、ch15=連続回転型
//============================================================
const int SERVO_MIN     = 150; // CA9685 12bitの生値（例: 150）
const int SERVO_MAX     = 600; // 同上              （例: 600）
const int SERVO_MID     = (SERVO_MIN + SERVO_MAX) / 2;
const int OffsetMax360  = 60;
const int STEPS         = 80;
const int STEP          = 1;
const int STEP_DELAY_MS = 20;
const int CH_180        = 0;
const int CH_360        = 15;
const int PCA_ADDR      = 0x40;

void RunPwm(bool mode) {

    Serial.println(
        String(mode ? "５.ＰＷＭ" : "６.Ｉ２Ｃ")
        + "（ PCA9685：サーボモータ180度型,連続回転型 ）"
    );

    Serial.println("　・初期位置");
    if (mode){
        mmp.pwm.output(CH_180, SERVO_MID);
        mmp.pwm.output(CH_360, SERVO_MID);
    } else {
        RunI2C(CH_180, SERVO_MID);
        RunI2C(CH_360, SERVO_MID);
    }
    delay(300);

    Serial.println("　・正転,加速");
    for (int i = 0; i <= STEPS; i+=STEP) {
        int pwm180 = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i  / STEPS;
        int pwm360 = SERVO_MID + (OffsetMax360           * i) / STEPS;
        if (mode)
        {
            mmp.pwm.output(CH_180, pwm180);
            mmp.pwm.output(CH_360, pwm360);
        }else{
            RunI2C(CH_180, pwm180);
            RunI2C(CH_360, pwm360);
        }
        delay(STEP_DELAY_MS);
    }

    Serial.println("　・逆転,減速");
    for (int i = STEPS; i >= 0; i-=STEP) {
        int pwm180 = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i   / STEPS;
        int pwm360 = SERVO_MID + (OffsetMax360           * i ) / STEPS;
        if (mode){
            mmp.pwm.output(CH_180, pwm180);
            mmp.pwm.output(CH_360, pwm360);
        } else {
            RunI2C(CH_180, pwm180);
            RunI2C(CH_360, pwm360);
        }
        delay(STEP_DELAY_MS);
    }

    Serial.println("　・初期位置");
    if (mode){
        mmp.pwm.output(CH_180, SERVO_MID);
        mmp.pwm.output(CH_360, SERVO_MID);
    } else {
        RunI2C(CH_180, SERVO_MID);
        RunI2C(CH_360, SERVO_MID);
    }

    Serial.println("　[終了]\n");
}

void RunI2C(int ch, int ticks){
    int base_reg = 0x06 + 4 * ch;
    mmp.i2c.write(PCA_ADDR, base_reg + 2, (ticks     ) & 0xFF);
    mmp.i2c.write(PCA_ADDR, base_reg + 3, (ticks >> 8) & 0x0F);
}

//============================================================
// 出力文字ヘルパ
//============================================================
static String bool2str(int source){return String(source ? "True" : "False");}
static String int2strOnOff(int source){return String(source == 0 ? "ON" : "OFF");}

//============================================================
// 互換シム（MMP.通信接続 / MMP.接続 を既存通り使えるように）
//============================================================
static String 引数取得(){
    static String spec;

    // 1) シリアル直結（自動）
    if (USE_SERIAL_AUTO){ return "auto"; }

    // 2) TCPブリッジ
    if (USE_TCP){
        spec =
            String("tcp://") + TCP_HOST + ":" + String(TCP_PORT)
            + "?io="     + String(IO_MS)
            + "&verify=" + String(VERIFY_MS);
        return spec;
    }

    // 3) usb4a (pydroid専用)
    // ※未使用
    if (USE_USB4A_DIRECT){
        spec = String("usb4a://") + String(USB4A_INDEX);
        return spec;
    }

    // 何も選ばれていなければ既定はシリアル自動
    return "auto";
}

//============================================================
// 互換シム利用し、接続を開始
//============================================================
static bool 接続開始(int mode){

    Serial.begin(115200);
    unsigned long deadline = millis() + 15000;
    while (!Serial && millis() < deadline) delay(10);
    delay(50);

    bool resp = false;

    //──────────
    // 0) アプリ設定：一括
    //──────────
    if (mode == 0){
        // Wifi接続(一括)
        resp = MMP::通信接続(
            引数取得()  ,   // 互換シムを利用
            mmp         ,   // クライアント
            WIFI_SSID   ,   // ★アプリ指定：SSID
            WIFI_PASS   ,   // ★アプリ指定：パスワード
            &Serial         // ログ用シリアル
            );
    }
    //──────────
    // 1) アプリ設定：個別
    //──────────
    if (mode == 1){
        // 一括接続
        if (!MMP::通信接続_WiFi(
            WIFI_SSID   ,   // ★アプリ指定：SSID
            WIFI_PASS   ,   // ★アプリ指定：パスワード
            &Serial     )   // ログ用シリアル
        ){return false;}

        // TCP接続
        resp = MMP::通信接続_Tcp(
            引数取得()  ,   // 互換シムを利用
            mmp         ,   // クライアント
            &Serial         // ログ用シリアル
            );
    }
    //──────────
    // 2) JSON設定：一括
    //──────────
    if (mode == 2){
        // 一括接続
        resp = MMP::通信接続(
            引数取得()  ,   // 互換シムを利用
            mmp         ,   // クライアント
            nullptr     ,   // ★JSON利用：SSID
            nullptr     ,   // ★JSON利用：パスワード
            &Serial         // ログ用シリアル
            );
    }
    //──────────
    // 3) JSON設定：個別
    //──────────
    if (mode == 3){
        // wifi接続(外部取込)
        if (!MMP::通信接続_WiFi_外部(&Serial)){return false;}

        // TCP接続
        resp = MMP::通信接続_Tcp(
            引数取得()  ,   // 互換シムを利用
            mmp         ,   // クライアント
            &Serial         // ログ用シリアル
            );
    }

    if (resp) MMP::状況表示(mmp,&Serial);
    return resp;
}

//============================================================
// メイン
//============================================================
void setup() {

    // 接続を開始する
    // 0 = アプリ設定：一括
    // 1 = アプリ設定：個別
    // 2 = JSON設定：一括
    // 3 = JSON設定：個別
    if (!接続開始(1)) return;

    Serial.println("\n＝＝＝ ＭＭＰ ＡＰＩテスト［開始］＝＝＝\n");
    // 実施するテストだけコメントを外してください（複数可）
    //RunAnalog()       ; // アナログ入力
    //RunDigital()      ; // デジタル入出力
    //RunMp3Playlist()  ; // MP3プレイヤー(基本)
    //RunMp3Control()   ; // MP3プレイヤー(制御)
    //RunPwm(true)      ; // PWM出力
    //RunPwm(false)     ; // I2C→PCA9685 直接制御
    Serial.println("＝＝＝ ＭＭＰ ＡＰＩテスト［終了］＝＝＝");

    mmp.Close();
}

void loop() {
  webui_handle();  // HTTPリクエスト処理を回す（Web UI のリスナー）
  delay(1);        // CPU占有を避ける最低限のウェイ/ト
}