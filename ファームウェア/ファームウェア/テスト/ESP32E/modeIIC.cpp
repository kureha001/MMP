#pragma once
#include "mode.h"

namespace modeIIC {
//=====================================================
// 基本情報
//=====================================================
  bool    ENABLED = false;
  uint8_t CONN; // I2C Masterアドレス

//=====================================================
// 接続する
//=====================================================
bool INIT() {
  ENABLED = true;
  return true;
}

//=====================================================
// コマンドを実行する
//=====================================================
String RUN(const char* cmdStr) {
return ("-----");

    // 接続済か確認
  if (!ENABLED) INIT();

  // 前処理
  String strRX = "";

  // コマンドをリクエストする
  Serial.printf("Sending command (IIC): %s\n", cmdStr);
//  tcpClient.print(cmdStr);

  // レスポンスを取得する

  // レスポンス値を返却する
  return (strRX);

} /* runTCP() */

} /* namespace adpI2C */