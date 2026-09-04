// filename : adapter/ESP_NOW.cpp
//========================================================
// 経路アダプタ：ESP NOW
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■┐Arduinoシステム
  #include <WiFi.h>
  #include <esp_now.h>
  //┴
//┴

//########################################################
//# クラス：経路アダプタ(ESP NOW)
//########################################################
class AdapterESPNOW : public AdapterBase {
private:
//========================================================
// Ａ．アダプタの基本
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // ステータス
    //─────────────────
    const String ADP_ID = "ESPN"; // アダプタID

    //─────────────────
    // 使用するサービス
    //─────────────────
    // ・ESP-NOW

//========================================================
// Ｂ．レスポンス
//========================================================
void SEND_CONN(const uint8_t* argConn){
    //┬
    //○動作モードを確認
    if (ctx.sysMode != MODE_MAIN) return;
    //│＼（メインモード以外の場合）
    //│ ▼終了：早期リターン
    //│
    //○メッセージをレスポンス    
    // 【対策1】返信相手がピアに未登録なら、ここで自動追加する
    if (!esp_now_is_peer_exist(argConn)) {
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, argConn, 6);
      peerInfo.channel = 0; // 現在のチャンネルを使用
      peerInfo.encrypt = false;
      esp_now_add_peer(&peerInfo);
    }

    // 【対策2】データ長から '+ 1' を外し、純粋な文字列の長さにする
    esp_now_send(
        argConn,                          // 送信先MACアドレス
        (const uint8_t*)ctx.resMSG.c_str(), // 送信データ
        ctx.resMSG.length()               // 送信データ長（+1 を削除）
    );
    //│
    //●ログ出力
    adpBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */

//========================================================
// Ｃ．リクエスト・キュー管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
  struct myQueue {
    uint8_t CONN[6]; // MACアドレス
    String  FRAME  ; // 受信バッファ
  };
  static std::queue<myQueue> QUEUE      ; // キューバッファ
  static std::mutex          QUEUE_MUTEX; // 別スレッドとの衝突回避用のロック

  //─────────────────
  // キューの取出
  //----------------------------------
  // 引数：
  // ・キュー受取用の変数
  //----------------------------------
  // 戻り値：キューの有無（論理値）
  // ・true ：あり
  // ・false：なし
  //─────────────────
  bool popQueue(myQueue &argData) {
    //┬
    //○別スレッドとの衝突回避用のロック
    std::lock_guard<std::mutex> lock(QUEUE_MUTEX);
    //│
    //○キューの容量を確認
    if (QUEUE.empty()) return false;
    //│＼（通信デバイスが起動していない場合）
    //│ ▼返却：なし
    //│
    //○先頭を抽出
    //○先頭を削除
    //▼返却：あり
    argData = QUEUE.front();
    QUEUE.pop();
     return true;
  } /* popQueue() */

//========================================================
// Ｄ．データ受信
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
    //○未取り込みデータを受信
//    if (recv_info == nullptr || payload == nullptr || length < 1) return;
    //│＼（空の場合）
    //│ ▼終了：早期リターン
    //│
    //○送信元MACアドレスを取得
    uint8_t mac[6];
    memcpy(mac, recv_info->src_addr, 6);
    //│
    //○受信データをキューに追加
    std::lock_guard<std::mutex> lock(QUEUE_MUTEX);
    myQueue pkt;
    memcpy(pkt.CONN, mac, 6);
    pkt.FRAME = String((const char*)payload, length);
    QUEUE.push(pkt);
    //│
    //▼終了：早期リターン
    return;
  } /* ON_RECIVE() */

//========================================================
// Ｅ．公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterESPNOW(MmpContext& argCtx) : AdapterBase(argCtx) {
    //┬
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
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━
  void handle() override {
    //┬
    //◎┐ルーティングを指示
    myQueue popDat;
    while (popQueue(popDat)) {
      //│＼（キューが空の場合）
      //│ ▼BREAK：ルーティングを終了
      //│
      //○フレームの状態を確認
      if (popDat.FRAME.startsWith("#")){SEND_CONN(popDat.CONN); continue;}
      //│＼（エラーが発生している場合）
      //│ ●エラーをレスポンス
      //│ ▽次へ：次のキューを走査
      //│
      //●コマンドを実行
      adpBase::RUN(ADP_ID, popDat.FRAME);
      //│
      //●実行結果をレスポンス
      SEND_CONN(popDat.CONN);
      //┴
    } /* END-while */
    //┴
  } /* handle() */

}; /* class AdapterESPNOW */


//########################################################
//# スタティック資源の実体
//########################################################
//┬
//■サーバ／サービス
//│
//■送受信バッファ
//│
//■スレッド／コールバック
//│
//■リクエスト
std::queue<AdapterESPNOW::myQueue> AdapterESPNOW::QUEUE;
std::mutex                         AdapterESPNOW::QUEUE_MUTEX;
//┴