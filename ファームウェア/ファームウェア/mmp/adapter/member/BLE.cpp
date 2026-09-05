// filename : adapter/BLE.cpp
//========================================================
// 経路アダプタ：BLE
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■同僚
  #include "_index_.h"
  //│
  //■他部門連携
  #include "../../dev.h"       // デバイスの初期化(devBLEを参照の為）
  //│
  //■Arduinoシステム
  #include <BLEDevice.h> // ユーザ受付資源
  #include <BLEServer.h> // ユーザ受付資源
  #include <BLEUtils.h > // ユーザ受付資源
  #include <BLE2902.h  > // ユーザ受付資源
  //┴
//┴

//########################################################
//# クラス：経路アダプタ(BLE)
//########################################################
class AdapterBLE : public AdapterBase {
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
    const  int  ADP_ID = ADP_ID_BLE;
    static bool IS_BUSY; // 接続状況｛true：接続あり｜false：接続なし｝

    //─────────────────
    // 使用するサービス
    //─────────────────
    // ※BLEはサービスポートを持たないため、
    //   dev.hで公開されたBLE固有の受付資源を使用
    // ・BLE_RX：受信用キャラクタリスティック
    // ・BLE_TX：送信用キャラクタリスティック

//========================================================
// Ｂ．レスポンス
//========================================================
  //─────────────────
  // スロットの受付資源に送信
  //----------------------------------
  // 引数：
  // ・出力制限：強制出力(true)、通常出力(false)
  // ・接続資源：キューから取得した物
  //─────────────────
  void SEND_CONN(bool argMode, uint8_t argConn){
    //┬
    //○動作モードを確認
    if (!argMode && ctx.sysMode != MODE_MAIN) return;
    //│＼（出力制限がなく、メインモード以外の場合）
    //│ ▼終了：早期リターン
    //│
    //○メッセージをレスポンス
    if (devBLE::BLE_TX != nullptr) {
      devBLE::BLE_TX->setValue(ctx.resMSG.c_str());
      devBLE::BLE_TX->notify();
    } /* END-if */
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */

//========================================================
// Ｃ．リクエスト・キュー管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
  static const int WAIT_MS = 15 ; // 受信タイムラグ
  struct myQueue {
    uint8_t CONN ; // アクセス資源(クライアント番号)
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
  // コールバック：サーバ用
  //━━━━━━━━━━━━━━━━━
  // ※既定のコールバック用のクラス関数をオーバーライドする
  class Callback_Server : public BLEServerCallbacks {
    //─────────────────
    // 接続イベント：接続制限（同時1人）
    //─────────────────
    void onConnect(BLEServer* pServer) override {
      //┬
      //○接続状況を確認
      if (IS_BUSY) return;
      //│＼（既に参加している場合）
      //│ ▼終了：これ以上は参加させない
      //│
      //○ステータスを変更（接続済）
      IS_BUSY = true;
      //│
      //○アドバタイジングを停止(新規の侵入を物理的に防ぐ)
      if (devBLE::MY_SRV != nullptr) devBLE::MY_SRV->getAdvertising()->stop();
      //┴
    } /* onConnect() */

    //─────────────────
    // 切断イベント：接続制限を解除
    //─────────────────
    void onDisconnect(BLEServer* pServer) override {
      //┬
      //○アドバタイジングを再開
      //○ステータスを変更（未接続）
      if (devBLE::MY_SRV != nullptr) devBLE::MY_SRV->startAdvertising();
      IS_BUSY = false;
      //┴
    } /* onDisconnect() */
  }; /* Callback_Server */
  static Callback_Server ON_CONNECTION;

  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  // ※既定のコールバック用のクラス関数をオーバーライドする
  class Callback_Client : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      //┬
      //○未取り込みデータを受信（getValue()参照後は消費されない）
      delay(WAIT_MS);
      String rxData = pCharacteristic->getValue(); // データ複製
      if (rxData.length() < 1) return;
      //│＼（空の場合）
      //│ ▼終了：早期リターン
      //│
      //○受信データをキューに追加
      std::lock_guard<std::mutex> lock(QUEUE_MUTEX);
      QUEUE.push({0,rxData});
      //┴
    }; /* onWrite() */
  }; /* Callback_Client */
  static Callback_Client ON_RECIVE;

//========================================================
// Ｅ．公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterBLE(MmpContext& argCtx) : AdapterBase(argCtx) {
    //┬
    //○サービス資源を生成
    devBLE::MY_SRV->setCallbacks(&ON_CONNECTION); // サーバ(接続/切断)
    devBLE::BLE_RX->setCallbacks(&ON_RECIVE    ); // クライアント(受信)
    //│
    //○メッセージ表示
    Serial.println(" [OK] Bluetooth");
    //┴
  } /* constractor AdapterBLE() */

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
      if (popDat.FRAME.startsWith("#")){SEND_CONN(true, popDat.CONN); continue;}
      //│＼（エラーが発生している場合）
      //│ ●エラーを強制レスポンス
      //│ ▽次へ：次のキューを走査
      //│
      //●コマンドを実行
      mode::RUN(ADP_ID, popDat.FRAME);
      //│
      //●実行結果をレスポンス
      SEND_CONN(false, popDat.CONN);
      //┴
    } /* END-while */
    //┴
  } /* handle() */

}; /* class AdapterBLE */


//########################################################
//# スタティック資源の実体
//########################################################
//┬
//■サーバ／サービス
//│
//■送受信バッファ
//│
//■スレッド／コールバック
AdapterBLE::Callback_Server AdapterBLE::ON_CONNECTION  ;
AdapterBLE::Callback_Client AdapterBLE::ON_RECIVE      ;
bool                        AdapterBLE::IS_BUSY = false; // 入場制限
//│
//■リクエスト
std::queue<AdapterBLE::myQueue>  AdapterBLE::QUEUE;
std::mutex                       AdapterBLE::QUEUE_MUTEX;
//┴