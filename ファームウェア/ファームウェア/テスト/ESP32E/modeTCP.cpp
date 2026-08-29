#pragma once
#include "mode.h"

namespace modeTCP {
//=====================================================
// 基本情報
//=====================================================
  bool CONNECT = false;
  WiFiClient tcpClient;
  const uint16_t SRV_PORT = 8081          ; // ポート番号

//=====================================================
// 接続する
//=====================================================
bool BEGIN() {
  if (WiFi.status() != WL_CONNECTED) return false;
    
  tcpClient.setTimeout(2000);

  if (tcpClient.connect(SRV_IP, SRV_PORT)) {
    Serial.println("TCP connected successfully.");
    CONNECT = true;

  } else {
    Serial.println("TCP connection failed.");
    CONNECT = false;
  }
  return CONNECT;
}

//=====================================================
// 切断する
//=====================================================
bool END() {
  tcpClient.stop();
  CONNECT = false;
}

//=====================================================
// コマンドを実行する
//=====================================================
String RUN(const char* cmdStr) {

  // WiFi接続を確認する
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    delay(100);
    return "ERR";
  }

  // 接続済か確認
  if (!CONNECT || !tcpClient.connected()) BEGIN();
  if (!CONNECT || !tcpClient.connected()) return "FAIL";

  // コマンドをリクエストする
  Serial.printf("Sending command (TCP RAW): %s\n", cmdStr);
  tcpClient.print(cmdStr);

  // 前処理
  uint8_t       strRX[6] = {0};
  unsigned long startMs  = millis();
  int           cntChar  = 0;

  // レスポンスを取得する
  while (cntChar < 5 && (millis() - startMs) < 2000) {
    while (tcpClient.available()) strRX[cntChar++] = tcpClient.read();
    delay(10);
  } /* END-while */

  // レスポンス値を返却する
  strRX[cntChar] = '\0';
  return ((char*)strRX);

} /* runTCP() */

} /* namespace adpI2C */