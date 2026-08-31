#pragma once
#include <Wire.h>
#include "mode.h"

namespace modeIIC {
//=====================================================
// 基本情報
//=====================================================
  bool    IS_CONNECT = false; // 接続状況
  uint8_t CONN              ; // I2C Masterアドレス

//========================================================
// 設定
//========================================================
constexpr size_t  RX_SIZE  = 128 ; // 最大データ長

//========================================================
// リクエスト管理
//========================================================
volatile bool IS_SEND = false; // リクエスト中フラグ
String        CMD_PATH       ; // コマンドパス

//========================================================
// レスポンス受信用バッファ
//========================================================
char          STR_RX[RX_SIZE] = {0}  ; // 受信バッファ
volatile bool IS_FRAME        = false; // フレームの作成状況


//========================================================
// Ａ．Ｉ２Ｃコールバック
//========================================================
//────────────
// MMPからREADされたとき
//────────────
void OnRequest() {
  if (IS_SEND) {Wire.write((uint8_t*)"!", 1); return;}
Serial.println(" command send");
  Wire.write((const uint8_t*)CMD_PATH.c_str(), strlen(CMD_PATH.c_str()));
  IS_SEND = true;
} /* OnRequest() */

//────────────
// MMPからWRITEされたとき
//────────────
void OnReceive(int len) {
  int id = 0;
  while (Wire.available() && id < RX_SIZE - 1) STR_RX[id++] = Wire.read();
  STR_RX[id] = '\0'; // 末尾処理
Serial.println(String(STR_RX));
  if (String(STR_RX) != "####!") IS_FRAME = true; // レスポンス待ち
} /* OnReceive() */


//=====================================================
// 接続する
//=====================================================
bool BEGIN(uint8_t argADDR) {

   Serial.println("---------- [IIC] BEGIN() ----------");
  Serial.printf  (" address : %s\n", String(argADDR));

  //○I2C Slaveとして開始
  Wire.begin(argADDR);
  //│
  //○I2Cコールバックを登録
  Wire.onRequest(OnRequest);
  Wire.onReceive(OnReceive);
  //│
  //○正常終了
  Serial.println("[OK] Successfully");
  IS_CONNECT = true;
  return       true;
}

//=====================================================
// 切断する
//=====================================================
bool END() {
  Wire.end();
  IS_CONNECT = false;
  Serial.println("IIC Disconnected");
  return IS_CONNECT;
}

//=====================================================
// コマンドを実行する
//=====================================================
String RUN(
  const    char* cmdStr,
  unsigned long  argTimeoutMs
) {
  Serial.println("---------- [IIC] RUN() ----------");
  Serial.printf (" command : %s\n", cmdStr);

  // エラーチェック
  String errMSG = "";
  if (!IS_CONNECT ) errMSG = "[NG] No Callback";
  if (errMSG != "") {Serial.println(errMSG); delay(100); return "[NG] Not Ready.";}

  // 前処理
  CMD_PATH = cmdStr;
  IS_SEND  = false; // リクエスト済フラグをOFF(未送信)
  IS_FRAME = false; // フレーム作成状況をOFF(未完成)

  // 割り込み競合を防ぎつつ受信フラグをリセット
  noInterrupts();
  IS_FRAME = false;
  interrupts();

  // レスポンスを受信
  unsigned long startMs = millis();
  while (true) {

    // 割り込み側からフラグが立っているか確認
    bool receivedCopy = false;
    noInterrupts();
    if (IS_FRAME) {
      IS_FRAME = false; // フラグをクリア
      receivedCopy = true;
    }
    interrupts();

    if (receivedCopy) break;

    if (millis() - startMs > argTimeoutMs) {
      errMSG = "[FAIL] Timeout";  
      Serial.println(errMSG);
      return errMSG;
    }
    delay(1);
  }

  // 正常終了
  return String(STR_RX);

} /* runTCP() */

} /* namespace adpI2C */