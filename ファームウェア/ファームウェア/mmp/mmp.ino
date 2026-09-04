// filename : mmp.ino
//========================================================
//  MMP Firmware
//--------------------------------------------------------
// - ボード情報      : Waveshare ESP32-S3-tiny用
// - ボート          : ESP32S3 Dev Module
// - USB CDC ON Boot : Enabled
// - Flash Size      : 4MB (32Mb)
// - Patition Scheme : Huge APP(3MB No OTA/1MB SPIFFS)
//--------------------------------------------------------
// 追加ライブラリ：
// - WebSockets by Markus Sattler
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/03) 
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <Wire.h> // setup()
  //│
  //■ＭＭＰシステム(マネージャ群)
  #include "dev.h"  // 通信デバイス
  #include "adp.h"  // 経路アダプタ
  #include "cmd.h"  // コマンド
  //┴
//┴

//━━━━━━━━━━━━━━━━━
// セットアップ部品
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 資源の初期化
  //─────────────────
  void initialize(){
    //┬
    //●通信デバイス・マネージャに初期化を依頼
    DeviceManager::INIT();
    //│
    //●経路アダプタ・マネージャに初期化を依頼
    AdapterManager::INIT();
    //│
    //●コマンド・マネージャに初期化を依頼
    CommandManager::INIT();
    //┴
  } /* initialize() */

  //─────────────────
  // オープニング
  //─────────────────
  void opening(){
    //┬
    //○開始メッセージ出力
    String strMode = "";
    if (MODE_BOOT == MODE_MAIN  ) strMode = "メイン"  ;
    if (MODE_BOOT == MODE_SUB   ) strMode = "サブ"    ;
    if (MODE_BOOT == MODE_BRIDGE) strMode = "ブリッジ";
    Serial.println("---------------------------");
    Serial.printf (" MMP Ver.%s\n"    , ctx.sysVer);
    Serial.printf (" 動作モード：%s\n", strMode   );
    Serial.println("---------------------------");
    //│
    //●ファンファーレを鳴らす
    if (MODE_BOOT == MODE_MAIN) {
      ctx.cmdPath = "MP3/TRACK/PLAY_ROOT:1:1!";
      CommandManager::RunCommand();
    }
    //┴
  } /* opening() */

//========================================================
// セットアップ
//========================================================
void setup(){
  //┬
  //○資源を初期化
  initialize();
  //│
  //●オープニングを表示
  opening();
  //┴
} /* setup() */

//========================================================
// ポーリング
//========================================================
void loop(){
  //┬
  //●経路アダプタ・マネージャにハンドル実行を依頼
  AdapterManager::HANDLE();
  //┴
} /* loop() */