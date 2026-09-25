// filename : Dep_Connect/adapter/UART.cpp
//========================================================
// 接続部門／担当：UART
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/24)
//========================================================

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス：非同期キュー型＋スロット型
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class  AdapterUART :
public AdapterQueueBase<Stream*>, // 接続識別子：Stream*
public AdapterSlotBase<Stream*>   // 接続識別子：Stream*
{
private:
//========================================================
//§基本情報
//========================================================
  //─────────────────
  // 一般情報
  //─────────────────
  const int ADP_ID = ADP_ID_UART;
  int getAID() const override { return ADP_ID; }

//========================================================
//§返信処理
//========================================================
  //─────────────────
  // クライアントにレスポンス
  //─────────────────
  void SEND_CONN(Stream* argConn) override final {
    //┬
    //○クライアントにレスポンス
    //●ログ出力
    argConn->print(ctx.resMSG);
    adpFnBase::SHOW_LOG();
    //┴
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
    //┴
  //│
  //○┐【主処理】
    //◎┐スロットを走査
    for (int qSID = 0; qSID < SLOTs; qSID++) {
      //│＼（最後のスロットに達した場合）
      //│ ▼完了：走査を終了
      //│
      //○┐キュー情報を用意
        //●受信データを求める
        String qFrame = adpFnStream::GET_FRAME(*(TBL[qSID].CONN));
        if (qFrame != "") continue;
        //│＼（受信データがない場合）
        //│ ▽次へ：次のスロットを走査
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
//§ハンドル前処理
//========================================================
//############################
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
  //─────────────────
  // 前処理(ブリッジ／マスタ)
  //─────────────────
  bool handle_SetupBridge() override final {
  //┬
  //○┐【前処理】
    //┴
  //│
  //○┐【主処理】
    //●スタートアップ(マスタ用)を実施
    bool retGo = modeBridge::MASTER(
      TBL[ctx.bridge.slotID].CONN,           // 退避済スロット
      [this](Stream* conn){SEND_CONN(conn);} // ラムダ式で包む
    );
    //┴
  //│
  //○┐【後処理】
    //▼返却：正常終了(進行判定)
    return retGo;
  //┴
  } /* handle_SetupBridge() */
#endif /* ➡ブリッジ */
//############################

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // コンストラクタ：非同期キュー型＋スロット型
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  AdapterUART(MmpContext& argCtx): 
  AdapterBase<Stream*>(argCtx),      // 接続識別子：Stream*
  AdapterQueueBase<Stream*>(argCtx), // 接続識別子：Stream*
  AdapterSlotBase<Stream*>(argCtx)   // 接続識別子：Stream*
  {
  //┬
  //○┐【前処理】
    //┴
  //│
  //○┐【主処理】
    //●接続管理TBLを作成
//--------------------------
//➡サブ
#if (MODE == MODE_SUB)
    SLOTs = 2;
    TBL = new T_SLOT[SLOTs];
    SLOT_SET(0, &Serial);
    SLOT_SET(1, &Serial2);
    String msg = " [OK] UART USB(CDC)+Serial#2";
//--------------------------
// ➡サブ以外
#else
    SLOTs = 3;
    TBL = new T_SLOT[SLOTs];
    SLOT_SET(0, &Serial);
    SLOT_SET(1, &Serial1);
    SLOT_SET(2, &Serial2);
    String msg = " [OK] UART / USB(CDC) + Serial#1,2";
#endif /* サブ,サブ以外 */
//--------------------------
    //│
    //●受信タスクを起動
    RUN_TASK(ADP_ID);
    //┴
  //│
  //○┐【後処理】
    //○メッセージ表示
    Log::prtln(msg);
  //┴┴
  } /* constractor AdapterUART() */

}; /* class AdapterUART */