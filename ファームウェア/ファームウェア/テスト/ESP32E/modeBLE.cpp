#pragma once
#include "mode.h"

namespace modeBLE {
//=====================================================
// 基本情報
//=====================================================
  bool    IS_CONNECTED = false;
  uint8_t CONN; // アクセス資源(クライアント番号)

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

  // 前処理
  String strRX = "";

  // コマンドをリクエストする
  Serial.printf("Sending command (BLE): %s\n", cmdStr);
//  tcpClient.print(cmdStr);

  // レスポンスを取得する

  // レスポンス値を返却する
  return (strRX);

} /* runTCP() */

} /* namespace adpI2C */