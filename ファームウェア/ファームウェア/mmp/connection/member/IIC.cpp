// filename : connection/member/IIC.cpp
//========================================================
// 経路アダプタ：IIC
//--------------------------------------------------------
// Ver 1.2.3 (2026/09/06) 
//========================================================
//┬
//■┐インクルード
  //■同僚
  #include "_index_.h"
  //│
  //■Arduinoシステム
  #include <Wire.h>
  //┴
//┴

//########################################################
//# クラス：経路アダプタ(IIC)
//########################################################
class AdapterIIC : public AdapterQueueBase<uint8_t> {
public:
  //━━━━━━━━━━━━━━━━━
  // 抽象基底クラスからコンテクストを継承
  //━━━━━━━━━━━━━━━━━
  using AdapterQueueBase::AdapterQueueBase;

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
    const int ADP_ID = ADP_ID_IIC;

  //━━━━━━━━━━━━━━━━━
  // ID取得 (基底クラスの dispatch 処理用)
  //━━━━━━━━━━━━━━━━━
  int getAdpId() const override { return ADP_ID; }
    
  //━━━━━━━━━━━━━━━━━
  // 接続管理
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // 基本情報
    //─────────────────
    static const uint8_t IIC_ADDR_MIN = 0xA0;
    static const uint8_t IIC_ADDR_MAX = 0xA4;
    String CONN_TX[IIC_ADDR_MAX - IIC_ADDR_MIN + 1]; // 返送バッファ

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
  void SEND_CONN(bool argMode, uint8_t argConn) override {
    //┬
    //○動作モードを確認
    if (!argMode && ctx.sysMode != MODE_MAIN) return;
    //│＼（出力制限がなく、メインモード以外の場合）
    //│ ▼終了：早期リターン
    //│
    //○レスポンス内容を返送バッファにセット
    //  ※ここではレスポンスしないでスレッド処理に回す
    CONN_TX[argConn - IIC_ADDR_MIN] = ctx.resMSG;
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */

//========================================================
// Ｄ．データ受信
//========================================================
  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  //static void ON_RECIVE(){
  void ON_RECIVE(){
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
      CONN_TX[nowID]  = "";
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
      //○キューに登録（基底クラスの pushQueue を呼出し）
      pushQueue((uint8_t)ID, retFrame);
      //┴
    } /* END-for */
    //┴
  } /* ON_RECIVE() */

  //━━━━━━━━━━━━━━━━━
  // スレッド処理の定義
  //━━━━━━━━━━━━━━━━━
  TaskHandle_t TaskHandle = NULL; // タスク・ハンドル
  static void StreamQueue(void *pvParameters) {
    AdapterIIC* self = static_cast<AdapterIIC*>(pvParameters);
    for (;;) {
      if (self) self->ON_RECIVE();        // 疑似コールバック関数
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
  AdapterIIC(MmpContext& argCtx) : AdapterQueueBase(argCtx) {
    //┬
    //○サービスを開始
    //  ※PWMモジュールが先行して初期化済み
    //│
    //○受信タスクをFreeRTOSの別スレッドとして起動（自動コア割当）
    xTaskCreate(
      StreamQueue           , // 実行するタスク関数
      String(ADP_ID).c_str(), // タスク名（デバッグ用）
      4096                  , // スタックサイズ（バイト単位）
      this                  , // パラメータ
      2                     , // 優先度
      &TaskHandle             // タスクハンドル
    );
    //│
    //○メッセージ表示
    Serial.print  (String(" [OK] IIC       -> "));
    Serial.print  (String(IIC_ADDR_MIN));
    Serial.print  (" ～ ");
    Serial.println(String(IIC_ADDR_MAX));
    //┴
  } /* constractor AdapterIIC() */
}; /* class AdapterIIC */