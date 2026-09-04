// filename : modeWebAPI.cpp

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

  Serial.println("---------- [WebAPI] BEGIN() ----------");
  String errMSG = "";
  if (WiFi.status() != WL_CONNECTED) errMSG = "[NG] No WiFi Service";
  if (errMSG != "") {Serial.println(errMSG); delay(100); return "[NG] Not ready";}

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

  Serial.println("---------- [WebAPI] RUN() ----------");
  Serial.printf (" command : %s\n", cmdStr);

  String errMSG = "";
  if (WiFi.status() != WL_CONNECTED) errMSG = "[NG] No WiFi Service";
  if (errMSG != "") {Serial.println(errMSG); delay(100); return "[NG] Not Ready.";}

  // 前処理
  String strURL = String("http://") + SRV_IP + ":" + CONN_PORT + "/" + cmdStr;

  // ＭＭＰへリクエスト
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
      Serial.println("[OK] Successfully");
      if (strRX.containsKey("source")) {
        retMSG = strRX["source"].as<String>();
      } else if (strRX.containsKey("text")) {
        retMSG = strRX["text"].as<String>();
      } else {
        retMSG = payload; 
      }
    } else {
      Serial.println("[FAIL] Parse JSON");
      retMSG = "ERR";
    }

  } else {
    Serial.printf("[FAIL] HTTP Error: %s\n", CONN.errorToString(intResCD).c_str());
    retMSG = "FAIL";
  }

  CONN.end(); // WEB APIは必ず閉じる

  return retMSG;
} /* RUN() */

} /* namespace modeWebAPI */