// filename : adapter/WEB_Socket.cpp
//========================================================
// 通信アダプタ：ＷＥＢ Ｓｏｃｋｅｔ
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/03) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■追加ライブラリ：WebSockets by Markus Sattler
  #include <WebSocketsServer.h>
  #include "_API_.h"
  //┴
//┴

//########################################################
//# クラス
//########################################################
class AdapterWEB_Socket : public AdapterBase {
private:
//========================================================
// Ａ．アダプタの基本
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // ステータス
    //─────────────────
    const String ADP_ID  = "WSOC"; // アダプタID

    //─────────────────
    // 使用するサービス
    //─────────────────
    static WebSocketsServer* ADP_SRV         ; // WebSocketサーバ
    static int               SRV_PORT        ; // ポート番号

//========================================================
// Ｂ．レスポンス
//========================================================
  void SEND_CONN(uint8_t argConn){
    //┬
    //○メッセージをレスポンス
    if (ADP_SRV) ADP_SRV->sendTXT(argConn, ctx.resMSG.c_str());
    //│
    //●ログ出力
    adpBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */

//========================================================
// Ｃ．リクエスト・キュー管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
  struct myQueue {
    uint8_t CONN ; // アクセス資源(クライアント番号)
    String  FRAME; // 受信バッファ
  };
  static std::queue<myQueue> QUEUE      ; // キューバッファ
  static std::mutex          QUEUE_MUTEX; // 別スレッドとの衝突回避用のロック
 
  //─────────────────
  // キューの取出
  //----------------------------------
  // 引数：
  // ・キュー受取用の変数
  //----------------------------------
  // 戻り値：キューの有無（論理値）
  // ・true ：あり
  // ・false：なし
  //─────────────────
  bool popQueue(myQueue &argData) {
    //┬
    //○別スレッドとの衝突回避用のロック
    std::lock_guard<std::mutex> lock(QUEUE_MUTEX);
    //│
    //○キューの容量を確認
    if (QUEUE.empty()) return false;
    //│＼（通信デバイスが起動していない場合）
    //│ ▼返却：なし
    //│
    //○先頭を抽出
    //○先頭を削除
    //▼返却：あり
    argData = QUEUE.front();
    QUEUE.pop();
     return true;
  } /* popQueue() */

//========================================================
// Ｄ．データ受信
//========================================================
  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  static void ON_RECIVE(
    uint8_t   num    , // クライアント番号
    WStype_t  type   , // エベント種別
    uint8_t * payload, // 受信データ
    size_t    length   // 受信データ長
  ){
    //┬
    //○イベントの種類を確認
    if(type !=WStype_TEXT) return;
    //│＼（テキスト以外の場合）
    //│ ▼終了：早期リターン
    //│
    //○未取り込みデータを受信
    if (payload == nullptr || length < 1) return;
    //│＼（空の場合）
    //│ ▼終了：早期リターン
    //│
    //○受信データをキューに追加
    std::lock_guard<std::mutex> lock(QUEUE_MUTEX);
    QUEUE.push({num, String((char*)payload)});
    //┴
  } /* ON_RECIVE() */

//========================================================
// Ｅ．公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterWEB_Socket(MmpContext& argCtx) : AdapterBase(argCtx) {
    //┬
    //○サービスを開始
    ADP_SRV = new WebSocketsServer(SRV_PORT); // サーバ生成
    ADP_SRV->onEvent(ON_RECIVE)             ; // コールバック関数登録
    ADP_SRV->begin()                        ; // サーバ起動
    //│
    //○メッセージ表示
    Serial.println(String(" [OK] WebSocket -> port ") + String(SRV_PORT));
    //┴
  } /* constractor AdapterWEB_Socket() */

  //━━━━━━━━━━━━━━━━━
  // ポーリング用ハンドラ
  //━━━━━━━━━━━━━━━━━
  void handle() override {
    //┬
    //○WebSocketサーバの処理を進める
    // ・新規クライアントからの接続要求（ハンドシェイク）の受付
    // ・パケットの受送信とイベント（ON_RECIVE）の発火
    // ・Ping / Pong によるキープアライブ（接続維持チェック）
    // ・切断処理（クリーンアップ）
    ADP_SRV->loop();
    //│
    //◎┐ルーティングを指示
    myQueue popDat;
    while (popQueue(popDat)) {
      //│＼（キューが空の場合）
      //│ ▼BREAK：ルーティングを終了
      //│
      //○フレームの状態を確認
      if (popDat.FRAME.startsWith("#")){SEND_CONN(popDat.CONN); continue;}
      //│＼（エラーが発生している場合）
      //│ ●エラーをレスポンス
      //│ ▽次へ：次のキューを走査
      //│
      //●コマンドを実行
      adpBase::RUN(ADP_ID, popDat.FRAME);
      //│
      //●実行結果をレスポンス
      SEND_CONN(popDat.CONN);
      //┴
    } /* END-while */
    //┴
  } /* handle() */

}; /* class AdapterWEB_Socket */

// staticメンバの実体定義
WebSocketsServer*           AdapterWEB_Socket::ADP_SRV = nullptr;
int                         AdapterWEB_Socket::SRV_PORT = 8082;
std::queue<AdapterWEB_Socket::myQueue> AdapterWEB_Socket::QUEUE;
std::mutex                  AdapterWEB_Socket::QUEUE_MUTEX;