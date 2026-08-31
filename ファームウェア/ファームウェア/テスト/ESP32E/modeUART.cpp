#pragma once
#include "mode.h"

namespace modeUART {
//=====================================================
// 基本情報
//=====================================================
  bool IS_CONNECT = false;
 HardwareSerial MySerial(1);
#define MY_TX_PIN 35  // 元のSCL
#define MY_RX_PIN 39  // 元のSDA

//=====================================================
// 接続する
//=====================================================
bool BEGIN(unsigned long argRate) {

  Serial.println("---------- [UART] BEGIN() ----------");

  // I2Cコネクタ側をUART1として初期化
  // ボーレート, 構成設定, RXピン, TXピン
  MySerial.begin(9600, SERIAL_8N1, MY_RX_PIN, MY_TX_PIN);

  delay(500);

  // 正常終了
  IS_CONNECT = true;
  return true;
}

//=====================================================
// 切断する
//=====================================================
bool END() {
  MySerial.end();
  IS_CONNECT = false;
  Serial.println("UART Disconnected");
  return IS_CONNECT;
}

//=====================================================
// コマンドを実行する
//=====================================================
String RUN(
  const    char* cmdStr,
  unsigned long  argTimeoutMs
) {
  Serial.println("---------- [UART] RUN() ----------");
  Serial.printf (" command : %s\n", cmdStr);

  String errMSG = "";
  if (!IS_CONNECT ) errMSG = "[NG] Initialization";
  if (errMSG != "") {Serial.println(errMSG); delay(100); return "[NG] Not Ready.";}

  // コマンドをリクエストする
  MySerial.print(cmdStr);

  // 前処理
  uint8_t       strRX[6] = {0};
  unsigned long startMs  = millis();
  int           cntChar  = 0;

  // レスポンスを取得する
  while (cntChar < 5 && (millis() - startMs) < argTimeoutMs) {
    strRX[cntChar++] = MySerial.read();
    delay(10);
  } /* END-while */

  // レスポンス値を返却する
  strRX[cntChar] = '\0';
  return ((char*)strRX);

} /* runUART() */

} /* namespace adpI2C */