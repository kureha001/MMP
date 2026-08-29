#pragma once
#include "mode.h"

namespace modeUART {
//=====================================================
// 基本情報
//=====================================================
  bool ENABLED = false;

//=====================================================
// 接続する
//=====================================================
bool INIT() {
  Serial.begin(115200);
  delay(500);
  ENABLED = true;
  return true;
}

//=====================================================
// コマンドを実行する
//=====================================================
String RUN(const char* cmdStr) {
 
  // 接続済か確認
  if (!ENABLED) INIT();

  // コマンドをリクエストする
  Serial.printf("Sending command (UART): %s\n", cmdStr);
  Serial1.print(cmdStr);

  // 前処理
  uint8_t       strRX[6] = {0};
  unsigned long startMs  = millis();
  int           cntChar  = 0;

  // レスポンスを取得する
  while (cntChar < 5 && (millis() - startMs) < 2000) {
    strRX[cntChar++] = Serial1.read();
    delay(10);
  } /* END-while */

  // レスポンス値を返却する
  strRX[cntChar] = '\0';
  return ((char*)strRX);

} /* runUART() */

} /* namespace adpI2C */