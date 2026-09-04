// filename : modeTCP.cpp

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

  Serial.println("---------- [TCP] BEGIN() ----------");
  String errMSG = "";
  if (WiFi.status() != WL_CONNECTED) errMSG = "[NG] No WiFi Service";
  if (errMSG != "") {Serial.println(errMSG); delay(100); return "[NG] Not ready";}

  CONN_PORT = argPort;   
  CONN.setTimeout(2000);

  if (CONN.connect(SRV_IP, CONN_PORT)) {
    Serial.printf("[OK] Successfully : port[%d]\n", CONN_PORT);
    IS_CONNECT = true;

  } else {
    Serial.println("[FAIL] Connection");
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
  Serial.println("TCP Disconnected");
  return IS_CONNECT;
}

//=====================================================
// コマンドを実行する
//=====================================================
String RUN(
  const    char* cmdStr,
  unsigned long  argTimeoutMs
) {
  Serial.println("---------- [WebSocket] RUN() ----------");
  Serial.printf (" command : %s\n", cmdStr);

  String errMSG = "";
  if      (!IS_CONNECT                  ) errMSG = "[NG] No Callback";
  else if (WiFi.status() != WL_CONNECTED) errMSG = "[NG] No WiFi Service";
  else if (!CONN.connected()            ) errMSG = "[NG] No TCP Connection";
  if (errMSG != "") {Serial.println(errMSG); delay(100); return "[NG] Not Ready.";}

  // 前処理
  uint8_t       strRX[6] = {0};
  unsigned long startMs  = millis();
  int           cntChar  = 0;

  // ＭＭＰへリクエスト
  CONN.print(cmdStr);

  // レスポンスを受信
  while (cntChar < 5) {
    if (millis() - startMs > argTimeoutMs) {
      errMSG = "[FAIL] Timeout";  
      Serial.println(errMSG);
      return errMSG;
    }
    while (CONN.available()) strRX[cntChar++] = CONN.read();
    delay(10);
  } /* END-while */

  // 正常終了
  strRX[cntChar] = '\0';
  return ((char*)strRX);

} /* runTCP() */

} /* namespace adpI2C */