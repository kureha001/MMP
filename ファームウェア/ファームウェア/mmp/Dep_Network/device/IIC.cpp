// filename : Dev_Network/device/IIC.cpp
//========================================================
// 通信部門／デバイス課：IIC 担当
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/15)
// ・UARTポートの見直し 
//========================================================
//┬
//■┐インクルード
  #include <Wire.h>
  #include <LittleFS.h>
  #include <ArduinoJson.h>
//┴┴

//########################################################
//# 処理詳細
//########################################################
namespace devIIC {
//========================================================
// 共通資源
//========================================================
  constexpr const char* FILE_PATH = "/device.json";

  int SDA_PIN = -1;
  int SCL_PIN = -1;

//========================================================
// JSON操作ヘルパ
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 設定ファイルの読み込み（起動ガード付き）
  //━━━━━━━━━━━━━━━━━
  static bool READ_JSON() {
    if (!LittleFS.begin(true)) {
      Log::prtln("   [NG] LittleFS のマウントに失敗しました");
      return false;
    }

    if (!LittleFS.exists(FILE_PATH)) {
      Log::prtln("   [NG] device.json が存在しません (起動停止)");
      return false;
    }

    File f = LittleFS.open(FILE_PATH, "r");
    if (!f) {
      Log::prtln("   [NG] device.json のオープンに失敗しました");
      return false;
    }

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
      Log::prtln("   [NG] device.json のパースに失敗しました");
      return false;
    }

    SDA_PIN = doc["iic"]["sda"] | -1;
    SCL_PIN = doc["iic"]["scl"] | -1;

    if (SDA_PIN < 0 || SCL_PIN < 0) {
      Log::prtln("   [NG] 不正な SDA/SCL ピン設定です");
      return false;
    }

    return true;
  }

//========================================================
// 担務（公開機能）
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
  bool ENABLED = false; // 有効判定：有効：true、無効：false

  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START() {
    Log::prtln(" [I2C]");

    //○起動ガード：device.json の読み込み
    if (!READ_JSON()) {
      ENABLED = false;
      return;
    }

    //○IICバスの初期化
    Wire.begin(SDA_PIN, SCL_PIN);

    //○メッセージ表示
    char msg[128];
    snprintf(msg, sizeof(msg), "   [OK] IIC -> SDA[%d], SCL[%d]\n", SDA_PIN, SCL_PIN);
    Log::prt (String(msg));
    Log::prtln("");

    //○有効性セット
    ENABLED = true;
  } /* START() */

  //━━━━━━━━━━━━━━━━━
  // ピン設定の更新と即時再起動（公開関数）
  //━━━━━━━━━━━━━━━━━
  bool UPDATE(int sda, int scl) {
    if (sda < 0 || scl < 0) return false;

    // 1. device.json の読み込み
    if (!LittleFS.exists(FILE_PATH)) return false;
    File fRead = LittleFS.open(FILE_PATH, "r");
    if (!fRead) return false;

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, fRead);
    fRead.close();
    if (err) return false;

    // 2. iic ノードのピン情報を更新して保存
    doc["iic"]["sda"] = sda;
    doc["iic"]["scl"] = scl;

    File fWrite = LittleFS.open(FILE_PATH, "w");
    if (!fWrite) return false;
    serializeJson(doc, fWrite);
    fWrite.close();

    // 3. 内部変数を更新
    SDA_PIN = sda;
    SCL_PIN = scl;

    // 4. Wire（I2C）バスの再構成
    Wire.end();
    Wire.begin(SDA_PIN, SCL_PIN);

    return true;
  }
} /* namespace devIIC */