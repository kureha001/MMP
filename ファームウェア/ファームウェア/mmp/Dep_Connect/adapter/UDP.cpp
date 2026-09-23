// filename : Dep_Connect/adapter/UDP.cpp
//========================================================
// 接続部門／担当：UDP(非同期キュー＋スロット型)
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/23)
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <WiFi.h>
  #include <WiFiUdp.h>
  #include <queue>
  #include <mutex>
//┴┴

//┬
//□┐接続部門
  //□担当：通信アダプタ
  #include "__index.h"
//┴┴

//########################################################
class AdapterUDP:
  public AdapterQueueBase<String>,
  public AdapterSlotBase<String>
//########################################################
{
private:
//========================================================
//§基本情報
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 一般情報
  //━━━━━━━━━━━━━━━━━
    const int  ADP_ID = ADP_ID_UDP;
    int getAID() const override {return ADP_ID;}

  //━━━━━━━━━━━━━━━━━
  // サービス関連情報
  //━━━━━━━━━━━━━━━━━
    TaskHandle_t MY_TASK = NULL; // タスク識別(並列処理)
    WiFiUDP      MY_NET        ; // クライアント・サーバ(実体)
    int          MY_PORT = 8083; // ポート番号

//========================================================
//§各種ヘルパ
//========================================================
  //─────────────────
  // 接続識別子を取得
  //─────────────────
  static String getConn(
    const IPAddress& argIP,
    uint16_t         argPORT
  ) {
    return argIP.toString() + ":" + String(argPORT);
  }

//========================================================
//§接続管理
//========================================================
  int     SLOTs = 0; // コンストラクタで決定
  T_SLOT* TBL   = nullptr;

  //─────────────────
  // スロット内容をセット
  //─────────────────
  void SLOT_SET(
    int           argSID , //
    const String& argConn  //
  ) {
    //┬
    //○スロット内容をセット
    TBL[argSID].used      = true    ; // 有効性[ON]
    TBL[argSID].CONN      = argConn ; // 接続識別子を反映
    //┴
  } /* SLOT_SET() */

  //─────────────────
  // スロットIDを取得
  //----------------------------------
  // 戻り値：スロットID（数値型）
  // ・0～：スロットID
  // ・-1 ：該当なし
  //─────────────────
  int SLOT_GET_ACTIVE(const String& argConn) {
    //┬
    //○┐【前処理】
      //┴
    //│
    //○┐【主処理】
      //◎┐スロットを走査
      int  ID = 0;
      for (ID = 0; ID < SLOTs; ID++) {
        //│＼（すべて走査し終えた場合）
        //│ ▽完了：走査を終了
        //│
        //○スロット状態を確認
        if (TBL[ID].CONN == argConn && TBL[ID].used) break;
        //│＼（該当するスロットにヒットした場合）
        //│ ▽完了：走査を終了
        //┴
      } /* for */
      //┴
    //│
    //○┐【後処理】
      //▼返却
      return (ID < SLOTs) ? ID : -1;
    //┴
  } /* SLOT_GET_ACTIVE() */

  //─────────────────
  // スロット割当
  //----------------------------------
  // 戻り値 ：スロットID（数値）
  //─────────────────
  int SLOT_ASSIGN(const String& argConn) {
    //┬
    //○┐【前処理】
      //┴
    //│
    //○┐【主処理】
      //●既存スロットで走査
      int ID = SLOT_GET_ACTIVE(argConn);
      if (ID<0) {
        // ＼（該当する既存スロットがない場合）
          //●空スロットを走査
          ID             = SLOT_GET_FREE();
          if (ID < 0) ID = SLOT_GET_OLD ();
          // ＼（該当する空スロットがない場合）
            //●古いスロットを走査
            //┴
          //│
          //●割当スロットに内容をセット
          SLOT_SET(ID, argConn);
          //┴
      } /* if */
      //│
      //○タイムスタンプを更新
      TBL[ID].timeStamp = millis();
      //┴
    //│
    //○┐【後処理】
      //▼返却：スロットID
      return ID;
    //┴
  } /* SLOT_ASSIGN() */

//========================================================
//§返信処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // クライアントにレスポンス
  //━━━━━━━━━━━━━━━━━
  void SEND_CONN(String argConn) override {
//--------------------------
//➡ブリッジ以外：
#if (MODE != MODE_BRIDGE)
    //┬
    //○┐【前処理】
      //┴
    //│
    //○┐【主処理】
      //○クライアントを特定
      int intPos = argConn.indexOf(':');
      IPAddress sendIP   ; sendIP.fromString(argConn.substring(0, intPos));
      uint16_t  sendPort = argConn.substring(intPos + 1).toInt();
      //│
      //○クライアントにレスポンス
      MY_NET.beginPacket(sendIP, sendPort);
      MY_NET.write((const uint8_t*)ctx.resMSG.c_str(), ctx.resMSG.length());
      MY_NET.endPacket();
      //┴
    //│
    //○┐【後処理】
      //●ログ出力
      adpFnBase::SHOW_LOG();
    //┴┴
#endif /* ➡ブリッジ以外 */
//--------------------------
  } /* SEND_CONN() */

//========================================================
//§受信処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用（ポーリング）
  //━━━━━━━━━━━━━━━━━
  void ON_RECIVE(){
    //┬
    //○┐【前処理】
      //○受信内容を確認
      int packetSize = MY_NET.parsePacket();
      if (packetSize < 1) return;
      //│＼（パケット内容が[空]の場合）
      //│ ▼終了：早期リターン
      //┴
    //│
    //○┐【主処理】
      //○┐キュー情報を取得
        //●接続情報を取得（IPアドレス＋ポート番号）
        IPAddress qIP   = MY_NET.remoteIP();
        uint16_t  qPort = MY_NET.remotePort();
        String    qCONN = getConn(qIP, qPort);
        //│
        //○┐フレームを取得
          //●受信データを取得（ストリーム型）
          char getDat[LIMIT::READ_LEN];
          int  getLen = MY_NET.read(getDat, sizeof(getDat) - 1);
          if (getLen < 1) return;
          //│＼（データ内容が[空]の場合）
          //│ ▼終了：早期リターン
          //│
          //○受信データを整形し、受信フレームにセット
          getDat[getLen] = '\0';
          String qFrame = String(getDat);
          //┴
        //│
        //●割当スロットIDを取得
        int qSID = SLOT_ASSIGN(qCONN);
        //┴
      //│
      //●キューを登録
      pushQueue(qCONN, qFrame, qSID);
      //┴
    //│
    //○┐【後処理】
    //┴┴
  } /* ON_RECIVE() */

  //━━━━━━━━━━━━━━━━━
  // スレッド処理の定義
  //━━━━━━━━━━━━━━━━━
  static void StreamQueue(void *pvParameters) {
    AdapterUDP* self = static_cast<AdapterUDP*>(pvParameters);
    for (;;) {
      if (self) self->ON_RECIVE();        // 疑似コールバック関数
      vTaskDelay(1 / portTICK_PERIOD_MS); // 短いウェイト
    } /* for */
  } /* StreamQueue() */

  //━━━━━━━━━━━━━━━━━
  // 並列処理の開始
  //━━━━━━━━━━━━━━━━━
  void RUN_TASK() {
    //○受信タスクをFreeRTOSの別スレッドとして起動（自動コア割当）
    xTaskCreate(
      StreamQueue           , // 実行するタスク関数
      String(ADP_ID).c_str(), // タスク名（デバッグ用）
      4096                  , // スタックサイズ（バイト単位）
      this                  , // パラメータ
      2                     , // 優先度
      &MY_TASK                // タスク識別を取得
    );
  } /* RUN_TASK() */

//========================================================
//§ハンドル前処理
//========================================================

//========================================================
//§転送処理
//========================================================
//############################
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
  //━━━━━━━━━━━━━━━━━
  // 転送実施
  //━━━━━━━━━━━━━━━━━
  void trans() override final {
    //┬
    //○┐【前処理】
      //○宛先情報を取得
      String transIP = ctx.bridge.Dat1;
      //┴
    //│
    //○┐【主処理】
      //○クライアントを起動
      MY_NET.beginPacket(transIP.c_str(), MY_PORT);
      //│
      //○退避したフレームでリクエスト(コールバックでデータ受信)
      MY_NET.write((const uint8_t*)ctx.bridge.Frame.c_str(), ctx.bridge.Frame.length());
      MY_NET.endPacket();
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
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterUDP(MmpContext& argCtx) :
    AdapterBase(argCtx), 
    AdapterQueueBase<String>(argCtx), 
    AdapterSlotBase<String>(argCtx)
  {
    //┬
    //○┐【前処理】
      //●WiFi接続状況を確認
      if (!devWiFi::ENABLED(true)) return;
      //┴
    //│
//--------------------------
//➡ブリッジ：[スロット]が単一，[受信タスク]が不要
#if (MODE == MODE_BRIDGE)
    //○┐【主処理】
      //○スロットを初期化
      SLOTs = 1;
      TBL   = new T_SLOT[SLOTs];
      //│
      //○UDPリスナー起動
      MY_NET.begin(MY_PORT);
      //┴
    //│
    //○┐【後処理】
      Log::prtln(" [OK] UDP");
    //┴┴
//➡ブリッジ以外：[スロット]が複数，[受信タスク]が必要
#else
    //○┐【主処理】
      //○スロットを初期化
      SLOTs = 10;
      TBL   = new T_SLOT[SLOTs];
      //│
      //○UDPリスナー起動
      MY_NET.begin(MY_PORT);
      //│
      //●受信タスクを登録
      RUN_TASK(); // 並列処理で登録
      //┴
    //│
    //○┐【後処理】
      //○メッセージ表示
      char msg[128];
      snprintf(msg, sizeof(msg), " [OK] UDP        (PORT %d)", MY_PORT);
      Log::prtln(String(msg));
    //┴┴
#endif /* ➡ブリッジ｜➡ブリッジ以外 */
//--------------------------
  } /* constractor AdapterUDP() */

}; /* class AdapterUDP */