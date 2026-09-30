// filename : Dep_Connect/template/apiQueue.h
//========================================================
// 接続部門／業務設計：抽象基底クラス（非同期キュー型）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/30)
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
    int    QID  ; // キューのスロットID
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

  //───────────────────────────
  // ブリッジ用(詳細)：スレーブ
  //------------------------------------------------------
  //【引数】
  //※デフォルトでは、スレーブ用に実装しておく。
  //※マスタ(UART)で利用する引数を用意しておく。
  //　UARTアダプタ側でオーバーライドする際に利用する。
  //・接続識別子：テンプレートのT型
  //・キューID：数値型
  //───────────────────────────
virtual void BRIDGE_PROCESS(T argConn, int argQID) {
#if (MODE == MODE_BRIDGE)
    if (this->MY_AID == ctx.trans.AID)
    {modeBridge::MOVE_BSTAT(BSTAT::DONE, "非同期応答");}
#endif
  } /* BRIDGE_PROCESS() */

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
      //│＼（異常の場合）
      //│ ▼終了：早期リターン
      //│
      //●進行判定を確認する（ブリッジ用）
      if (this->SETUP_BRIDGE()) return;
      //│＼（異常の場合）
      //│ ▼終了：早期リターン
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
        //●コンテキストを初期化する
        adpFnBase::SETUP_CTX(tmpAID, popDat.frame);
        //│
  //──────────────────
  //➡ブリッジモード
  //※SETUP_BRIDGE()でフィルタリングしているが、
  //  マスタ・スレーブが混在する為、
  //  改めて条件分岐させる必要がある。
  #if (MODE == MODE_BRIDGE)
  //------------------------------------
        BRIDGE_PROCESS(popDat.conn, popDat.QID);
        //▼終了：早期リターンする ※1件ずつ処理
        return;
  //──────────────────
  //➡ブリッジ以外
  #else
  //------------------------------------
        //○フレームの状態を確認する
        if (ctx.base.Frame.startsWith("#"))
        {this->SEND_RESULT(popDat.conn); continue;}
        //│＼（フレームが[エラーCD]の場合）
        //│ ●ブリッジ元にレスポンス
        //│ ▽次へ：次のキューを走査
        //│
    //──────────────────
    //➡メイン
    #if   (MODE == MODE_MAIN)
    //------------------------------------
        //●コマンドを実行する
        modeMain::RUN();
    //──────────────────
    //➡サブ
    #elif (MODE == MODE_SUB)
    //------------------------------------
        //●コマンドを実行する
        modeSub::RUN();
    //------------------------------------
    #endif //➡メイン｜➡サブ
    //──────────────────
        //│
        //●接続元に処理結果を送信する
        this->SEND_RESULT(popDat.conn);
        //┴
  //------------------------------------
  #endif //➡ブリッジ｜➡ブリッジ以外
  //──────────────────
      //┴
    } //～while
    //│
    //○┐【後処理】
      //○（処理なし）
    //┴┴
  } /* handle() */

}; /* class AD_API_Queue */