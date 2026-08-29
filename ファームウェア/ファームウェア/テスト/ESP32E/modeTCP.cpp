#pragma once
#include "mode.h"

namespace modeTCP {
//=====================================================
// 基本情報
//=====================================================
  bool ENABLED = false;
  WiFiClient tcpClient;
  const char*    SRV_IP   = "192.168.2.99"; // IPアドレス
  const uint16_t SRV_PORT = 8081          ; // ポート番号

//=====================================================
// 接続する
//=====================================================
bool INIT() {
  if (WiFi.status() != WL_CONNECTED) return false;
    
  Serial.printf("Connecting to TCP server %s:%d...\n", SRV_IP, SRV_PORT);
  tcpClient.setTimeout(2000);
  if (tcpClient.connect(SRV_IP, SRV_PORT)) {
    ENABLED = true;
    Serial.println("TCP connected successfully.");
    return true;
  } else {
    ENABLED = false;
    Serial.println("TCP connection failed.");
    return false;
  }
}

//=====================================================
// 切断する
//=====================================================
void DISCONNECT() {
  tcpClient.stop();
  ENABLED = false;
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
  if (!ENABLED || !tcpClient.connected()) INIT();
  if (!ENABLED || !tcpClient.connected()) return "FAIL";

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