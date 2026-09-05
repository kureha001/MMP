// filename : connection/member/WEB_Socket.cpp
//========================================================
// 経路アダプタ：WEB Socket
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■同僚
  #include "_index_.h"
  //│
  //■追加ライブラリ：WebSockets by Markus Sattler
  #include <WebSocketsServer.h>
  //┴
//┴

//########################################################
//# クラス：経路アダプタ(WEB Socket)
//########################################################
class AdapterWEB_Socket : public AdapterBase {
//========================================================
// Ａ．アダプタの基本
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // インスタンス管理用
    //（静的コールバックからのルーティング用）
    //─────────────────
    static AdapterWEB_Socket* MY_INSTANS;

    //─────────────────
    // ステータス
    //─────────────────
    const int ADP_ID = ADP_ID_WSOC;

    //─────────────────
    // 使用するサービス
    //─────────────────
    WebSocketsServer* ADP_SRV  = nullptr; // WebSocketサーバ
    int               SRV_PORT = 8082   ; // ポート番号

//========================================================
// Ｂ．レスポンス
//========================================================
  //─────────────────
  // クライアントにレスポンス
  //----------------------------------
  // 引数：
  // ・出力制限：強制出力(true)、通常出力(false)
  // ・接続資源：キューから取得した物
  //─────────────────
  void SEND_CONN(bool argMode, uint8_t argConn){
    //┬
    //○動作モードを確認
    if (!argMode && ctx.sysMode != MODE_MAIN) return;
    //│＼（出力制限がなく、メインモード以外の場合）
    //│ ▼終了：早期リターン
    //│
    //○メッセージをレスポンス
    if (ADP_SRV) ADP_SRV->sendTXT(argConn, ctx.resMSG.c_str());
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
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
  std::queue<myQueue> QUEUE      ; // キューバッファ
  std::mutex          QUEUE_MUTEX; // 別スレッドとの衝突回避用のロック
 
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
    //○インスタンスを確認
    if (!MY_INSTANS) return;
    //│＼（通信デバイスが起動していない場合）
    //│ ▼終了：早期リターン
    //│
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
    std::lock_guard<std::mutex> lock(MY_INSTANS->QUEUE_MUTEX);
    MY_INSTANS->QUEUE.push({num, String((char*)payload)});
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
    //○インスタンスを登録
    MY_INSTANS = this;
    //│
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
      if (popDat.FRAME.startsWith("#")){SEND_CONN(true, popDat.CONN); continue;}
      //│＼（エラーが発生している場合）
      //│ ●エラーを強制レスポンス
      //│ ▽次へ：次のキューを走査
      //│
      //●コマンドを実行
      mode::RUN(ADP_ID, popDat.FRAME);
      //│
      //●実行結果をレスポンス
      SEND_CONN(false, popDat.CONN);
      //┴
    } /* END-while */
    //┴
  } /* handle() */

}; /* class AdapterWEB_Socket */

//━━━━━━━━━━━━━━━━━
//インスタンス管理用
//━━━━━━━━━━━━━━━━━
AdapterWEB_Socket* AdapterWEB_Socket::MY_INSTANS = nullptr;