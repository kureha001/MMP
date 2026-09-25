// filename : Dep_Connect/adapter/TCP.cpp
//========================================================
// 接続部門／担当：TCP
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/24)
//========================================================

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス：非同期キュー型＋スロット型
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class  AdapterTCP:
public AdapterQueueBase<WiFiClient>, // 接続識別子：WiFiClient
public AdapterSlotBase<WiFiClient>   // 接続識別子：WiFiClient
{
private:
//========================================================
//§基本情報
//========================================================
  //─────────────────
  // 一般情報
  //─────────────────
  const int ADP_ID = ADP_ID_TCP;
  int getAID() const override {return ADP_ID;}
  
  //─────────────────
  // サービス関連情報
  //─────────────────
  int         MY_PORT = 8081   ; // ポート番号
//--------------------------
//➡ブリッジ：クライアント
#if (MODE == MODE_BRIDGE)
  WiFiClient  MY_NET           ; // クライアント(実体)
//➡ブリッジ以外：サーバ
#else
  WiFiServer* MY_NET  = nullptr; // サーバ(ポインタ)
#endif /* ➡ブリッジ｜➡ブリッジ以外 */
//--------------------------

//========================================================
//§各種ヘルパ
//========================================================
  //─────────────────
  // 有効性確認
  //─────────────────
  bool ENA_CLIENT(WiFiClient argConn, bool argLog){
    bool ret = argConn.connected();
    if (ret == false && argLog) Log::prtln("[ERROR] TCPクライアントが切断されました。");
    return ret;
  } /* ENA_CLIENT() */

//========================================================
//§接続管理
//========================================================
  //─────────────────
  // 初期化
  //----------------------------------
  // 引数：(参照)接続管理スロット
  //─────────────────
  void SLOT_INI(T_SLOT& argSlot){
    argSlot.used = false;
    if (argSlot.CONN) argSlot.CONN.stop();
  } /* SLOT_INI() */

  //─────────────────
  // スロット内容をセット
  //─────────────────
  void SLOT_INI_SET(
    int              argSID , //
    const WiFiClient argConn  //
  ) {
    //┬
    //●スロットを初期化
    SLOT_INI(TBL[argSID]);
    //│
    //●スロット内容をセット
    SLOT_SET(argSID, argConn);
    //│
    //○スロット内容をセット
    TBL[argSID].CONN.setNoDelay(true); // TCPパケット遅延制御
    //┴
  } /* SLOT_INI_SET() */

  //─────────────────
  // 新接続のスロットを作成
  //----------------------------------
  // 戻り値 ：処理結果（論理値）
  // ・false：正常
  // ・true ：異常
  //─────────────────
  void SLOT_CREATE(){
//--------------------------
//➡ブリッジ：単一スロット
#if (MODE == MODE_BRIDGE)
  //┬
  //○┐【前処理】
    //┴
  //│
  //○┐【主処理】
    //●割当スロット内容をセット
    SLOT_INI_SET(0, MY_NET);
  //│
  //○┐【後処理】
  //┴┴
//➡ブリッジ以外：動的スロット
#else
  //┬
  //○┐【前処理】
    //┴
  //│
  //○┐【主処理】
    //◎┐新接続のスロットを作成
    while (true) {
      //│
      //○未管理のTCP接続を取得
      WiFiClient newConn = MY_NET->available();
      if (!newConn) return;
      //│＼（未管理のTCP接続がない場合）
      //│ ▼終了：早期リターンする
      //│
      //●空きスロットを探す
      int       ID = SLOT_GET_FREE();
      if (ID<0) ID = SLOT_GET_OLD ();
      //│＼（該当する既存スロットがない場合）
      //│ ●古いスロットを走査
      //│ ┴
      //│
      //●割当スロット内容をセット
      SLOT_INI_SET(ID, newConn);
      //┴
    } //* while */
  //│
  //○┐【後処理】
  //┴┴
#endif /* ➡ブリッジ｜➡ブリッジ以外 */
//--------------------------
  } /* SLOT_CREATE() */

//========================================================
//§返信処理
//========================================================
  //─────────────────
  // クライアントにレスポンス
  //─────────────────
  void SEND_CONN(WiFiClient argConn) override final {
//--------------------------
//➡ブリッジ以外
#if (MODE != MODE_BRIDGE)
  //┬
  //○┐【前処理】
    //●TCP接続状況を確認
    if (!ENA_CLIENT(argConn, true)) return;
    //┴
  //│
  //○┐【主処理】
    //○クライアントにレスポンス
    argConn.print(ctx.resMSG);
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
  //─────────────────
  // タスク関数
  //─────────────────
  void ON_RECIVE() override final {
  //┬
  //○┐【前処理】
    //●新接続のスロットを作成
    SLOT_CREATE();
    //┴
  //│
  //○┐【主処理】
    //◎┐スロットを走査
    for (int qSID = 0; qSID < SLOTs; qSID++) {
      //│＼（すべて走査し終えた場合）
      //│ ▼完了：走査を終了
      //│
      //○┐【前処理（走査単位）】
        //○使用状況を確認
        if (!TBL[qSID].used) continue;
        //│＼（[未使用]の場合）
        //│ ▽次へ：次のスロットの走査へ進む
        //│
        //●TCP接続状況を確認
        if (!ENA_CLIENT(TBL[qSID].CONN, false)){SLOT_INI(TBL[qSID]); continue;}
        //│＼（接続が切れている場合）
        //│ ●スロットを初期化する
        //│ ▽次へ：次のスロットの走査へ進む
        //│
        //○タイムスタンプを更新
        TBL[qSID].timeStamp = millis();
        //┴
      //│
      //○┐キュー情報を用意
        //●フレームを求める
        String qFrame = adpFnStream::GET_FRAME(TBL[qSID].CONN);
        if (qFrame == "") continue;
        //│＼（受信データがない場合）
        //│ ▽次へ：次のスロットの走査へ進む
        //┴
      //│
      //●キューを登録
      pushQueue(TBL[qSID].CONN, qFrame, qSID);
      //┴
    } /* for */
    //┴
  //│
  //○┐【後処理】
  //┴┴
  } /* ON_RECIVE() */

//========================================================
//§転送処理(ブリッジ／スレーブ)
//========================================================
//############################
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
  //─────────────────
  // 転送実施
  //─────────────────
  void TRANS() override final {
  //┬
  //○┐【前処理】
    //○宛先情報を取得
    String transIP = ctx.bridge.Dat1;
    //┴
  //│
  //○┐【主処理】
    //◇┐クライアントを起動
    if (!ENA_CLIENT(MY_NET, false)) {
      //├┐（未接続の場合）
        //○クライアントを起動（成功するまでの待ち時間を指定）
        MY_NET.setTimeout(LIMIT::TIME_CONNECT);
        if (!MY_NET.connect(transIP.c_str(), MY_PORT)) {ctx.bridge.MSG = RCD::Trn1Err; return;}
        //│＼（接続に失敗した場合）
        //│ ○完了MSGにエラーCDをセット
        //│ ▼終了：早期リターンする
        //│
        //○スロットを初期化
        SLOT_INI(TBL[0]);
        //│
        //●受信タスクを登録
        RUN_TASK(ADP_ID); // 並列処理で登録
        //┴
      //└┐（その他）
        //┴
    } /* if */
    //│
    //○退避したフレームでリクエスト(非同期でデータ受信)
    MY_NET.print(ctx.bridge.Frame);
    //┴
  //│
  //○┐【後処理】
  //┴┴
  } /* TRANS() */
#endif /* ➡ブリッジ */
//############################

//========================================================
//§ハンドル前処理
//========================================================
  //─────────────────
  // 前処理(一般)
  //─────────────────
  bool SETUP_NORMAL() override final {
    //●WiFi接続状況を確認
    return !devWiFi::isConnect(true);
  } /* SETUP_NORMAL() */

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // コンストラクタ：非同期キュー型＋スロット型
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  AdapterTCP(MmpContext& argCtx):
  AdapterBase<WiFiClient>(argCtx),      // 接続識別子：WiFiClient
  AdapterQueueBase<WiFiClient>(argCtx), // 接続識別子：WiFiClient
  AdapterSlotBase<WiFiClient>(argCtx)   // 接続識別子：WiFiClient
  {
  //┬
  //○┐【前処理】
    //●WiFi接続状況を確認
    if (!devWiFi::isConnect(true)) return;
    //┴
  //│
//--------------------------
//➡ブリッジ：[スロット]が単一，[サーバ][受信タスク]が不要
#if (MODE == MODE_BRIDGE)
  //○┐【主処理】
    //○接続管理TBLを作成
    SLOTs = 1;
    TBL   = new T_SLOT[SLOTs];
    //┴
  //│
  //○┐【後処理】
    //○メッセージ表示
    Log::prtln(" [OK] TCP");
  //┴┴
//➡ブリッジ以外：[スロット]が複数，[サーバ][受信タスク]が必要
#else
  //○┐【主処理】
    //○接続管理TBLを作成
    SLOTs = 10;
    TBL   = new T_SLOT[SLOTs];
    //│
    //○サーバを起動
    MY_NET = new WiFiServer(MY_PORT);
    MY_NET->begin();
    //│
    //●受信タスク（並列処理）を登録
    RUN_TASK(ADP_ID);
    //┴
  //│
  //○┐【後処理】
    //○メッセージ表示
    char msg[128];
    snprintf(msg, sizeof(msg), " [OK] TCP RAW    (PORT %d)", MY_PORT);
    Log::prtln(String(msg));
  //┴┴
#endif /* ➡ブリッジ｜➡ブリッジ以外 */
//--------------------------
  } /* constractor AdapterTCP() */

}; /* class AdapterTCP */