#pragma once
#include <Wire.h>
#include "mode.h"

namespace modeIIC {
//=====================================================
// 基本情報
//=====================================================
  bool    ENABLED = false;
  uint8_t CONN; // I2C Masterアドレス

//========================================================
// 設定
//========================================================
const     uint8_t I2C_ADDR = 0xA0; // I2C Slaveアドレス
constexpr size_t  RX_SIZE  = 128 ; // 最大データ長

//========================================================
// リクエスト管理
//========================================================
volatile bool IsSend = false;
String COMMAND_PATH;

//========================================================
// レスポンス受信用バッファ
//========================================================
char RES_MSG[RX_SIZE] = {0};
volatile bool IsReceived = false;


//========================================================
// Ａ．Ｉ２Ｃコールバック
//========================================================
//────────────
// MMPからREADされたとき
//────────────
void OnRequest() {
if (IsSend) {Wire.write((uint8_t*)"!", 1); return;}
  Wire.write((const uint8_t*)COMMAND_PATH.c_str(), strlen(COMMAND_PATH.c_str()));
  IsSend = true;
} /* OnRequest() */

//────────────
// MMPからWRITEされたとき
//────────────
void OnReceive(int len) {
  int id = 0;
  while (Wire.available() && id < RX_SIZE - 1) RES_MSG[id++] = Wire.read();
  RES_MSG[id] = '\0'; // 末尾処理
if (String(RES_MSG) != "####!") IsReceived = true; // レスポンス待ち
} /* OnReceive() */


//=====================================================
// 接続する
//=====================================================
bool BEGIN() {
  //○I2C Slaveとして開始
  Wire.begin(I2C_ADDR);
  //│
  //○I2Cコールバックを登録
  Wire.onRequest(OnRequest);
  Wire.onReceive(OnReceive);
  //│
  //○I2Cコールバックを登録
  ENABLED = true;
  return true;
}

//=====================================================
// 切断する
//=====================================================
bool END() {
  Wire.end();
  ENABLED = false;
}

//=====================================================
// コマンドを実行する
//=====================================================
String RUN(const char* cmdStr) {
return ("-----");

    // 接続済か確認
  if (!ENABLED) BEGIN();

  COMMAND_PATH = cmdStr;

  //┬
  //○┐MMPからWRITEされたレスポンスを確認
  while(true){
    //○┐MMPからWRITEされたレスポンスを確認
    if (IsReceived) {
        //│
        //○割り込み側から受信状態を引き継ぐ
        noInterrupts();
        IsReceived = false;
        interrupts();
        //┴
    } /* END-if */
    //│
    //○短い待機
    delay(1);
    //┴
  } /* END-while */
  
  // レスポンス値を返却する
  return (RES_MSG);

} /* runTCP() */

} /* namespace adpI2C */