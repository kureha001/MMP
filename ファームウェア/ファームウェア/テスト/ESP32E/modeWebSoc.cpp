// filename : modeWebSoc.cpp

#pragma once
#include "mode.h"
#include <WebSocketsClient.h> // WebSockets by Markus Sattler

namespace modeWebSoc {
//=====================================================
// 基本情報
//=====================================================
  bool             IS_CONNECT = false; // 接続状況
  WebSocketsClient CONN              ; // クライアント(接続資源)
  uint16_t         CONN_PORT  = 0    ; // ポート番号


//=====================================================
// イベントコールバック
//=====================================================
  //--------------------------------------------------
  // ワーク変数
  //--------------------------------------------------
  String STR_RX   = ""   ; // 受信バッファ
  bool   IS_FRAME = false; // フレームの作成状況

  //--------------------------------------------------
  // コールバック関数
  //--------------------------------------------------
  void OnReceive(WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
      case WStype_TEXT:
        if (payload != nullptr && length > 0) {
          STR_RX   = String((char*)payload);
          IS_FRAME = true;
        }
        break;
      case WStype_DISCONNECTED: IS_CONNECT = false; break;
      case WStype_CONNECTED   : IS_CONNECT = true ; break;
      default                 : break;
    }
  }


//=====================================================
// 接続する
//=====================================================
bool BEGIN(uint16_t argPort) {

  Serial.println("---------- [WebSocket] BEGIN() ----------");
  String errMSG = "";
  if (WiFi.status() != WL_CONNECTED) errMSG = "[NG] No WiFi Service";
  if (errMSG != "") {Serial.println(errMSG); delay(100); return "[NG] Not ready";}

  // サービスの取得
  CONN_PORT = argPort;   
  CONN.begin(SRV_IP, CONN_PORT, "/");

  // コールバックを登録
  CONN.onEvent(OnReceive);  

  // 正常終了
  Serial.printf("[OK] Successfully : port[%d]\n", CONN_PORT);
  IS_CONNECT = true;
  return       true;
}

//=====================================================
// 切断する
//=====================================================
bool END() {
  CONN.disconnect();
  IS_CONNECT = false;
  Serial.println("WEB Socket Disconnected");
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
  if (errMSG != "") {Serial.println(errMSG); delay(100); return "[NG] Not Ready.";}

  // 前処理
  STR_RX   = ""   ; // 受信バッファ
  IS_FRAME = false; // フレームの作成状況を判定

  // ＭＭＰへリクエスト
  CONN.sendTXT(cmdStr);

  //レスポンスを取得する（イベント駆動の完了を待つ）
  unsigned long startMs = millis();
  while (!IS_FRAME) {
    if (millis() - startMs > argTimeoutMs) {
      errMSG = "[FAIL] Timeout";  
      Serial.println(errMSG);
      return errMSG;
    }
    CONN.loop();
    delay(5);
  }

  // 正常終了
  return STR_RX;

} /* runTCP() */

} /* namespace modeWebSoc */