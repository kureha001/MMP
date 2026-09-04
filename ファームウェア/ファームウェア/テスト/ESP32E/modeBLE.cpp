// filename : modeBLE.cpp

#pragma once
#include "mode.h"
#include <BLERemoteCharacteristic.h>

extern BLEClient*  BLE_CLIENT; // 初期化済みクライアント

namespace modeBLE {
//=====================================================
// 基本情報
//=====================================================
  bool   IS_CONNECT = false; // 接続状況
  static BLERemoteCharacteristic* CONN_RX = nullptr; // 書き込み用キャラクタリスティック
  static BLERemoteCharacteristic* CONN_TX = nullptr; // 通知用キャラクタリスティック

  // 標準的なUARTサービスのUUID
  static BLEUUID UUID   ("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
  static BLEUUID UUID_RX("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
  static BLEUUID UUID_TX("6e400003-b5a3-f393-e0a9-e50e24dcca9e");

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
bool BEGIN() {

  Serial.println("---------- [BLE] BEGIN() ----------");
  String errMSG = "";
  if      (BLE_CLIENT == nullptr     ) errMSG = "[NG] No Client (null)";
  else if (!BLE_CLIENT->isConnected()) errMSG = "[NG] No Client (disconect)";
  else if (CONN_RX != nullptr && CONN_TX != nullptr) errMSG = "[EXIST] Already Exists Rx,Tx";
  if (errMSG != "") {Serial.println(errMSG); delay(100); return "[NG] Not ready";}

  // サービスとキャラクタリスティックの取得
  BLERemoteService* pService = BLE_CLIENT->getService(UUID);
  if (pService == nullptr) {
    Serial.println("[FAIL] Not Found UUID (service)");
    return false;
  }

  // キャラクタリスティックの取得
  CONN_RX = pService->getCharacteristic(UUID_RX);
  CONN_TX = pService->getCharacteristic(UUID_TX);
  if (CONN_RX == nullptr || CONN_TX == nullptr) {
    Serial.println("[FAIL] Not Found UUIDs (Rx|Tx)");
    return false;
  }

  // コールバックを登録
  if (CONN_TX->canNotify()) CONN_TX->registerForNotify(notifyCallback);

  // 正常終了
  Serial.println("[OK] Successfully");
  IS_CONNECT = true;
  return       true;
}

//=====================================================
// 切断する
//=====================================================
bool END() {
  // コールバックを破棄
  if (CONN_TX != nullptr) CONN_TX->registerForNotify(notifyCallback);

  // キャラクタリスティックを破棄
  CONN_RX = nullptr;
  CONN_TX = nullptr;

  // 正常終了
  IS_CONNECT = false;
  Serial.println("BLE Disconnected");
  return IS_CONNECT;
}

//=====================================================
// コマンドを実行する
//=====================================================
String RUN(
  const    char* cmdStr,
  unsigned long  argTimeoutMs
) {
  Serial.println("---------- [BLE] RUN() ----------");
  Serial.printf (" command : %s\n", cmdStr);

  String errMSG = "";
  if      (!IS_CONNECT       ) errMSG = "[NG] No Callback";
  else if (CONN_RX == nullptr) errMSG = "[NG] No Characteristic (Rx)";
  else if (CONN_TX == nullptr) errMSG = "[NG] No Characteristic (Tx)";
  if (errMSG != "") {Serial.println(errMSG); delay(100); return "[NG] Not Ready.";}

  // 前処理
  STR_RX   = ""   ; // 受信バッファ
  IS_FRAME = false; // フレーム作成状況

  // ＭＭＰへリクエスト
  String sendData = String(cmdStr);
  if (!sendData.endsWith("!")) sendData += "!";
  CONN_RX->writeValue(sendData.c_str(), sendData.length());

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
  return STR_RX;

} /* RUN() */

} /* namespace modeBLE */