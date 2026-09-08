// filename : Dep_Connect/adapter/bridge/ESP_NOW.cpp
//========================================================
// 接続部門／業務課／ブリッジ係：ESP-NOW 担当
//--------------------------------------------------------
// Ver 1.3.1 (2026/09/07) 
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <WiFi.h>
  #include <esp_now.h>
//┴┴

//########################################################
//# 処理詳細
//########################################################
namespace brdESPN {
//=====================================================
// 基本情報
//=====================================================
  bool IS_CONNECT = false; // 接続状況

//=====================================================
// ワーク変数
//=====================================================
  static uint8_t mmpMAC[6] = {0};
  static volatile esp_now_send_status_t lastSendStatus = ESP_NOW_SEND_FAIL;

  //--------------------------------------------------
  // 送信完了コールバック関数
  //--------------------------------------------------
  void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    lastSendStatus = status;
  }

  //--------------------------------------------------
  // MAC文字列 (XX-XX-XX-XX-XX-XX) デコード関数
  //--------------------------------------------------
  static bool parseMAC(const String& macStr, uint8_t macOut[6]) {
    int lastIndex = 0;
    for (int i = 0; i < 6; i++) {
      int index = macStr.indexOf('-', lastIndex);
      String byteStr;
      if (index == -1) {
        if (i == 5) {
          byteStr = macStr.substring(lastIndex);
        } else {
          return false; // フォーマット不正
        }
      } else {
        byteStr = macStr.substring(lastIndex, index);
        lastIndex = index + 1;
      }
      macOut[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
    }
    return true;
  }

//=====================================================
// 接続する（初期化・ピア登録）
//=====================================================
void BEGIN(String argMACStr) {

  // MACアドレス文字列のパース
  uint8_t targetMAC[6] = {0};
  if (!parseMAC(argMACStr, targetMAC)) {
    ctx.resMSG = "#BG3!"; // MACアドレス指定エラー
    return;
  }

  // Wi-Fiモードの確認（STAモードを有効化）
  if (WiFi.getMode() == WIFI_OFF) WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  esp_err_t initResult = esp_now_init();
  if (initResult != ESP_OK && initResult != ESP_ERR_ESPNOW_EXIST) {
    ctx.resMSG = "#BG1!";
    return;
  }

  // 送信完了コールバックのみ登録
  esp_now_register_send_cb(OnDataSent);

  // 送信先MACアドレスを保存し、ピア登録を行う
  memcpy(mmpMAC, targetMAC, 6);
  if (esp_now_is_peer_exist(mmpMAC)) esp_now_del_peer(mmpMAC);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mmpMAC, 6);
  peerInfo.channel = 0; // 現在のWi-Fiチャンネルを使用
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    ctx.resMSG = "#BG2!";
    return;
  }

  // 正常終了
  IS_CONNECT = true;
}

//=====================================================
// 切断する
//=====================================================
void END() {
  if (esp_now_is_peer_exist(mmpMAC)) esp_now_del_peer(mmpMAC);
  esp_now_deinit();
  IS_CONNECT = false;
}

//=====================================================
// データを送信する（送信専用・ノンブロッキング）
//=====================================================
void SEND() {
  if (!IS_CONNECT) {ctx.resMSG = "#SN1!"; return;}

  // 端末記号 '!' を付与
  String sendData = ctx.strFrame;
  if (!sendData.endsWith("!")) sendData += "!";

  // リクエスト送信（ヌル文字を含めない長さで送信）
  esp_err_t result = esp_now_send(
    mmpMAC,
    (const uint8_t*)sendData.c_str(),
    sendData.length()
  );

  if (result != ESP_OK) {ctx.resMSG = "#SN2!"; return;}

} /* SEND() */

} /* namespace brdESPN */