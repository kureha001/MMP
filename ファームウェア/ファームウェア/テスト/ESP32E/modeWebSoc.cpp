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

  if (WiFi.status() != WL_CONNECTED) return false;

  CONN_PORT = argPort;   
  CONN.begin(SRV_IP, CONN_PORT, "/");
  CONN.onEvent(OnReceive);
  
  unsigned long startMs = millis();
  while (!IS_CONNECT && (millis() - startMs) < 2000) {
    CONN.loop();
    delay(10);
  }

  return IS_CONNECT;
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

  // WiFi接続を確認する
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    delay(100);
    return "FAIL#0";
  }

  // WinSocket接続を確認する
  if (!IS_CONNECT) {
    Serial.println("WebSocket not connected.");
    delay(100);
    return "FAIL#1";
  }

  // 前処理
  STR_RX   = ""   ; // 受信バッファ
  IS_FRAME = false; // フレームの作成状況を判定

  //コマンドをリクエストする
  Serial.printf("Sending command (WEB Socket): %s\n", cmdStr);
  CONN.sendTXT(cmdStr);

  //レスポンスを取得する（イベント駆動の完了を待つ）
  unsigned long startMs = millis();
  while (!IS_FRAME && (millis() - startMs) < argTimeoutMs) {
    CONN.loop();
    delay(5);
  }

  // レスポンス値を返却する
  return STR_RX;

} /* runTCP() */

} /* namespace modeWebSoc */