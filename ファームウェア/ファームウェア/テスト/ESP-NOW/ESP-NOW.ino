#pragma once
#include "mode.h"
#include <WiFi.h>
#include <esp_now.h>

namespace modeEspNow {
  bool IS_CONNECT = false;

  static char STR_RX[32] = {0};
  static volatile bool responseReceived = false;
  static volatile esp_now_send_status_t lastSendStatus = ESP_NOW_SEND_FAIL;
  static uint8_t targetMac[6] = {0};

  // 送信完了コールバック（ESP-IDF v3.x用シグネチャ）
  void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    lastSendStatus = status;
  }

  // データ受信コールバック
  void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
    int copyLen = (len < (int)sizeof(STR_RX) - 1) ? len : (int)sizeof(STR_RX) - 1;
    memset(STR_RX, 0, sizeof(STR_RX));
    memcpy(STR_RX, incomingData, copyLen);
    STR_RX[copyLen] = '\0';
    responseReceived = true;
  }

bool BEGIN(const uint8_t argMAC[]) {
  Serial.println("---------- [ESP NOW] BEGIN() ----------");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[NG] No WiFi (Station mode required)");
    return false;
  }

  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  // esp_now_is_init() は存在しないため、esp_now_init() の戻り値で判定
  if (esp_now_init() != ESP_OK) {
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

bool END() {
  if (esp_now_is_peer_exist(targetMac)) {
    esp_now_del_peer(targetMac);
  }
  esp_now_deinit();
  IS_CONNECT = false;
  return true;
}

String RUN(const char* cmdStr, unsigned long argTimeoutMs) {
  if (!IS_CONNECT) return "FAIL";

  responseReceived = false;
  memset(STR_RX, 0, sizeof(STR_RX));

  String sendData = String(cmdStr);
  if (!sendData.endsWith("!")) sendData += "!";

  esp_err_t result = esp_now_send(targetMac, (uint8_t*)sendData.c_str(), sendData.length());

  if (result != ESP_OK) {
    return "FAIL";
  } else {
    unsigned long waitStart = millis();
    while (!responseReceived && (millis() - waitStart < argTimeoutMs)) {
      delay(5);
    }
    if (responseReceived) {
      return String(STR_RX);
    } else {
      return "TIME";
    }
  }
}

} /* namespace modeEspNow */