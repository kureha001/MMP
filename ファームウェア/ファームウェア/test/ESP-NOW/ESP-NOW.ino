// filename : test01.ino
//========================================================
// ＥＳＰ－ＮＯＷアダプタのテスト
//--------------------------------------------------------
// Ver 1.2.1 (2026/08/27) 
//========================================================
#include <WiFi.h>
#include <esp_now.h>

// --- 設定 ---
uint8_t MAC_ADDR[] = {0x50, 0x78, 0x7D, 0x18, 0x51, 0x50}; // MMPのMACアドレス
String SSID = "Buffalo-G-7050"; // 無線ルータのSSID
String PSWD = "etnxhurnecbs7" ; // 無線ルータのパスワード
int WAIT_MS = 500             ; // レスポンスのタイムアウト(msec)

// 送信するコマンドのリスト（5種類）
const char* commandList[] = {
  "SYS/VERSION!",
  "MP3/TRACK/PLAY:1:1:1!",
  "MP3/TRACK/STOP:1!",
  "DIGITAL/OUTPUT:17:1!",
  "DIGITAL/OUTPUT:17:0!"
};
const int totalCommands = sizeof(commandList) / sizeof(commandList[0]);
int currentCmdIndex = 0;

// タイマー管理用
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 3000; // 3秒おき

// 返信データ受信用の一時バッファ
char lastResponse[32] = {0};
volatile bool responseReceived = false;
volatile esp_now_send_status_t lastSendStatus = ESP_NOW_SEND_FAIL;

// --- 送信完了コールバック関数（最新仕様に対応） ---
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  lastSendStatus = status;
}

// --- データ受信コールバック関数（最新仕様に対応） ---
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  int copyLen = (len < sizeof(lastResponse) - 1) ? len : sizeof(lastResponse) - 1;
  memcpy(lastResponse, incomingData, copyLen);
  lastResponse[copyLen] = '\0';
  responseReceived = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1. Wi-Fiをステーションモードに設定し、ルーターに接続する
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected! Channel: " + String(WiFi.channel()));
  WiFi.disconnect();

  // 送信出力を最大に設定
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  // 2. ESP-NOWの初期化
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERROR] ESP-NOW Initialization Failed");
    return;
  }

  // 3. コールバック関数の登録
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // 4. 宛先ピア（Peer）の登録（既に登録されている場合は一度削除してクリーンアップ）
  if (esp_now_is_peer_exist(MAC_ADDR)) {
    esp_now_del_peer(MAC_ADDR);
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, MAC_ADDR, 6);
  peerInfo.channel = 0; // 現在のチャンネルを使用
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ERROR] Failed to add peer");
    return;
  }

  Serial.println("=== ESP-NOW Master Initialized ===");
  Serial.print("Target MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", MAC_ADDR[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println("\n----------------------------------");
}

void loop() {
  unsigned long currentMillis = millis();

  // 3秒おきにコマンドを送信
  if (currentMillis - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = currentMillis;
    responseReceived = false;
    memset(lastResponse, 0, sizeof(lastResponse));

    // 送信する文字列を取得
    const char* payload = commandList[currentCmdIndex];
    size_t len = strlen(payload);

    // 送信開始ログ
    Serial.print("[SEND] ");
    Serial.print(payload);
    Serial.print(" -> ");

    // ESP-NOWでデータを送信
    esp_err_t result = esp_now_send(MAC_ADDR, (uint8_t*)payload, len);

    if (result != ESP_OK) {
      Serial.println("[FAIL: Stack Error]");
    } else {
      // 送信結果(ACK)と返信を少し待つ（最大300ms）
      unsigned long waitStart = millis();
      while (!responseReceived && (millis() - waitStart < WAIT_MS)) {
        delay(5);
      }

      // ログの出力判定
      if (lastSendStatus == ESP_NOW_SEND_SUCCESS) {
        Serial.print("[ACK OK] ");
      } else {
        Serial.print("[ACK FAIL(No Radio)] ");
      }

      if (responseReceived) {
        Serial.print("==> Response: [");
        Serial.print(lastResponse);
        Serial.println("]");
      } else {
        Serial.println("==> Response: [Timeout / No Reply]");
      }
    }

    // 次のコマンドインデックスへ進める
    currentCmdIndex = (currentCmdIndex + 1) % totalCommands;
  }
}