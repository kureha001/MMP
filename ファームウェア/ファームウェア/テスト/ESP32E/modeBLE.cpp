#pragma once
#include "mode.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <BLERemoteCharacteristic.h>

namespace modeBLE {
//=====================================================
// 基本情報
//=====================================================
  bool                            IS_CONNECT = false  ; // 接続状況
  static BLEClient*               CONN       = nullptr; // クライアント
  static BLERemoteCharacteristic* CONN_RX    = nullptr; // 書き込み用キャラクタリスティック
  static BLERemoteCharacteristic* CONN_TX    = nullptr; // 通知用キャラクタリスティック
  String                          CONN_DEV   = ""     ; // 接続先デバイス名

  // 標準的なUARTサービスのUUID
  static BLEUUID serviceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
  static BLEUUID charRxUUID ("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
  static BLEUUID charTxUUID ("6e400003-b5a3-f393-e0a9-e50e24dcca9e");

//=====================================================
// イベントコールバック
//=====================================================
  //--------------------------------------------------
  // ワーク変数
  //--------------------------------------------------
  static String STR_RX   = ""   ; // 受信バッファ
  static bool   IS_FRAME = false; // フレームの作成状況

  //--------------------------------------------------
  // コールバック関数
  //--------------------------------------------------
static void notifyCallback(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData   ,
    size_t   length  ,
    bool     isNotify
  ) {
    if (length < 1) return;
    STR_RX   = String((char*)pData).substring(0, length);
    IS_FRAME = true;
  }

//=====================================================
// 接続する
//=====================================================
bool BEGIN(String argDevName) {

  if (IS_CONNECT && CONN != nullptr && CONN->isConnected()) return true;

  CONN_DEV = argDevName;

if (!BLEDevice::getInitialized()) BLEDevice::init("");
  if (CONN == nullptr) CONN = BLEDevice::createClient();

  Serial.printf("Scanning for BLE device: %s...\n", CONN_DEV.c_str());

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  BLEScanResults* foundDevices = pBLEScan->start(5, false);
  BLEAdvertisedDevice* targetDevice = nullptr;

  for (int i = 0; i < foundDevices->getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices->getDevice(i);
    if (device.haveName() && device.getName() == CONN_DEV) {
      targetDevice = new BLEAdvertisedDevice(device);
      break;
    }
  }
  pBLEScan->clearResults();

  if (targetDevice == nullptr) {
    Serial.println("Target device not found.");
    IS_CONNECT = false;
    return false;
  }

  Serial.println("Connecting to BLE Server...");
  if (!CONN->connect(targetDevice)) {
    Serial.println("Failed to connect to server.");
    delete targetDevice;
    IS_CONNECT = false;
    return false;
  }
  delete targetDevice;
  Serial.println("Connected to server.");

  // サービスとキャラクタリスティックの取得
  BLERemoteService* pRemoteService = CONN->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find service UUID.");
    CONN->disconnect();
    IS_CONNECT = false;
    return false;
  }

  CONN_RX = pRemoteService->getCharacteristic(charRxUUID);
  CONN_TX = pRemoteService->getCharacteristic(charTxUUID);

  if (CONN_RX == nullptr || CONN_TX == nullptr) {
    Serial.println("Failed to find characteristic UUIDs.");
    CONN->disconnect();
    IS_CONNECT = false;
    return false;
  }

  if (CONN_TX->canNotify()) {
    CONN_TX->registerForNotify(notifyCallback);
  }

  Serial.println("BLE connected and ready successfully.");
  IS_CONNECT = true;
  return true;
}

//=====================================================
// 切断する
//=====================================================
bool END() {
  if (CONN != nullptr && CONN->isConnected()) CONN->disconnect();
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
  // BLE接続を確認する
  if (!IS_CONNECT) {
    Serial.println("BLE not connected.");
    delay(100);
    return "FAIL#1";
  }

  // 前処理
STR_RX   = "";
  IS_FRAME = false;

  Serial.printf("Sending command (BLE): %s\n", cmdStr);

  String sendData = String(cmdStr);
  if (!sendData.endsWith("!")) sendData += "!";

  if (CONN_RX != nullptr) {
    CONN_RX->writeValue(sendData.c_str(), sendData.length());
  } else {
    return "FAIL#2";
  }

  unsigned long startMs = millis();
  while (!IS_FRAME) {
    if (millis() - startMs > argTimeoutMs) {
      Serial.println("BLE Response Timeout.");
      break;
    }
    delay(5);
  }

  return IS_FRAME ? STR_RX : "FAIL#3";

} /* runTCP() */

} /* namespace modeBLE */