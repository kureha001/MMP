// filename : Dep_Connect/adapter/_template_/_api1_.h
//========================================================
// 接続部門／業務課／作業標準：抽象基底クラス（非同期キュー型）
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/14)
// ・コンテキスト初期化を共通へ移動
// ・転送処理の共通部を[modeBridge]に部品化
// ・[handle_Begin()]を[AdapterBase]へ移動
//========================================================
#ifndef CONN_ADP_API1_H
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

  //━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━
  void handle() override final {
    //┬
    //●前処理
    if (handle_Begin()) return;
    //│
    //◎┐キューの自動消化とルーティング
    QueueItem popDat;
    while (popQueue(popDat)) {
      //│＼（キューが空の場合）
      //│ ▼完了：ルーティングを終了
      //│
//-----------------------------------------
//【ブリッジ】エラーでも中断せずにそのまま扱う
//-----------------------------------------
#if (MODE != MODE_BRIDGE)
      //○フレームの状態を確認
      if (popDat.frame.startsWith("#")) {
        SEND_CONN(popDat.conn);
        continue;
      }
      //│＼（エラーが発生している場合）
      //│ ●ブリッジ元にレスポンス
      //│ ▽次へ：次のキューを走査
#endif
      //│
      //●コンテキストを初期化
      adpFnBase::SETUP_CTX(getAID(), popDat.frame);
      //│
//-----------------------------------------
//【メイン】
//-----------------------------------------
#if   (MODE == MODE_MAIN)
      //●コマンドを実行
      modeMain::RUN();
      //│
      //●実行結果をレスポンス
      SEND_CONN(popDat.conn);
      //┴
//-----------------------------------------
//【サブ】
//-----------------------------------------
#elif (MODE == MODE_SUB)
      //●コマンドを実行
      modeSub  ::RUN();
      //│
      //●実行結果をレスポンス
      SEND_CONN(popDat.conn);
      //┴
//-----------------------------------------
//【ブリッジ】マスタ／スレーブで処理分け
//-----------------------------------------
#elif (MODE == MODE_BRIDGE)
      //◇┐ブリッジのマスタ／スレーブを処理
      if (ctx.adpID == ADP_ID_UART) {
        //├┐（マスタの場合）
          //○スロットIDを退避
          ctx.bridge.slotID = popDat.slotID;
          //│
          //◆┐ブリッジ処理を実行
          modeBridge::RUN();
          if (ctx.resMSG == "") ctx.bridge.Stat = BSTAT::REQ;
            //├┐（リクエストが[MMPコマンド転送]の場合）
              //○進捗状況を[依頼中]にセット
              //┴
          else SEND_CONN(popDat.conn);
            //└┐（その他：[特殊コマンド実行][システムコマンド実行][エラーあり]）
              //●ブリッジ元にレスポンス
              //┴
          //┴
      } else if(getAID() == ctx.bridge.adpID) {
        //├┐（スレーブの場合）
          //○ステータスを[処理済]にセット
          ctx.bridge.Stat = BSTAT::DONE;
          //┴
      } else continue;
        //└┐（その他：対象外アダプタの場合）
          //▽次へ：スキップ ※無駄なキューを破棄し続ける
      //│
      //▼終了：早期リターン ※関係アダプタでの処理は1件だけで良い
      return; 
#endif
//-----------------------------------------
    } /* END-while */
    //│
//-----------------------------------------
//【ブリッジ】キューがない時は転送に応答
//-----------------------------------------
#if (MODE == MODE_BRIDGE)
    //○転送処理を開始
    if (modeBridge::TARNS_BEGIN(getAID())) return
    //│＼（転送要求が無い場合）
    //│ ▼終了：早期リターン
    //│
    //●転送を実施
    trans();
    //│
    //○転送処理を終了
    modeBridge::TARNS_END();
#endif
//-----------------------------------------
    //┴
  } /* handle() */

}; /* class AdapterQueueBase */
#endif