#pragma once
#include "mode.h"

namespace modeUART {
//=====================================================
// 基本情報
//=====================================================
  bool IS_CONNECT = false;

//=====================================================
// 接続する
//=====================================================
bool BEGIN() {
  Serial1.begin(115200);
  delay(500);
  IS_CONNECT = true;
  return true;
}

//=====================================================
// 切断する
//=====================================================
bool END() {
  Serial1.end();
  IS_CONNECT = false;
  return false;
}

//=====================================================
// コマンドを実行する
//=====================================================
String RUN(
  const    char* cmdStr,
  unsigned long  argTimeoutMs
) {
  // 接続済か確認
  if (!IS_CONNECT) BEGIN();

  // コマンドをリクエストする
  Serial.printf("Sending command (UART): %s\n", cmdStr);
  Serial1.print(cmdStr);

  // 前処理
  uint8_t       strRX[6] = {0};
  unsigned long startMs  = millis();
  int           cntChar  = 0;

  // レスポンスを取得する
  while (cntChar < 5 && (millis() - startMs) < argTimeoutMs) {
    strRX[cntChar++] = Serial1.read();
    delay(10);
  } /* END-while */

  // レスポンス値を返却する
  strRX[cntChar] = '\0';
  return ((char*)strRX);

} /* runUART() */

} /* namespace adpI2C */