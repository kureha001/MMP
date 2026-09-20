// filename : Dep_Connect/adapter/_template_/_api1_.h
//========================================================
// 接続部門／業務課／作業標準：抽象基底クラス（非同期キュー型）
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/20)
// ・ブリッジモードの不具合対応
// ・コンテキスト初期化を共通へ移動
// ・転送処理の共通部を[modeBridge]に部品化
// ・[handle_Begin()]を[AdapterBase]へ移動
//========================================================
#define CONN_ADP_API1_H
#define CONN_ADP_API1_H

#include "_api0_.h"
#include <queue>
#include <mutex>

//========================================================
// モード処理係（前方宣言）
//========================================================
namespace modeMain   { void RUN(); }
namespace modeSub    { void RUN(); }
namespace modeBridge { void RUN(); }

//========================================================
// 作業標準：抽象基底クラス（非同期キュー型）
//========================================================
template <typename T>
class AdapterQueueBase : public AdapterBase {
protected:
  //━━━━━━━━━━━━━━━━━
  // キュー要素構造体
  //━━━━━━━━━━━━━━━━━
  struct QueueItem {
    T      conn  ; // 接続識別子 (uint8_t, WiFiClient, String 等)
    String frame ; // 受信データフレーム
    int    slotID; // スロットID
  };

private:
  std::queue<QueueItem> rxQueue;
  std::mutex            queueMutex;

  //━━━━━━━━━━━━━━━━━
  // 純粋仮想関数（派生クラスで実装）
  //━━━━━━━━━━━━━━━━━
  virtual int  getAID() const = 0;
  virtual void SEND_CONN(T argConn) = 0;

public:
  using AdapterBase::AdapterBase;

  //━━━━━━━━━━━━━━━━━
  // キューへの追加
  //━━━━━━━━━━━━━━━━━
  void pushQueue(
    const T&      conn , // 接続識別
    const String& frame, // フレーム
    const int     SID    // スロットID ※対象外はダミー値をセット
  ) {
    if (frame.length() < 1) return;
    std::lock_guard<std::mutex> lock(queueMutex);
    rxQueue.push({conn, frame, SID});
  }

  //━━━━━━━━━━━━━━━━━
  // キューからの取り出し
  //━━━━━━━━━━━━━━━━━
  bool popQueue(QueueItem& outItem) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (rxQueue.empty()) return false;
    outItem = rxQueue.front();
    rxQueue.pop();
    return true;
  }

//############################
//# 転送処理はブリッジのみ
//############################
#if (MODE == MODE_BRIDGE)
  //━━━━━━━━━━━━━━━━━
  // 前処理(ブリッジ用)
  //━━━━━━━━━━━━━━━━━
  bool handle_SetupBridge() override {
    //┬
    //○┐前処理
      //●進行判定を取得
      bool retGo = modeBridge::TRANS_BEGIN(getAID());
      //│
      //○スレーブを確認
      if (getAID() != ctx.bridge.adpID) return retGo;
      //│＼（スレーブではない場合）
      //│ ▼終了：早期リターン（進行判定)
      //│
      //○進行状況を確認
      if (ctx.bridge.Stat != BSTAT::REQ) return retGo;
      //│＼（[依頼中]ではない場合）
      //│ ▼終了：早期リターン（進行判定)
      //┴
    //│
    //○┐主処理
      //○進行状況を[処理中]に遷移
      ctx.bridge.Stat = BSTAT::BUSY;
      //│
      //●転送を実施
      trans();
      //│
      //●転送処理（終了）...即時応答は[処理済]に遷移される
      modeBridge::TRANS_END();
      //┴
    //│
    //○┐後処理
      //▼返却：正常終了(進行判定)
      return retGo;
    //┴
  } /* handle_SetupBridge() */
#endif
//############################

  //━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━
  void handle() override final {
    //┬
    //○┐前処理
      //●一般用
      //●ブリッジ用
      if (handle_Setup()) return;
      if (handle_SetupBridge()) return;
      //┴
    //│
    //○┐主処理
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
//➡ブリッジ以外
//　※ブリッジではエラーCDもそのまま扱う
//-----------------------------------------
#if (MODE != MODE_BRIDGE)
        //○フレームの状態を確認
        if (ctx.strFrame.startsWith("#")) {SEND_CONN(popDat.conn); continue;}
        //│＼（フレームが[エラーCD]の場合）
        //│ ●ブリッジ元にレスポンス
        //│ ▽次へ：次のキューを走査
#endif
//-----------------------------------------
//-----------------------------------------
//➡メイン
//-----------------------------------------
#if   (MODE == MODE_MAIN)
        //●コマンドを実行
        //●実行結果をレスポンス
        modeMain::RUN();
        SEND_CONN(popDat.conn);
        //┴
//-----------------------------------------
//➡サブ
//-----------------------------------------
#elif (MODE == MODE_SUB)
        //●コマンドを実行
        //●実行結果をレスポンス
        modeSub::RUN();
        SEND_CONN(popDat.conn);
        //┴
//-----------------------------------------
//➡ブリッジ
//-----------------------------------------
#elif (MODE == MODE_BRIDGE)
        //◇┐[待機中→依頼中]に遷移
        if (ctx.bridge.Stat == BSTAT::IDLE && ctx.adpID == ADP_ID_UART) {
          //├┐（[待機中]かつ[マスタ]の場合）
            //●ブリッジ処理を実行
            modeBridge::RUN(popDat.slotID, popDat.frame);
            if (ctx.resMSG != "") {SEND_CONN(popDat.conn); return;}
            //│＼（[内部コマンド応答済][エラーあり]の場合）
            //│ ○クライアントにレスポンス
            //│ ▼終了：早期リターン
            //│
            //○進捗状況を[依頼中]に遷移
            //▼終了：早期リターン ※1件ずつ処理
            ctx.bridge.Stat  = BSTAT::REQ;
            return;

        } else if (ctx.bridge.Stat == BSTAT::BUSY && getAID() != ctx.bridge.adpID) {
          //├┐（[処理中]かつ[スレーブ] の場合）
            //○レスポンスMSGに[フレーム内容]をセット
            //○進行状況を[処理済]に遷移
            //▼終了：早期リターン ※1件ずつ処理
            ctx.resMSG      = ctx.strFrame;
            ctx.bridge.Stat = BSTAT::DONE;
            return;
          //└┐（その他）
            //○なにもしない（キューは空振りになる）
            //┴
        } /* END-switch */
        //┴
#endif
//-----------------------------------------
      //┴
    } /* END-while */
    //│
    //○┐後処理
      //○（処理なし）
      //┴
    //┴
  } /* handle() */

}; /* class AdapterQueueBase */