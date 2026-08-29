#pragma once
#include "mode.h"

namespace modeWebSoc {
//=====================================================
// 基本情報
//=====================================================
  bool ENABLED = false;
  const  char* SRV_IP   = "192.168.2.99"; // IPアドレス
  static int   SRV_PORT = 8082          ; // ポート番号

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