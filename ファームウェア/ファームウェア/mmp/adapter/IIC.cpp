// filename : adapter/IIC.cpp
//========================================================
// 経路アダプタ：IIC
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <Wire.h>
  //┴
//┴

//########################################################
//# クラス：経路アダプタ(IIC)
//########################################################
class AdapterIIC : public AdapterBase {
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
    const String ADP_ID   = "IIC"; // アダプタID
    
  //━━━━━━━━━━━━━━━━━
  // 接続管理
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // 基本情報
    //─────────────────
    static const uint8_t IIC_ADDR_MIN = 0xA0; // スレーブのIICアドレス（先頭）
    static const uint8_t IIC_ADDR_MAX = 0xA4; // スレーブのIICアドレス（末尾）
    static       String  CONN_TX[IIC_ADDR_MAX - IIC_ADDR_MIN + 1]; // 返送バッファ

 //========================================================
// Ｂ．レスポンス
//========================================================
  void SEND_CONN(uint8_t argConn){
#if !defined(MMP_TYPE_BRIDGE) // ---┨ブリッジモード以外┠┐
    //┬
    //○レスポンス内容を返送バッファにセット
    //  ※ここではレスポンスしないでスレッド処理に回す
    CONN_TX[argConn - IIC_ADDR_MIN] = ctx.resMSG;
    //│
    //●ログ出力
    adpBase::SHOW_LOG();
    //┴
#endif // ------------------------------------------------┘
  } /* SEND_CONN() */

//========================================================
// Ｃ．リクエスト・キュー管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
    struct myQueue {
      uint8_t CONN ; // IIC Slaveアドレス
      String  FRAME; // 受信バッファ
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
  static void ON_RECIVE(){
    //┬
    //◎┐スレーブ（IICアドレス）を走査
    for (uint8_t ID = IIC_ADDR_MIN; ID <= IIC_ADDR_MAX; ID++) {
      //│＼（最後のアドレスに達した場合）
      //│ ▼完了：走査を終了
      //│
      //○前処理
      String retFrame = "";
      int    nowID    = ID - IIC_ADDR_MIN;
      String msg      = CONN_TX[nowID] == "" ? "####!" : CONN_TX[nowID];
      CONN_TX[nowID] = "";
      //│
      //○レスポンスをスレーブへ返信
      Wire.beginTransmission(ID);
      Wire.write((const uint8_t*)msg.c_str(),msg.length());
      Wire.endTransmission(false);
      //│
      //○リクエストをスレーブから取得
      Wire.requestFrom(ID, SS_RX_SIZE); // 指定サイズ分取得する
      while (Wire.available()) retFrame += (char)Wire.read();
      //│
      //○末尾の余分をカット
      int idx = retFrame.indexOf('!');
      if (idx < 0) continue;
      retFrame = retFrame.substring(0, idx + 1);
      if (retFrame == "!") continue;
      //│
      //○キューに登録
      std::lock_guard<std::mutex> lock(QUEUE_MUTEX); // 排他ロック
      QUEUE.push({(uint8_t)ID, retFrame});           // キューを追加(通信資源、フレーム)
      //┴
    } /* END-for */
    //┴
  } /* ON_RECIVE() */

  //━━━━━━━━━━━━━━━━━
  // スレッド処理の定義
  //━━━━━━━━━━━━━━━━━
  static TaskHandle_t TaskHandle;         // タスク・ハンドル
  static void StreamQueue(void *pvParameters) {
    for (;;) {
      ON_RECIVE();                        // 疑似コールバック関数
      vTaskDelay(1 / portTICK_PERIOD_MS); // 短いウェイト
    }
  } /* StreamQueue() */

//========================================================
// Ｅ．公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterIIC(MmpContext& argCtx) : AdapterBase(argCtx) {
    //┬
    //○サービスを開始
    //  ※PWMモジュールが先行して初期化済み
    //│
    //○受信タスクをFreeRTOSの別スレッドとして起動（自動コア割当）
    xTaskCreate(
      StreamQueue,    // 実行するタスク関数
      ADP_ID.c_str(), // タスク名（デバッグ用）
      4096,           // スタックサイズ（バイト単位）
      NULL,           // パラメータ
      2,              // 優先度
      &TaskHandle     // タスクハンドル
    );
    //│
    //○メッセージ表示
    Serial.print  (String(" [OK] IIC       -> "));
    Serial.print  (String(IIC_ADDR_MIN));
    Serial.print  (" ～ ");
    Serial.println(String(IIC_ADDR_MAX));
    //┴
  } /* constractor AdapterIIC() */

  //━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━
  void handle() override {
    //┬
    //◎┐ルーティングを指示
    myQueue popDat;
    while (popQueue(popDat)) {
      //│＼（キューが空の場合）
      //│ ▼完了：ルーティングを終了
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

}; /* class AdapterIIC */


//########################################################
//# スタティック資源の実体
//########################################################
//┬
//■サーバ／サービス
//│
//■送受信バッファ
String AdapterIIC::CONN_TX[AdapterIIC::IIC_ADDR_MAX - AdapterIIC::IIC_ADDR_MIN + 1];
//│
//■スレッド／コールバック
TaskHandle_t  AdapterIIC::TaskHandle = NULL;
//│
//■リクエスト
std::queue<AdapterIIC::myQueue>   AdapterIIC::QUEUE;
std::mutex                        AdapterIIC::QUEUE_MUTEX;
//┴
