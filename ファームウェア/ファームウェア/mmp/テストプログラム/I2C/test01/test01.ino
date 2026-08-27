// filename : test01.ino
//========================================================
// Ｉ２Ｃスレーブのテスト
//--------------------------------------------------------
// Ver 1.0.0 (2026/08/27)
//========================================================
#include <Wire.h>

//========================================================
// 設定
//========================================================
const     uint8_t I2C_ADDR = 0xA0; // I2C Slaveアドレス
constexpr size_t  RX_SIZE  = 256 ; // 最大データ長
const     int     WAIT_MS  = 500 ; // レスポンス待機時間(msec)

//========================================================
// MMPへ返すリクエストのリスト
//========================================================
const char* commandList[] = {
  "SYS/VERSION!",
  "MP3/TRACK/PLAY:1:1:1!",
  "MP3/TRACK/STOP:1!",
  "DIGITAL/OUTPUT:17:1!",
  "DIGITAL/OUTPUT:17:0!"
};

const int totalCommands =
  sizeof(commandList) / sizeof(commandList[0]);

int currentCmdIndex = 0;

//========================================================
// リクエスト管理
//========================================================
volatile bool requestPending = false;

//========================================================
// レスポンス受信用バッファ
//========================================================
char lastResponse[RX_SIZE] = {0};
volatile bool responseReceived = false;


//========================================================
// Ａ．Ｉ２Ｃコールバック
//========================================================

//────────────────────────────────────────
// MMPからREADされたとき
//────────────────────────────────────────
void OnRequest() {
  //┬
  //○現在のリクエストを取得
  const char* payload = commandList[currentCmdIndex];

  //│
  //○MMPへリクエストを返す
  Wire.write((const uint8_t*)payload, strlen(payload));

  //│
  //○送信済みとして記録
  requestPending = true;
  //┴
} /* OnRequest() */


//────────────────────────────────────────
// MMPからWRITEされたとき
//────────────────────────────────────────
void OnReceive(int len) {
  //┬
  //○受信バッファを初期化
  int index = 0;

  //│
  //◎┐I2Cパケットを取得
  while (Wire.available() && index < RX_SIZE - 1) {
    //○1バイト取得
    lastResponse[index++] = Wire.read();
  } /* END-while */

  //│
  //○文字列終端
  lastResponse[index] = '\0';

  //│
  //○受信完了を記録
  responseReceived = true;
  //┴
} /* OnReceive() */


//========================================================
// Ｂ．初期化
//========================================================
void setup() {
  //┬
  //○シリアル通信を開始
  Serial.begin(115200);
  delay(1000);

  //│
  //○I2C Slaveとして開始
  Wire.begin(I2C_ADDR);

  //│
  //○I2Cコールバックを登録
  Wire.onRequest(OnRequest);
  Wire.onReceive(OnReceive);

  //│
  //○起動情報を表示
  Serial.println("=== I2C Slave Initialized ===");
  Serial.print("Slave Address: 0x");
  Serial.println(I2C_ADDR, HEX);
  Serial.println("----------------------------------");
  //┴
}


//========================================================
// Ｃ．メイン処理
//========================================================
void loop() {
  //┬
  //○MMPからWRITEされたレスポンスを確認
  if (responseReceived) {
    //│
    //○割り込み側から受信状態を引き継ぐ
    noInterrupts();
    responseReceived = false;
    interrupts();

    //│
    //○レスポンスを表示
    Serial.print("[RECV] Response: [");
    Serial.print(lastResponse);
    Serial.println("]");

    //│
    //○次のリクエストへ
    currentCmdIndex =
      (currentCmdIndex + 1) % totalCommands;
  } /* END-if */

  //│
  //○短い待機
  delay(1);
  //┴
}