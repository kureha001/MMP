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
    ctx.resMSG  = ""  ; // レスポンスメッセージ
    ctx.cmdPath = ""  ; // コマンドパス
    ctx.authCD  = ""  ; // 認証コード
    ctx.accID   = -1  ; // アクセスID
  }

//========================================================
// レスポンス
//========================================================
  //─────────────────
  // クライアントにレスポンス
  // ※AdapterQueueBaseをオーバーライド
  //─────────────────
  void SEND_CONN_BRIDGE() {
    //┬
    //○メッセージをUSB(CDC)へレスポンス
    Serial.print(ctx.resMSG.c_str());
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */

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
      //●コンテキストを初期化
      setupCTX(popDat.frame);
      //│
#if   (MODE == MODE_MAIN)
      //●コマンドを実行
      //●実行結果をレスポンス
      modeMain::RUN();
      SEND_CONN(popDat.conn);
      //┴

#elif (MODE == MODE_SUB)
      //●コマンドを実行
      //●実行結果をレスポンス
      modeSub  ::RUN();
      SEND_CONN(popDat.conn);
      //┴

#elif (MODE == MODE_BRIDGE)
      //●ブリッジ・マスタの場合：コマンドを実行
      //●ブリッジ・スレーブかエラーがある場合：クライアントにレスポンス
      if (ctx.adpID == ADP_ID_UART) modeBridge::RUN();
      if (ctx.adpID != ADP_ID_UART || ctx.resMSG != "") SEND_CONN_BRIDGE();
#endif
    } /* END-while */
    //│
    //○ポーリング後処理
    handle_end();
    //│
#if (MODE == MODE_BRIDGE)
    //○転送依頼を確認
    if (ctx.transOn && ctx.adpID == ctx.transID) {
    //│＼（自分宛に転送依頼がきている場合）
        //○転送依頼フラグをオフ
        //●転送を受付
        //▼終了：早期リターン
        ctx.transOn = false;
        trans();
        return;
    //┴
      } /* END-if */
#endif
    //┴
  } /* handle() */
}; /* class AdapterQueueBase */

#endif