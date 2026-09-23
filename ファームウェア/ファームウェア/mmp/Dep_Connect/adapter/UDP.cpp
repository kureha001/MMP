// filename : Dep_Connect/adapter/UDP.cpp
//========================================================
// 接続部門／業務課／担当(標準型)：UDP 担当
//--------------------------------------------------------
// Ver 1.4.1 (2026/09/23)
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <WiFi.h>
  #include <WiFiUdp.h>
  #include <queue>
  #include <mutex>
//┴┴

//========================================================
// 組織図
//========================================================
//┬
//□┐接続部門
  //□┐業務課
    //□担当
    #include "__index.h"
//┴┴┴

//########################################################
//# 処理詳細
//########################################################
class AdapterUDP : public AdapterQueueBase<String> {
public:
  //━━━━━━━━━━━━━━━━━
  // 抽象基底クラスからコンテクストを継承
  //━━━━━━━━━━━━━━━━━
  using AdapterQueueBase::AdapterQueueBase;

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
  int SLOTs = 0; // コンストラクタで決定
  struct T_SLOT {
    bool          used      = false             ; // 使用状況
    IPAddress     CONN_IP   = IPAddress(0,0,0,0); // 接続元（IPアドレス）
    uint16_t      CONN_Port = 0                 ; // 接続元（ポート番号）
    unsigned long timeStamp = 0                 ; // タイムスタンプ
  };
  T_SLOT* TBL = nullptr;

  //─────────────────
  // スロット内容をセット
  //─────────────────
  void SLOT_SET(
    int              argSID , //
    const IPAddress& argIP  , //
    uint16_t         argPort  //
  ) {
    //┬
    //○スロット内容をセット
    TBL[argSID].used      = true    ; // 有効性[ON]
    TBL[argSID].CONN_IP   = argIP   ; // IPアドレスを反映
    TBL[argSID].CONN_Port = argPort ; // ポート番号を反映
    TBL[argSID].timeStamp = millis(); // タイムスタンプを更新
    //┴
  } /* SLOT_SET() */

  //─────────────────
  // スロットIDを取得
  //----------------------------------
  // 戻り値：スロットID（数値型）
  // ・0～：スロットID
  // ・-1 ：該当なし
  //─────────────────
  int SLOT_ATTACH_ACTIVE(
    const IPAddress& argIP  , //
    uint16_t         argPort  //
  ) {
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
        if (
          TBL[ID].CONN_IP   == argIP   &&
          TBL[ID].CONN_Port == argPort &&
          TBL[ID].used
        ) break;
        //│＼（該当するスロットにヒットした場合）
        //│ ▽完了：走査を終了
        //┴
      } /* for */
      //│
      //○割当スロットを確認
      if (ID >= SLOTs) return -1;
      //│＼（該当するスロットがない場合）
      //│ ▼終了：早期リターン
      //│
      //○割当スロット内容をセット
      TBL[ID].timeStamp = millis();
      //┴
    //│
    //○┐【後処理】
      //▼返却：正常終了（スロットID）
      return ID;
    //┴
  } /* SLOT_GET_SID() */


  //─────────────────
  // スロットIDを取得
  //----------------------------------
  // 戻り値：スロットID（数値型）
  // ・0～：正常終了（スロットID）
  // ・-1 ：該当なし
  //─────────────────
  int SLOT_ATTACH_FREE(
    const IPAddress& argIP  , //
    uint16_t         argPort  //
  ) {
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
        if (!TBL[ID].used) break;
        //│＼（該当するスロットにヒットした場合）
        //│ ▽完了：走査を終了
        //┴
      } /* for */
      //│
      //○走査結果を確認
      if (ID >= SLOTs) return -1;
      //│＼（該当するスロットがない場合）
      //│ ▼終了：早期リターン
      //│
      //●割当スロット内容をセット
      SLOT_SET(ID, argIP, argPort);
      //┴
    //│
    //○┐【後処理】
      //▼返却：正常終了（スロットID）
      return ID;
    //┴
  } /* SLOT_GET_SID() */

  //─────────────────
  // 古いスロットIDを取得
  //----------------------------------
  // 戻り値：スロットID（数値型）
  // ・0～：スロットID
  //─────────────────
  int SLOT_ATTACH_OLD(
    const IPAddress& argIP  , //
    uint16_t         argPort  //
  ) {
    //┬
    //○┐【前処理】
      //┴
    //│
    //○┐【主処理】
      //◎┐最古のスロットIDを取得
      int oldID = 0;
      unsigned long oldTime = TBL[0].timeStamp;
      for (int ID = 1; ID < SLOTs; ID++) {
        //○スロット状態を確認
        if (TBL[ID].timeStamp < oldTime) {
          oldID   = ID;
          oldTime = TBL[ID].timeStamp;
        } /* if */
      } /* for */
      //│
      //●割当スロット内容をセット
      SLOT_SET(oldID, argIP, argPort);
      //┴
    //│
    //○┐【後処理】
      //▼返却：該当なし
      return oldID;
    //┴
  } /* SLOT_ATTACH_OLD() */

  //─────────────────
  // 動的アタッチ
  //----------------------------------
  // 戻り値 ：スロットID（数値）
  //─────────────────
  int SLOT_ATTACH(
    const IPAddress& argIP,
    uint16_t         argPort
  ) {
    //┬
    //○┐【前処理】
      //┴
    //│
    //○┐【主処理】
      //●既存スロットで走査
      //●空スロットを走査
      //●古いスロットで走査
      int       ID = SLOT_ATTACH_ACTIVE(argIP, argPort);
      if (ID<0) ID = SLOT_ATTACH_FREE  (argIP, argPort);
      if (ID<0) ID = SLOT_ATTACH_OLD   (argIP, argPort);
      //┴
    //│
    //○┐【後処理】
      //▼返却：スロットID
      return ID;
    //┴
  } /* SLOT_ATTACH() */

//========================================================
//§返信処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // クライアントにレスポンス（基底クラスの純粋仮想関数を実装）
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
        //●スロットIDを取得
        int qSID = SLOT_ATTACH(qIP, qPort);
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
  AdapterUDP(MmpContext& argCtx) : AdapterQueueBase(argCtx) {
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