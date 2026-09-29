// filename : Dep_Connect/template/apiQueue.h
//========================================================
// 接続部門／業務設計：抽象基底クラス（非同期キュー型）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/29)
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
        adpFnBase::SETUP_CTX(this->MY_AID, popDat.frame);
        //│
  //──────────────────
  //➡ブリッジ以外
  //・ブリッジではエラーCDもそのまま扱う
  #if (MODE != MODE_BRIDGE)
  //-----------------------------------------
        //○フレームの状態を確認する
        if (ctx.base.Frame.startsWith("#")) {this->SEND_CONN(popDat.conn); continue;}
        //│＼（フレームが[エラーCD]の場合）
        //│ ●ブリッジ元にレスポンス
        //│ ▽次へ：次のキューを走査
  //------------------------------------
  #endif //➡ブリッジ以外
  //──────────────────
        //│
  //──────────────────
  //➡メイン
  #if   (MODE == MODE_MAIN)
  //------------------------------------
        //●コマンドを実行する
        //●接続元にレスポンスMSGを送信する
        modeMain::RUN();
        this->SEND_CONN(popDat.conn);
        //┴
  //──────────────────
  //➡サブ
  #elif (MODE == MODE_SUB)
  //------------------------------------
        //●コマンドを実行する
        //●接続元にレスポンスMSGを送信する
        modeSub::RUN();
        this->SEND_CONN(popDat.conn);
        //┴
  //──────────────────
  //➡ブリッジモード：マスタ(UART)
  #elif UART
  //------------------------------------
        //●ブリッジ処理を実行
        modeBridge::RUN(popDat.QID, popDat.frame);
        //│
        //◇┐進捗開始／即時応答 で分岐処理する
        if (ctx.base.Msg == "") {
          //├┐（即時応答ではない場合）
            //○進捗状況を[依頼中]に遷移する
            Log::Outln("1.待機中→依頼中");
            ctx.trans.Stat  = BSTAT::REQ;
            //┴
          } else this->SEND_CONN(popDat.conn);
          //└┐（その他）
            //●接続元にレスポンスMSGを送信する
            //┴
        //│
        //▼終了：早期リターンする ※1件ずつ処理
        return;
  //──────────────────
  //➡ブリッジモード：スレーブ
  #else
  //------------------------------------
        //○レスポンスMSGに[フレーム内容]をセットする
        //○進行状況を[処理済]に遷移する
        //▼終了：早期リターンする ※1件ずつ処理
        Log::Outln("3.処理中→処理済(キュー)");
        ctx.base.Msg   = ctx.base.Frame;
        ctx.trans.Stat = BSTAT::DONE;
        return;
  //------------------------------------
  #endif //➡メイン｜➡サブ｜➡ブリッジ
  //──────────────────
      //┴
    } //～while
    //│
    //○┐【後処理】
      //○（処理なし）
    //┴┴
  } /* handle() */

}; /* class AD_API_Queue */