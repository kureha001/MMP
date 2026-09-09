// filename : Dep_Connect/adapter/base/ESP_NOW.cpp
//========================================================
// 接続部門／業務課／担当(標準型)：ESP-NOW 担当
//--------------------------------------------------------
// Ver 1.2.3 (2026/09/06) 
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <WiFi.h>
  #include <esp_now.h>
//┴┴

//========================================================
// 組織図
//========================================================
//┬
//□┐接続部門
  //□┐業務課
    //□担当(標準型)
    #include "_index_.h"
//┴┴┴

//########################################################
//# 処理詳細
//########################################################
class AdapterESPNOW : public AdapterQueueBase<String> {
public:
  //━━━━━━━━━━━━━━━━━
  // 抽象基底クラスからコンテクストを継承
  //━━━━━━━━━━━━━━━━━
  using AdapterQueueBase::AdapterQueueBase;

private:
//========================================================
// アダプタの基本
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 一般情報
  //━━━━━━━━━━━━━━━━━
    const int ADP_ID = ADP_ID_ESPN;
    int getAID() const override {return ADP_ID;} // 基底クラスに連携
    static AdapterESPNOW* MY_INSTANS; // 静的コールバックからのルーティング用

//========================================================
// 各種ヘルパ
//========================================================
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
  // String -> uint8_t[6] デコード関数
  //─────────────────
  static void stringToMac(const String& macStr, uint8_t* mac) {
    sscanf(macStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
  } /* stringToMac() */

//========================================================
// レスポンス
//========================================================
  //━━━━━━━━━━━━━━━━━
  // クライアントにレスポンス
  // ※AdapterQueueBaseをオーバーライド
  //─────────────────
  void SEND_CONN(String argConn) override {
    //┬
    //○MACアドレス文字列をデコード
    uint8_t macBuf[6];
    stringToMac(argConn, macBuf);

    //○メッセージをレスポンス    
    // 【対策1】返信相手がピアに未登録なら、ここで自動追加する
    if (!esp_now_is_peer_exist(macBuf)) {
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, macBuf, 6);
      peerInfo.channel = 0; // 現在のチャンネルを使用
      peerInfo.encrypt = false;
      esp_now_add_peer(&peerInfo);
    }

    // 【対策2】データ長から '+ 1' を外し、純粋な文字列の長さにする
    esp_now_send(
        macBuf,                           // 送信先MACアドレス
        (const uint8_t*)ctx.resMSG.c_str(), // 送信データ
        ctx.resMSG.length()               // 送信データ長（+1 を削除）
    );
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */

//========================================================
// データ受信
//========================================================
  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  static void ON_RECIVE(
    const esp_now_recv_info_t *recv_info,
    const uint8_t *payload,
    int length
  ) {
    //┬
    //○インスタンスを確認
    if (!MY_INSTANS) return;
    //│＼（通信デバイスが起動していない場合）
    //│ ▼終了：早期リターン
    //│
    //○送信元MACアドレスを取得してStringへ変換
    String macStr = macToString(recv_info->src_addr);
    String frame  = String((const char*)payload, length);
    //│
    //○受信データをキューに追加（基底クラスの pushQueue を呼出し）
    MY_INSTANS->pushQueue(macStr, frame);
    //┴
  } /* ON_RECIVE() */


//========================================================
// 担務（公開機能）
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterESPNOW(MmpContext& argCtx) : AdapterQueueBase(argCtx) {
    //┬
    //○インスタンスを登録
    MY_INSTANS = this;
    //│
    //○サービス資源を生成
    if (esp_now_init() != ESP_OK) {
    //│＼（通信デバイスが起動していない場合）
        //○起動ログを表示（異常終了）
        //▼終了：早期リターン
        Serial.println(" [NG ] ESP-NOW -> 初期化失敗");
        return;
    } /* END-if */
    esp_now_register_recv_cb(ON_RECIVE); // コールバック関数登録
    //│
    //○メッセージ表示
    Serial.println(String(" [OK] ESP-NOW   -> MAC ") + String(WiFi.macAddress()));
    //┴
  } /* constractor AdapterESPNOW() */

  //━━━━━━━━━━━━━━━━━
  // 転送受付
  //━━━━━━━━━━━━━━━━━
//  void trans() {}

}; /* class AdapterESPNOW */

//━━━━━━━━━━━━━━━━━
//インスタンス管理用
//━━━━━━━━━━━━━━━━━
AdapterESPNOW* AdapterESPNOW::MY_INSTANS = nullptr;