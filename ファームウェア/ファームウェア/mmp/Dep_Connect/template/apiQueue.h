// filename : Dep_Connect/template/apiQueue.h
//========================================================
// 接続部門／業務設計：抽象基底クラス（非同期キュー型）
//--------------------------------------------------------
// Ver 1.4.0 (2026/10/01)
//========================================================
#pragma once

//========================================================
// モード処理係（前方宣言）
//========================================================
namespace modeMain   { void RUN(); }
namespace modeSub    { void RUN(); }
namespace modeBridge { void RUN(); }

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス：基本型
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
template <typename T>
class          AD_API_Queue:
virtual public AD_API<T>
{
public:
  using AD_API<T>::AD_API;

protected:
//========================================================
//§非同期キュー処理
//========================================================
  //───────────────────────────
  // 基本情報
  //───────────────────────────
  struct QueueItem {
    T      conn ; // 接続識別子 (uint8_t, WiFiClient, String 等)
    String frame; // 受信データフレーム
    int    SID  ; // 接続スロットID
  };
  std::queue<QueueItem> rxQueue;
  std::mutex            queueMutex;

  //───────────────────────────
  // リクエストDSにキューを追加する
  //───────────────────────────
  void pushQueue(
    const T&      conn , // 接続識別
    const String& frame, // フレーム
    const int     SID    // スロットID ※対象外はダミー値をセット
  ) {
    String msg = "["+ String(this->MY_AID) + "] push:" + frame;
    Log::Outln(msg);

    if (frame.length() < 1) return;
    std::lock_guard<std::mutex> lock(queueMutex);
    rxQueue.push({conn, frame, SID});
  } /* pushQueue() */

  //───────────────────────────
  // リクエストDSからのキューを取り出す
  //───────────────────────────
  bool popQueue(QueueItem& outItem) {
    //┬
    //○┐【前処理】
      //○先頭のキューを取り出す
      std::lock_guard<std::mutex> lock(queueMutex);
      //│
      //○キューを確認する
      if (rxQueue.empty()) return false;
      //│＼（キューが空の場合）
      //│ ▼終了：早期リターンする → キューなし
      //┴
    //│
    //○┐【主処理】
      //○先頭のキューを取り出す
      outItem = rxQueue.front();
      rxQueue.pop();
      //┴
    //│
    //○┐【後処理】
      //○ログ出力する
      String msg = "[" + String(this->MY_AID) + "] pop :" + outItem.frame;
      Log::Outln(msg);
      //│
      //▼返却：正常終了 → キューあり
      return true;
    //┴
  } /* popQueue() */

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  void handle() override final {
    //┬
    //○┐【前処理】
      //●進行判定を確認する（一般用）
      if (this->SETUP_NORMAL()) return;
      //│＼（進行不可の場合）
      //│ ▼終了：早期リターン
      //│
  //──────────────────
  //➡ブリッジモード
  //・マスタ／スレーブを通しての制御が必要
  #if MODE == MODE_BRIDGE
  //------------------------------------
      //●処理フローを制御する
      if (this->WORKFLOW()) return;
      //│＼（進行不可の場合）
      //│ ▼終了：早期リターン
  //------------------------------------
  #endif //➡ブリッジ
  //──────────────────
      //│
      //○通信アダプタIDを控える
      int tmpAID = this->MY_AID; 
      //┴
    //│
    //○┐【主処理】
      //◎┐キューに応答する
      QueueItem popDat;
      while (popQueue(popDat)) {
        //│＼（キューが空の場合）
        //│ ▼完了：ルーティングを終了
        //│
        //●コンテキストを更新する
        adpFnBase::SETUP_CTX(
          tmpAID,       // 通信アダプタID
          popDat.SID,   // キュー.接続スロットID
          popDat.frame  // キュー.フレーム
        );
  //──────────────────
  //➡ブリッジ以外
  //・エラー時は後続処理が不要
  //  （単純に接続元へ送信）
  #if MODE != MODE_BRIDGE
  //------------------------------------
        //│
        //○フレームの状態を確認する
        if (ctx.base.Frame.startsWith("#"))
        {this->SEND_RESULT(popDat.conn); continue;}
        //│＼（フレームが[エラーCD]の場合）
        //│ ●接続元に処理結果を送信する
        //│ ▽次へ：次のキューを走査する
  //------------------------------------
  #endif //➡ブリッジ以外
  //──────────────────
        //│
        //●主処理を実行する
        if (this->RUN_PROCESS(popDat.conn)) return;
        //│＼（進行不可の場合）
        //│ ▼終了：早期リターン
        //┴
    } //～while
    //│
    //○┐【後処理】
      //○（処理なし）
    //┴┴
  } /* handle() */

}; /* class AD_API_Queue */