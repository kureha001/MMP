#pragma once
#include "mode.h"

namespace modeTCP {
//=====================================================
// 基本情報
//=====================================================
  bool       IS_CONNECT = false; // 接続状況
  WiFiClient CONN              ; // クライアント(接続資源)
  uint16_t   CONN_PORT  = 0    ; // ポート番号

//=====================================================
//=====================================================
bool BEGIN(uint16_t argPort) {
  if (WiFi.status() != WL_CONNECTED) return false;

  CONN_PORT = argPort;   
  CONN.setTimeout(2000);

  if (CONN.connect(SRV_IP, CONN_PORT)) {
    Serial.println("TCP connected successfully.");
    IS_CONNECT = true;

  } else {
    Serial.println("TCP connection failed.");
    IS_CONNECT = false;
  }
  return IS_CONNECT;
}

//=====================================================
// 切断する
//=====================================================
bool END() {
  CONN.stop();
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
  // WiFi接続を確認する
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    delay(100);
    return "FAIL#0";
  }

  // TCP接続を確認する
  if (!IS_CONNECT || !CONN.connected()) {
    Serial.println("TCP RAW not connected.");
    delay(100);
    return "FAIL#1";
  }

  // コマンドをリクエストする
  Serial.printf("Sending command (TCP RAW): %s\n", cmdStr);
  CONN.print(cmdStr);

  // 前処理
  uint8_t       strRX[6] = {0};
  unsigned long startMs  = millis();
  int           cntChar  = 0;

  // レスポンスを取得する
  while (cntChar < 5 && (millis() - startMs) < argTimeoutMs) {
    while (CONN.available()) strRX[cntChar++] = CONN.read();
    delay(10);
  } /* END-while */

  // レスポンス値を返却する
  strRX[cntChar] = '\0';
  return ((char*)strRX);

} /* runTCP() */

} /* namespace adpI2C */