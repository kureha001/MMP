// filename : Dep_Connect/adapter/TCP_TURBO.cpp
//========================================================
// 接続部門／担当：TCP(高速型)
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/24)
//========================================================

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス：基本型
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class  AdapterTCP:
public AdapterBase<WiFiClient> // 接続識別子：WiFiClient
{
private:
//========================================================
//§基本情報
//========================================================
  //─────────────────
  // 一般情報
  //─────────────────
  const int ADP_ID = ADP_ID_TCP;

  //─────────────────
  // サービス関連情報
  //─────────────────
//--------------------------
//➡ブリッジ：クライアント
#if (MODE == MODE_BRIDGE)
  WiFiClient  MY_NET           ; // WiFiクライアント(実体)
//➡ブリッジ以外：サーバ
#else
  WiFiServer* MY_NET  = nullptr; // WiFiサーバ(ポインタ)
  int         MY_PORT = 8081   ; // ポート番号
#endif
//--------------------------

//========================================================
//§接続管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
  int SLOTs = 0; // コンストラクタで決定
  struct T_SLOT{
    bool       used = false;
    WiFiClient CONN; // TCP接続の実体
  };
  T_SLOT* TBL = nullptr;

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
  // 空きSID取得
  //----------------------------------
  // 戻り値：スロットID
  // ・0,1,2...：空きスロットのID
  // ・-1：空きスロットが無い
  //─────────────────
  int SLOT_GET_FREE() {
    //┬
    //◎┐先頭から走査
    for (int ID = 0; ID < SLOTs; ID++) {
    //│＼（全スロットを走査し終えた場合）
    //│ ▽中断：ループ処理を中断
    //│
    //○スロットを確認
    if (!TBL[ID].used) return ID;
    //│＼（未使用の場合）
    //│ ▼返却：当該スロットIDを返す
    } /* for */
    //│
    //▼返却：エラーCD(空きスロットがない)
    return -1;
    //┴
  } /* SLOT_GET_FREE() */

  //─────────────────
  // 動的アタッチ
  //----------------------------------
  // 戻り値 ：処理結果（論理値）
  // ・false：正常
  // ・true ：異常
  //─────────────────
  bool SLOT_ATTACH(){
//--------------------------
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
    //【ブリッジ（クライアント）モードの場合】
    // MY_NET 自身がアクティブであれば 0番スロットに直接割り当てる
    if (!MY_NET.connected()) return false;

    if (!TBL[0].used) {
      SLOT_INI(TBL[0]);
      TBL[0].used = true;
      TBL[0].CONN = MY_NET; // クライアント接続をスロット0にセット
      TBL[0].CONN.setNoDelay(true);
    }
    return false;
//➡ブリッジ以外
#else
    //┬
    //◎┐未管理のTCP接続をMMP管理対象へ登録する
    while (true) {
    //│
    //○新規のTCP接続を取得
    WiFiClient newConn = MY_NET->available(); // WiFiサーバ(ポインタ)
    if (!newConn) return false;
    //│＼（あらたな接続がない場合）
    //│ ▼返却：正常
    //│
    //●空きスロットを探す
    int ID = SLOT_GET_FREE();
    if (ID < 0) return true;
    //│＼（空きスロットがない）
    //│ ▼返却：異常
    //│
    //●スロットを初期化
    SLOT_INI(TBL[ID]);
    //│
    //○スロットに新規接続を登録
    TBL[ID].used = true   ; // 使用中
    TBL[ID].CONN = newConn; // TCP接続(実体)を登録
    TBL[ID].CONN.setNoDelay(true); // TCPパケット遅延制御
    //┴
    } //* while */
#endif /* ➡ブリッジ｜➡ブリッジ以外 */
//--------------------------
  } /* SLOT_ATTACH() */

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
    //○クライアントにレスポンス
    if (argConn.connected()) argConn.print(ctx.resMSG);
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
#endif /* ➡ブリッジ以外 */
//--------------------------
  } /* SEND_CONN() */

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
    //◇┐クライアントを起動
    if (!MY_NET.connected()) {
      //├┐（未接続の場合）
        //○TCPクライアントを起動
        String   ip   = ctx.bridge.Dat1;
        uint16_t port = (uint16_t)ctx.bridge.Dat2.toInt();
        MY_NET.setTimeout(2000);
        if (!MY_NET.connect(ip.c_str(), port)) {ctx.bridge.MSG = RCD::Trn1Err; return;}
        //│＼（接続に失敗した場合）
        //│ ○完了MSGにエラーCDをセット
        //│ ▼終了：早期リターンする
        //┴
      //└┐（その他）
        //┴
    } /* if */
    //│
    //○退避したフレームでリクエスト
    MY_NET.print(ctx.strFrame);
    //│
    //●フレームに受信データをセット
    String ctx.strFrame = adpFnStream::GET_FRAME(MY_NET);
    if (ctx.strFrame == "") ctx.bridge.MSG == RCD::Trn2Err;
    //│＼（接続に失敗した場合）
    //│ ○完了MSGにエラーCDをセット
    //│ ▼終了：早期リターンする
    //│
    //○終了MSGに[フレーム内容]をセット
    ctx.bridge.MSG = ctx.strFrame;
    //┴
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
    //┬
    //●接続管理スロットを動的アタッチ
    bool Result = SLOT_ATTACH();
    //│
    //●WiFi接続状況を確認
    return !devWiFi::isConnect(true);
    //┴
  } /* SETUP_NORMAL() */

//############################
//➡ブリッジ
  //─────────────────
  // 前処理(ブリッジ／スレーブ)
  //─────────────────
  bool SETUP_BRIDGE() override final {
    //┬
    //●進行判定を取得
    bool retGo = modeBridge::TRANS_BEGIN(ADP_ID);
    //│＼（[依頼中]ではない場合）
    //│ ▼終了：早期リターンする（進行OK/NG)
    //│
    //○処理対象を確認
    if (ADP_ID != ctx.bridge.adpID || ctx.bridge.Stat != BSTAT::REQ)
    return retGo;
    //│＼（[スレーブ以外]または[依頼中以外]場合）
    //│ ▼終了：早期リターンする（進行判定)
    //│
    //○進行状況を[処理中]にセット
    ctx.bridge.Stat = BSTAT::BUSY;
    //│
    //●転送を実施
    TRANS();
    //│
    //●転送処理（終了）...進行状況を[処理済]に遷移
    TmodeBridge::TRANS_END()
    //│
    //▼返却：正常終了(進行判定)
    return retGo;
    //┴
  } /* SETUP_BRIDGE() */
#endif /* ➡ブリッジ */
//############################

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // コンストラクタ：基本型
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  AdapterTCP(MmpContext& argCtx):
  AdapterBase<WiFiClient>(argCtx) // 接続識別子：WiFiClient
  {
  //┬
  //○┐【前処理】
    //●WiFi接続状況を確認
    if (!devWiFi::isConnect(true)) return;
    //┴
  //│
//--------------------------
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
    //┬
    //●接続管理TBLを作成
    SLOTs = 1;
    TBL   = new T_SLOT[SLOTs];
    //│
    //○メッセージ表示
    Serial.println(" [OK] TCP");
    //┴
//➡ブリッジ以外
#else
    //┬
    //●接続管理TBLを作成
    SLOTs = 10;
    TBL   = new T_SLOT[SLOTs];
    //│
    //○サービス資源を生成
    MY_NET = new WiFiServer(MY_PORT);
    MY_NET->begin();
    //│
    //○メッセージ表示
    Serial.printf(" [OK] TCP Hi-Speed (PORT = %d)\n", MY_PORT);
    //┴
#endif /* ➡ブリッジ｜➡ブリッジ以外 */
//--------------------------
  } /* constractor AdapterTCP() */

  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  void handle() override {
  //┬
  //○┐【前処理】
    //●ハンドル前処理(一般)
    if (SETUP_NORMAL()) return;
    //│＼（異常の場合）
    //│ ▼終了：早期リターンする
    //┴
  //│
  //○┐主処理
    //◎┐スロットを走査
    for (int ID = 0; ID < SLOTs; ID++) {
      //│＼（最後のスロットに達した場合）
      //│ ▼完了：走査を終了
      //│
      //○┐スロットの状態を確認
        //│
        //○接続状況を確認
        if (!TBL[ID].CONN.connected()) {
        //│＼（切断の場合）
            //○スロットを初期化する
            //▽次へ：次のスロットの走査へ進む
            SLOT_INI(TBL[ID]);
            continue;
        } /* if */
        //│
        //○使用状況を確認
        if (!TBL[ID].used) continue;
        //│＼（未使用のスロットの場合）
        //│ ▽次へ：次のスロットの走査へ進む
        //┴
      //│
      //●レスポンスを取得
      String retFrame = adpFnStream::GET_FRAME(TBL[ID].CONN);
      if (retFrame == "") continue;
      //│＼（受信データがない場合）
      //│ ▽次へ：次のスロットの走査へ進む
      //│
      //●コンテキストを初期化
      adpFnBase::SETUP_CTX(ADP_ID, retFrame);
//--------------------------
//➡メイン
#if   (MODE == MODE_MAIN)
      //●コマンドを実行
      //●実行結果をレスポンス
      modeMain::RUN();
      SEND_CONN(TBL[ID].CONN);
      //┴
//➡サブ
#elif (MODE == MODE_SUB)
      //●コマンドを実行
      //●実行結果をレスポンス
      modeSub::RUN();
      SEND_CONN(TBL[ID].CONN);
      //┴
//➡ブリッジ
#elif (MODE == MODE_BRIDGE)
      //○スレーブを確認
      if (ADP_ID != ctx.bridge.adpID) continue;
      //│＼（アダプタが[対象外]の場合）
      //│ ▽次へ：次のキューを走査
      //│
      //◇┐[依頼中→処理中]に遷移
      if(ctx.bridge.Stat == BSTAT::REQ ) {
        //├┐（[依頼中]の場合）
          //○進行状況を[処理中]にセット
          //●転送を実施
          //●転送処理（終了）...進捗状況を[処理済]に遷移
          ctx.bridge.Stat = BSTAT::BUSY;
          TRANS();
          modeBridge::TRANS_END();
          //┴
        //└┐（その他）
          //┴
      } /* if */
      //│
      //◇┐[処理中→処理済]に遷移
      if (ctx.bridge.Stat == BSTAT::BUSY) {
        //├┐（[処理中] の場合）
          //○レスポンスMSGに[完了MSG内容]をセット
          //○進行状況を[処理済]にセット
          //▼終了：早期リターンする ※1件ずつ処理
          ctx.resMSG      = ctx.bridge.MSG;
          ctx.bridge.Stat = BSTAT::DONE;
          return;
        //└┐（その他）
          //┴
      } /* if */
      //┴
  //│
  //○┐【後処理】
  //┴┴
#endif /* ➡メイン｜➡サブ｜➡ブリッジ */
//--------------------------
    } /* for */
    //┴
  } /* handle() */

}; /* class AdapterTCP */