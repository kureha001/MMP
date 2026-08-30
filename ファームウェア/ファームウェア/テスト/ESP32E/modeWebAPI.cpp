#pragma once
#include "mode.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace modeWebAPI {
 //=====================================================
// 基本情報
//=====================================================
  bool       IS_CONNECT = false; // 接続状況
  HTTPClient CONN              ; // クライアント
  uint16_t   CONN_PORT  = 0    ; // ポート番号

//=====================================================
// 接続する
//=====================================================
bool BEGIN(uint16_t argPort) {
  if (WiFi.status() != WL_CONNECTED) return false;
  CONN_PORT = argPort;   
  return false;
}

//=====================================================
// 切断する
//=====================================================
bool END() {return false;}

//=====================================================
// コマンドを実行する
//=====================================================
String RUN(const char* cmdStr) {

  // WiFi接続を確認する
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    delay(100);
    return "FAIL#0";
  }

  // コマンドをリクエストする
  String strURL = String("http://") + SRV_IP + ":" + CONN_PORT + "/" + cmdStr;
  Serial.printf("Sending command (WEB API): %s\n", strURL.c_str());
  CONN.begin(strURL)       ; // クライアントを開始
  int intResCD = CONN.GET(); // GETメソッドでリクエスト

  // レスポンスを取得する
  String retMSG = "FAIL";
  if (intResCD > 0) {
    String payload = CONN.getString();

    // サーバから返却されたJSONから "source" メンバーの値を抽出する
    StaticJsonDocument<256> strRX; 
    DeserializationError error = deserializeJson(strRX, payload);
    
    if (!error) {
      if (strRX.containsKey("source")) {
        retMSG = strRX["source"].as<String>();
      } else if (strRX.containsKey("text")) {
        retMSG = strRX["text"].as<String>();
      } else {
        retMSG = payload; 
      }
    } else {
      Serial.println("Failed to parse JSON response.");
      retMSG = "ERR";
    }

  } else {
    Serial.printf("HTTP Error: %s\n", CONN.errorToString(intResCD).c_str());
    retMSG = "FAIL";
  }

  CONN.end(); // WEB APIは必ず閉じる

  return retMSG;
} /* RUN() */

} /* namespace modeWebAPI */