// filename : Dep_Connect/template/_api1.h
//========================================================
// 接続部門／業務設計：抽象基底クラス（非同期キュー型）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/24)
//========================================================
#ifndef CONN_ADP_API1_H
#define CONN_ADP_API1_H
#pragma once

//========================================================
// モード処理係（前方宣言）
//========================================================
namespace modeMain   { void RUN(); }
namespace modeSub    { void RUN(); }
namespace modeBridge { void RUN(); }

//########################################################
template <typename T>
class AdapterQueueBase :
  virtual public AdapterBase<T>
//########################################################
{
public:
  using AdapterBase<T>::AdapterBase;

protected:
  //━━━━━━━━━━━━━━━━━
  // 純粋仮想関数（派生クラスで実装）
  //━━━━━━━━━━━━━━━━━
  virtual int  getAID() const = 0;

  //━━━━━━━━━━━━━━━━━
  // 非同期キュー処理
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // 基本情報
    //─────────────────
    struct QueueItem {
      T      conn  ; // 接続識別子 (uint8_t, WiFiClient, String 等)
      String frame ; // 受信データフレーム
      int    slotID; // スロットID
    };
    std::queue<QueueItem> rxQueue;
    std::mutex            queueMutex;

    //─────────────────
    // キューへの追加
    //─────────────────
    void pushQueue(
      const T&      conn , // 接続識別
      const String& frame, // フレーム
      const int     SID    // スロットID ※対象外はダミー値をセット
    ) {
      String msg = "["+ String(getAID()) + "] push:" + frame;
      Log::Outln(msg);

      if (frame.length() < 1) return;
      std::lock_guard<std::mutex> lock(queueMutex);
      rxQueue.push({conn, frame, SID});
    } /* pushQueue() */

    //─────────────────
    // キューからの取り出し
    //─────────────────
    bool popQueue(QueueItem& outItem) {
      //┬
      //○┐【前処理】
        //┴
      //│
      //○┐【主処理】
        std::lock_guard<std::mutex> lock(queueMutex);
        if (rxQueue.empty()) return false;
        //│
        outItem = rxQueue.front();
        rxQueue.pop();
        //┴
      //│
      //○┐【後処理】
        String msg = "[" + String(getAID()) + "] pop :" + outItem.frame;
        Log::Outln(msg);
        //│
        return true;
      //┴┴
    } /* popQueue() */

  //━━━━━━━━━━━━━━━━━
  // 前処理(ブリッジ用)
  //━━━━━━━━━━━━━━━━━
//############################
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
  bool handle_SetupBridge() override {
    //┬
    //○┐【前処理】
      //○(処理なし)
      //┴
    //│
    //○┐【主処理】
      //●スタートアップ(スレーブ用)を実施
      bool retGo = modeBridge::SLAVE(getAID(), [this](){this->trans();});
      //┴
    //│
    //○┐【後処理】
      //▼返却：正常終了(進行判定)
      return retGo;
    //┴
  } /* handle_SetupBridge() */
#endif /* ブリッジ */
//############################

//========================================================
//§公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━
  void handle() override final {
    //┬
    //○┐【前処理】
      //●一般用
      if (this->handle_Setup()) return;
//-----------------------------------------
//➡ブリッジ
#if (MODE == MODE_BRIDGE)
      //│
      //●ブリッジ用
      if (handle_SetupBridge()) return;
#endif /* ブリッジ */
//-----------------------------------------
      //┴
    //│
    //○┐【主処理】
      //◎┐キューの自動消化とルーティング
      QueueItem popDat;
      while (popQueue(popDat)) {
        //│＼（キューが空の場合）
        //│ ▼完了：ルーティングを終了
        //│
        //●コンテキストを初期化
        adpFnBase::SETUP_CTX(getAID(), popDat.frame);
        //│
//-----------------------------------------
//➡ブリッジ以外：ブリッジはエラーCDもそのまま扱う
#if (MODE != MODE_BRIDGE)
        //○フレームの状態を確認
        if (ctx.strFrame.startsWith("#")) {this->SEND_CONN(popDat.conn); continue;}
        //│＼（フレームが[エラーCD]の場合）
        //│ ●ブリッジ元にレスポンス
        //│ ▽次へ：次のキューを走査
#endif /* ブリッジ以外 */
//-----------------------------------------

//-----------------------------------------
//➡メイン
#if   (MODE == MODE_MAIN)
        //●コマンドを実行
        //●実行結果をレスポンス
        modeMain::RUN();
        this->SEND_CONN(popDat.conn);
        //┴
//➡サブ
#elif (MODE == MODE_SUB)
        //●コマンドを実行
        //●実行結果をレスポンス
        modeSub::RUN();
        this->SEND_CONN(popDat.conn);
        //┴
//➡ブリッジ
#elif (MODE == MODE_BRIDGE)
        //◇┐[待機中→依頼中]に遷移
        if (ctx.bridge.Stat == BSTAT::IDLE && ctx.adpID == ADP_ID_UART) {
          //├┐（[待機中]かつ[マスタ]の場合）
            //●ブリッジ処理を実行
            modeBridge::RUN(popDat.slotID, popDat.frame);
            if (ctx.resMSG != "") {this->SEND_CONN(popDat.conn); return;}
            //│＼（[内部コマンド応答済][エラーあり]の場合）
            //│ ○クライアントにレスポンス
            //│ ▼終了：早期リターン
            //│
            //○進捗状況を[依頼中]に遷移
            //▼終了：早期リターン ※1件ずつ処理
            Log::Outln("1.待機中→依頼中");
            ctx.bridge.Stat  = BSTAT::REQ;
            return;

        } else if (ctx.bridge.Stat == BSTAT::BUSY && getAID() == ctx.bridge.adpID) {
          //├┐（[処理中]かつ[スレーブ] の場合）
            //○レスポンスMSGに[フレーム内容]をセット
            //○進行状況を[処理済]に遷移
            //▼終了：早期リターン ※1件ずつ処理
            Log::Outln("3.処理中→処理済(キュー)");
            ctx.resMSG      = ctx.strFrame;
            ctx.bridge.Stat = BSTAT::DONE;
            return;
          //└┐（その他）
            //○なにもしない（キューは空振りになる）
            Log::Outln("0.空振り(キュー)");
            //┴
        } /* switch */
        //┴
#endif /* メイン，サブ，ブリッジ */
//-----------------------------------------
      //┴
    } /* while */
    //│
    //○┐【後処理】
      //○（処理なし）
    //┴┴
  } /* handle() */

}; /* class AdapterQueueBase */
#endif