#pragma once
#include "mode.h"
#include <WiFi.h>
#include <esp_now.h>

namespace modeEspNow {
//=====================================================
// 基本情報
//=====================================================
  bool IS_CONNECT = false; // 接続状況

//=====================================================
// イベントコールバック用ワーク変数
//=====================================================
  static          char   STR_RX[32]    = {0}  ; // 受信バッファ
  static volatile bool   IS_FRAME      = false; // フレームの作成状況
  static          uint8_t targetMac[6] = {0}  ;
  static volatile esp_now_send_status_t lastSendStatus = ESP_NOW_SEND_FAIL;

  //--------------------------------------------------
  // コールバック関数 (ESP-IDF v3.x対応シグネチャ)
  //--------------------------------------------------
  void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    lastSendStatus = status;
  }

  void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
    int copyLen = (len < (int)sizeof(STR_RX) - 1) ? len : (int)sizeof(STR_RX) - 1;
    memset(STR_RX, 0, sizeof(STR_RX));
    memcpy(STR_RX, incomingData, copyLen);
    STR_RX[copyLen] = '\0';
    IS_FRAME = true; // bool変数のフラグに正しい値を代入
  }

//=====================================================
// 接続する
//=====================================================
bool BEGIN(const uint8_t argMAC[]) {
  Serial.println("---------- [ESP NOW] BEGIN() ----------");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[NG] No WiFi (Station mode required)");
    return false;
  }

  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  esp_err_t initResult = esp_now_init();
  if (initResult != ESP_OK && initResult != ESP_ERR_ESPNOW_EXIST) {
    Serial.println("[FAIL] Initialization");
    return false;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  memcpy(targetMac, argMAC, 6);
  if (esp_now_is_peer_exist(targetMac)) {
    esp_now_del_peer(targetMac);
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, targetMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[FAIL] Failed to add peer");
    return false;
  }

  IS_CONNECT = true;
  return IS_CONNECT;
}

//=====================================================
// 切断する
//=====================================================
bool END() {
  if (esp_now_is_peer_exist(targetMac)) esp_now_del_peer(targetMac);
  esp_now_deinit();
  IS_CONNECT = false;
  Serial.println("ESP-NOW Disconnected");
  return true;
}

//=====================================================
// コマンドを実行する
//=====================================================
String RUN(const char* cmdStr, unsigned long argTimeoutMs) {

  Serial.println("---------- [ESP-NOW RUN() ----------");
  Serial.printf (" command : %s\n", cmdStr);
  String errMSG = "";
  if      (!IS_CONNECT        ) errMSG = "[NG] No Callback";
  else if (WiFi.status() != WL_CONNECTED) errMSG = "[NG] WiFi not connected";
  if (errMSG != "") {Serial.println(errMSG); delay(100); return "[NG] Not Ready.";}

  // 前処理
  memset(STR_RX, 0, sizeof(STR_RX)); // 受信バッファ
  IS_FRAME = false                 ; // フレーム作成状況

  // ＭＭＰへリクエスト
  String sendData = String(cmdStr);
  if (!sendData.endsWith("!")) sendData += "!";
  esp_err_t result = esp_now_send(targetMac, (uint8_t*)sendData.c_str(), sendData.length());
  if (result != ESP_OK) {
    errMSG = "[FAIL] Send Repuest";  
    Serial.println(errMSG);
    return errMSG;
    return "FAIL";
  }

  // レスポンスを受信
  unsigned long startMs = millis();
  while (!IS_FRAME) {
    if (millis() - startMs > argTimeoutMs) {
      errMSG = "[FAIL] Timeout";  
      Serial.println(errMSG);
      return errMSG;
    }
    delay(5);
  }

  // 正常終了
  return String(STR_RX);

} /* RUN() */

} /* namespace modeEspNow */