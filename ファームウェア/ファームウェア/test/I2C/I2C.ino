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
constexpr size_t  RX_SIZE  = 128 ; // 最大データ長

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

const int cdmIDs =
  sizeof(commandList) / sizeof(commandList[0]);

int cdmID = 0;

//========================================================
// リクエスト管理
//========================================================
volatile bool IsSend = false;

//========================================================
// レスポンス受信用バッファ
//========================================================
char RES_MSG[RX_SIZE] = {0};
volatile bool IsReceived = false;


//========================================================
// Ａ．Ｉ２Ｃコールバック
//========================================================
//────────────
// MMPからREADされたとき
//────────────
void OnRequest() {
if (IsSend) {Wire.write((uint8_t*)"!", 1); return;}
  const char* payload = commandList[cdmID];
  Wire.write((const uint8_t*)payload, strlen(payload));
  IsSend = true;
} /* OnRequest() */

//────────────
// MMPからWRITEされたとき
//────────────
void OnReceive(int len) {
  int id = 0;
  while (Wire.available() && id < RX_SIZE - 1) RES_MSG[id++] = Wire.read();
  RES_MSG[id] = '\0'; // 末尾処理
if (String(RES_MSG) != "####!") IsReceived = true; // レスポンス待ち
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
  Serial.println("\n=== I2C Slave Initialized ===");
  Serial.print  (" Slave Address: 0x");
  Serial.println(I2C_ADDR, HEX);
  Serial.println("----------------------------------");
  //┴
}

//========================================================
// Ｃ．メイン処理
//========================================================
void loop() {
  //┬
  //○┐MMPからWRITEされたレスポンスを確認
  if (IsReceived) {
    //│
    //○割り込み側から受信状態を引き継ぐ
    noInterrupts();
    IsReceived = false;
    interrupts();
    //│
    // 送信済みコマンドのログを表示
    Serial.print(commandList[cdmID]);
    Serial.print("：");
    Serial.println(RES_MSG);
    //│
    //○次のリクエストへ
    cdmID = (cdmID + 1) % cdmIDs;
    IsSend = false;
    //┴
  } /* END-if */
  //│
  //○短い待機
  delay(1);
  //┴
}