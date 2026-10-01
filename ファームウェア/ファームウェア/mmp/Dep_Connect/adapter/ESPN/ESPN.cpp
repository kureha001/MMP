// filename : Dep_Connect/adapter/ESP/ESPN.cpp
//========================================================
// 接続部門／担当：ESP-NOW（ベース）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/30)
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <esp_now.h>
//┴┴

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス【非同期キュー型】
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class  AD_ESPN : // 接続識別子：String
public AD_API_Queue<String> // 非同期キュー型
{
private:
//========================================================
//§基本情報
//========================================================
  static AD_ESPN* MY_TASK; // タスク識別(インスタンス)

//========================================================
//§最終処理
//========================================================
  //─────────────────
  // ヘルパ：MACアドレス宛に送信
  //─────────────────
void SendToMac(
    const String&  argMac,  // MACアドレス
    const String&  argFrame // 送信データ
  ) {
  //┬
  //○┐【前処理】
    //○┐宛先情報を用意する
      //○書式を求める ← 動作モード
      String strFormat =
        (MODE == MODE_BRIDGE) ?              // 動作モード
          "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx" : // 単純連結
          "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx"  ; // コロン区切
      //│
      //○MACアドレスを求める ← 書式・引数
      uint8_t macBuf[6];
      sscanf(
        argMac.c_str()   , // 引数：MACアドレス
        strFormat.c_str(), // 書式
        &macBuf[0],&macBuf[1],&macBuf[2],&macBuf[3],&macBuf[4],&macBuf[5]
      );
    //┴┴
  //│
  //○┐【主処理】
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
      macBuf,                            // 用意した宛先情報
      (const uint8_t*)argFrame.c_str(), // 引数：送信データ
      argFrame.length()                 // 引数：送信データ長
    );
  //│
  //○┐【後処理】
    //●コンテクスト・ログを出力する
    adpFnBase::LOG_CTX();
  //┴┴
  } /* SendToMac() */

//========================================================
//§受信処理
//========================================================
  //───────────────────────────
  // 並列処理を定義
  //───────────────────────────
  static void ON_RECIVE(
    const esp_now_recv_info_t *argINFO, // 各種情報
    const uint8_t             *argDATA, // 受信データ
    int                       argLEN    // 受信データ長
  ) {
  //┬
  //○┐【前処理】
    //○受信内容を確認
    if (!MY_TASK) return;
    //│＼（[タスク違い]の場合）
    //│ ▼終了：早期リターンする
    //│
    //○宛先情報を用意する ← 引数：各種情報
    char macBuf[18];
    snprintf(
      macBuf,
      sizeof(macBuf),
      "%02X:%02X:%02X:%02X:%02X:%02X",
      argINFO->src_addr[0], argINFO->src_addr[1],
      argINFO->src_addr[2], argINFO->src_addr[3],
      argINFO->src_addr[4], argINFO->src_addr[5]
    );
    //┴┴
  //│
  //○┐【主処理】
    //○┐キュー情報を用意する
      //○接続識別子を求める ← 用意した宛先情報
      //○フレームを求める　 ← 引数：受信データ・受信データ長
      String qConn  = String(macBuf);
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
//§モード別実装のインクルード
//========================================================
#if MODE == MODE_BRIDGE
  #include "ESPN_Bridge.cpp"
#else
  #include "ESPN_MainSub.cpp"
#endif

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // コンストラクタ【非同期キュー型】
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  AD_ESPN( MmpContext& argCtx) : // 接続識別子：String
  AD_API<      String>(argCtx, AID::ESPN), // 基本型
  AD_API_Queue<String>(argCtx, AID::ESPN)  // 非同期キュー型
  {
  //┬
  //○┐【前処理】
    //●初期化の健全性を確認する
    if (SETUP()) return;
    //│＼（問題がある場合）
    //│ ▼終了：早期リターンする
    //┴
  //│
  //○┐【主処理】
    //○サーバを起動
    if (esp_now_init() != ESP_OK) {Log::prtln(" [NG] ESP-NOW"); return;}
    //│＼（起動に失敗した場合）
    //│ ○メッセージ表示
    //│ ▼終了：早期リターンする
    //│
    //●受信タスクを登録
    MY_TASK = this                     ; // タスク識別を取得
    esp_now_register_recv_cb(ON_RECIVE); // コールバック関数で登録
    //┴
  //│
  //○┐【後処理】
    //●起動ログMSGを表示する
    char msg[128];
    snprintf(msg, sizeof(msg), " [OK] ESP-NOW (MAC %s)", String(WiFi.macAddress()));
    Log::prtln(String(msg));
  //┴┴
  } /* constractor AD_ESPN() */

}; /* class AD_ESPN */

//========================================================
//§インスタンス管理
//========================================================
AD_ESPN* AD_ESPN::MY_TASK = nullptr;