// filename : Dep_Connect/adapter/_template_/_api1_.h
//========================================================
// 接続部門／業務課／作業標準：抽象基底クラス（非同期キュー型）
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/09)
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
    T      conn;  // 接続識別子 (uint8_t, WiFiClient, String 等)
    String frame; // 受信データフレーム
  };

private:
  std::queue<QueueItem> rxQueue;
  std::mutex            queueMutex;

  //━━━━━━━━━━━━━━━━━
  // 純粋仮想関数（派生クラスで実装）
  //━━━━━━━━━━━━━━━━━
  virtual int  getAID() const = 0;
  virtual void SEND_CONN(T argConn) = 0;

  //━━━━━━━━━━━━━━━━━
  // コンテキストを初期化
  //━━━━━━━━━━━━━━━━━
  void setupCTX(String argFrame) {
    ctx.adpID    = getAID(); // アダプタID
    ctx.strFrame = argFrame; // フレーム
    if (!ctx.strFrame.endsWith ("!")) ctx.strFrame += "!";
    if (ctx.strFrame.startsWith("/")) ctx.strFrame.remove(0, 1);
    ctx.resMSG  = ""  ; // レスポンスMSG
    ctx.cmdPath = ""  ; // コマンドパス
    ctx.authCD  = ""  ; // 認証コード
    ctx.accID   = -1  ; // アクセスID
  }

public:
  using AdapterBase::AdapterBase;

  //━━━━━━━━━━━━━━━━━
  // キューへの追加
  //━━━━━━━━━━━━━━━━━
  void pushQueue(const T& conn, const String& frame) {
    if (frame.length() < 1) return;
    std::lock_guard<std::mutex> lock(queueMutex);
    rxQueue.push({conn, frame});
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
  // 前後処理用のフック関数
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // ポーリングの前処理
    //─────────────────
    virtual bool handle_Begin() {return false;}

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
#if (MODE != MODE_BRIDGE)
      //○フレームの状態を確認
      if (popDat.frame.startsWith("#")) {
        SEND_CONN(popDat.conn);
        continue;
      }
      //│＼（エラーが発生している場合）
      //│ ●エラーを強制レスポンス
      //│ ▽次へ：次のキューを走査
#endif
      //│
      //●コンテキストを初期化
      setupCTX(popDat.frame);
      //│

#if   (MODE == MODE_MAIN)
      //●コマンドを実行
      modeMain::RUN();
      //│
      //●実行結果をレスポンス
      SEND_CONN(popDat.conn);
      //┴

#elif (MODE == MODE_SUB)
      //●コマンドを実行
      modeSub  ::RUN();
      //│
      //●実行結果をレスポンス
      SEND_CONN(popDat.conn);
      //┴

#elif (MODE == MODE_BRIDGE)
      //◇┐ブリッジのマスタ／スレーブを処理
      if (ctx.adpID == ADP_ID_UART) {
        //├┐（マスタの場合）
          //◆┐ブリッジ処理を実行
          modeBridge::RUN();
          if (ctx.resMSG == "") ctx.bridge.Stat = 1;
            //├┐（通常コマンドの場合）
              //○進捗状況を[依頼中]にセット
              //┴
          else SEND_CONN(popDat.conn);
            //└┐（その他）
              //●ブリッジ元にレスポンス
              //┴
          //┴
      } else if(getAID() == ctx.bridge.adpID) {
        //├┐（スレーブの場合）
          //○ステータスを[処理済]にセット
          ctx.bridge.Stat = 3;
          //┴
        //└┐（その他：対象外）
          //▽次へ：スキップ ※できる限りゴミキューを削除
          continue;
          //┴
      } /* END-if */
      //│
      //▼終了：早期リターン ※関係するアダプタは1件だけ処理
      return; 
#endif
    } /* END-while */
    //│
#if (MODE == MODE_BRIDGE)
    //○┐転送処理を実施
      //│
      //○転送依頼を確認
      if (ctx.bridge.Stat != 1 || getAID() != ctx.bridge.adpID) return;
      //│＼（自分宛に転送依頼がない場合）
      //│ ▼終了：早期リターン
      //│
      //○進行状況を[処理中]にセット
      //●転送を実施
      ctx.bridge.Stat = 2;
      trans();
      //┴
#endif
    //┴
  } /* handle() */

}; /* class AdapterQueueBase */

#endif