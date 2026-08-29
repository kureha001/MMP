#pragma once
#include "mode.h"

namespace modeWebSoc {
//=====================================================
// 基本情報
//=====================================================
  bool IS_CONNECTED = false;
  static int SRV_PORT = 8082;

//=====================================================
// 接続する
//=====================================================
bool BEGIN() {
  IS_CONNECTED = true;
  return true;
}

//=====================================================
// 切断する
//=====================================================
bool END() {
//  tcpClient.stop();
  IS_CONNECTED = false;
}

//=====================================================
// コマンドを実行する
//=====================================================
String RUN(const char* cmdStr) {
return ("-----");

  // 接続済か確認
  if (!IS_CONNECTED) BEGIN();

  // WiFi接続を確認する
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    delay(100);
    return "ERR";
  }

  // 前処理
  String strRX = "";

  // コマンドをリクエストする
  Serial.printf("Sending command (WEB Socket): %s\n", cmdStr);
//  tcpClient.print(cmdStr);

  // レスポンスを取得する

  // レスポンス値を返却する
  return (strRX);

} /* runTCP() */

} /* namespace adpI2C */