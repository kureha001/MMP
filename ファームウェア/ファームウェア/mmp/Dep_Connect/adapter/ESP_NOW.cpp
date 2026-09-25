// filename : Dep_Connect/adapter/ESP_NOW.cpp
//========================================================
// 接続部門／担当：ESP-NOW
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/24)
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <esp_now.h>
//┴┴

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス：非同期キュー型
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class  AdapterESPNOW:
public AdapterQueueBase<String> // 接続識別子：String
{
private:
//========================================================
//§基本情報
//========================================================
  //─────────────────
  // 一般情報
  //─────────────────
  const int ADP_ID = ADP_ID_ESPN;
  int getAID() const override {return ADP_ID;}

  //─────────────────
  // サービス関連情報
  //─────────────────
  static AdapterESPNOW* MY_TASK; // タスク識別(インスタンス)

//========================================================
//§各種ヘルパ
//========================================================
  //━━━━━━━━━━━━━━━━━
  // MACアドレス編集部品
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // uint8_t[6] -> String (例: "AA:BB:CC:DD:EE:FF")
    //─────────────────
    static String macToString(const uint8_t* mac) {
      char buf[18];
      snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      return String(buf);
    } /* macToString() */

    //─────────────────
    // String -> uint8_t[6] デコード関数 (コロン区切り用)
    //─────────────────
    static void stringToMac(const String& macStr, uint8_t* mac) {
      sscanf(macStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
            &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    } /* stringToMac() */

    //─────────────────
    // Raw String -> uint8_t[6] デコード関数 (12桁連続ヘキサ用)
    // 例: "50787D185150" -> 0x50, 0x78, 0x7D, 0x18, 0x51, 0x50
    //─────────────────
    static void rawStringToMac(const String& rawMacStr, uint8_t* mac) {
      if (rawMacStr.length() < 12) return;
      sscanf(rawMacStr.c_str(), "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
            &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    } /* rawStringToMac() */

  //━━━━━━━━━━━━━━━━━
  // 汎用送信処理
  //━━━━━━━━━━━━━━━━━
  void sendRaw(
    const uint8_t* macBuf,  // 送信先MACアドレス
    const String&  msg      // 送信データ
  ) {
    //○返信相手がピアに未登録なら自動追加 
    if (!esp_now_is_peer_exist(macBuf)) {
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, macBuf, 6);
      peerInfo.channel = 0; // 現在のチャンネルを使用
      peerInfo.encrypt = false;
      esp_now_add_peer(&peerInfo);
    }
    //○返信相手がピアに未登録なら自動追加 
    esp_now_send(
      macBuf                     , // 送信先MACアドレス
      (const uint8_t*)msg.c_str(), // 送信データ
      msg.length()                 // 送信データ長
    );
  } /* sendRaw() */

//========================================================
//§返信処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // クライアントにレスポンス
  //━━━━━━━━━━━━━━━━━
  void SEND_CONN(String argConn) override final {
//--------------------------
//➡ブリッジ以外
#if (MODE != MODE_BRIDGE)
    //┬
    //●MACアドレス文字列をデコード
    //●クライアントにレスポンス    
    //●ログ出力
    uint8_t macBuf[6]; stringToMac(argConn, macBuf);
    sendRaw(macBuf, ctx.resMSG);
    adpFnBase::SHOW_LOG();
    //┴
#endif /* ➡ブリッジ以外 */
//--------------------------
  } /* SEND_CONN() */

//========================================================
//§受信処理
//========================================================
  //─────────────────
  // タスク関数
  //─────────────────
  static void ON_RECIVE(
    const esp_now_recv_info_t *argINFO, // 各種情報
    const uint8_t             *argDATA, // 受信データ
    int                       argLEN    // 受信データ長
  ) {
  //┬
  //○┐【前処理】
    //○受信内容を確認
    if (!MY_TASK) return;
    //│＼（当該インスタンスではない場合）
    //│ ▼終了：早期リターン
    //┴
  //│
  //○┐【主処理】
    //○┐キュー情報を取得
      //●接続識別子を取得
      //○フレームを取得
      String qConn  = macToString(argINFO->src_addr);
      String qFrame = String((const char*)argDATA, argLEN);
      //┴
    //│
    //●キューを登録
    MY_TASK->pushQueue(qConn, qFrame, 0);
    //┴
  //│
  //○┐【後処理】
  //┴┴
  } /* ON_RECIVE() */

//========================================================
//§転送処理
//========================================================
//############################
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
  //─────────────────
  // 転送実施
  //─────────────────
  void trans() override final {
  //┬
  //○┐【前処理】
    //●宛先情報を取得
    uint8_t macBuf[6] = {0};
    rawStringToMac(ctx.bridge.Dat1, macBuf);
    //┴
  //│
  //○┐【主処理】
    //○退避したフレームでリクエスト(コールバックでデータ受信)
    sendRaw(macBuf, ctx.bridge.Frame);
    //┴
  //│
  //○┐【後処理】
  //┴┴
  } /* trans() */
#endif /* ➡ブリッジ */
//############################

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // コンストラクタ：非同期キュー型
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  AdapterESPNOW(MmpContext& argCtx):
  AdapterBase<String>(argCtx),     // 接続識別子：String
  AdapterQueueBase<String>(argCtx) // 接続識別子：String
  {
  //┬
  //○┐【前処理】
    //●WiFi接続状況を確認
    if (!devWiFi::isConnect(true)) return;
    //┴
  //│
  //○┐【主処理】
    //○サーバを起動
    if (esp_now_init() != ESP_OK) {Log::prtln(" [NG] ESP-NOW"); return;}
    //│＼（起動に失敗した場合）
    //│ ○メッセージ表示
    //│ ▼終了：早期リターン
    //│
    //●受信タスクを登録
    MY_TASK = this                     ; // タスク識別を取得
    esp_now_register_recv_cb(ON_RECIVE); // コールバック関数で登録
    //┴
  //│
  //○┐【後処理】
    //○メッセージ表示
    char msg[128];
    snprintf(msg, sizeof(msg), " [OK] ESP-NOW (MAC %s)", String(WiFi.macAddress()));
    Log::prtln(String(msg));
  //┴┴
  } /* constractor AdapterESPNOW() */

}; /* class AdapterESPNOW */

//========================================================
//§インスタンス管理
//========================================================
AdapterESPNOW* AdapterESPNOW::MY_TASK = nullptr;