// filename : connection/member/_api1_.h
//========================================================
// 抽象基底クラス：非同期キュー付き経路アダプタ
//--------------------------------------------------------
// Ver 1.2.3 (2026/09/06)
//========================================================
#ifndef ADAPTER_QUEUE_BASE_H
#define ADAPTER_QUEUE_BASE_H

#include "_index_.h"
#include <queue>
#include <mutex>

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
  virtual void SEND_CONN(T argConn) = 0;
  virtual int  getAdpId() const = 0;

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
  virtual void handle_begin() {}
  virtual void handle_end()   {}

  //━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━
  void handle() override final {
    //┬
    //○ポーリング前処理（例: WebSocketの loop() など）
    handle_begin();
    //│
    //◎┐キューの自動消化とルーティング
    QueueItem popDat;
    while (popQueue(popDat)) {
      //│＼（キューが空の場合）
      //│ ▼完了：ルーティングを終了
      //│
      //○フレームの状態を確認
      if (popDat.frame.startsWith("#")) {
        SEND_CONN(popDat.conn);
        continue;
      }
      //│＼（エラーが発生している場合）
      //│ ●エラーを強制レスポンス
      //│ ▽次へ：次のキューを走査
      //│
      //●コマンドを実行
      mode::RUN(getAdpId(), popDat.frame);
      //│
      //●実行結果をレスポンス
      if (ctx.sysMode == MODE_MAIN) SEND_CONN(popDat.conn);
      //┴
    } /* END-while */
    //│
    //○ポーリング後処理
    handle_end();
    //┴
  } /* handle() */
}; /* class AdapterQueueBase */

#endif // ADAPTER_QUEUE_BASE_H