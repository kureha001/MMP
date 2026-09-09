// filename : Dep_Connect/adapter/base/WEB_Socket.cpp
//========================================================
// 接続部門／業務課／担当(標準型)：WEB Socket 担当
//--------------------------------------------------------
// Ver 1.2.3 (2026/09/06) 
//========================================================
//┬
//□┐インクルード
  //□追加ライブラリ：WebSockets by Markus Sattler
  #include <WebSocketsServer.h>
  #include <WebSocketsClient.h>
//┴┴

//========================================================
// 組織図
//========================================================
//┬
//□┐接続部門
  //□┐業務課
    //□担当(標準型)
    #include "_index_.h"
//┴┴┴

//########################################################
//# 処理詳細
//########################################################
class AdapterWEB_Socket : public AdapterQueueBase<uint8_t> {
public:
  //━━━━━━━━━━━━━━━━━
  // 抽象基底クラスからコンテクストを継承
  //━━━━━━━━━━━━━━━━━
  using AdapterQueueBase::AdapterQueueBase;

private:
//========================================================
// アダプタの基本
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 一般情報
  //━━━━━━━━━━━━━━━━━
    const int ADP_ID = ADP_ID_WSOC;
    int getAID() const override {return ADP_ID;} // 基底クラスに連携

  //━━━━━━━━━━━━━━━━━
  // サービス関連情報
  //━━━━━━━━━━━━━━━━━
#if (MODE == MODE_BRIDGE)
    WebSocketsClient  MY_NET           ; // WebSocketクライアント
#else
    WebSocketsServer* MY_NET  = nullptr; // WebSocketサーバ
    int               MY_PORT = 8082   ; // ポート番号
#endif
    static AdapterWEB_Socket* MY_INSTANS; // 静的コールバックからのルーティング用

//========================================================
// レスポンス
//========================================================
  //─────────────────
  // クライアントにレスポンス
  // ※AdapterQueueBaseをオーバーライド
  //─────────────────
  void SEND_CONN(uint8_t argConn) override {
    //┬
    //○メッセージをレスポンス
#if (MODE == MODE_BRIDGE)
    MY_NET.sendTXT(ctx.resMSG.c_str());
#else
    if (MY_NET) MY_NET->sendTXT(argConn, ctx.resMSG.c_str());
#endif
    //│
    //●ログ出力
    adpFnBase::SHOW_LOG();
    //┴
  } /* SEND_CONN() */

//========================================================
// データ受信
//========================================================
  //━━━━━━━━━━━━━━━━━
  // コールバック：クライアント用
  //━━━━━━━━━━━━━━━━━
  static void ON_RECIVE(
#if (MODE != MODE_BRIDGE)
    uint8_t   num    , // クライアント番号
#endif
    WStype_t  type   , // イベント種別
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
    if (type != WStype_TEXT) return;
    //│＼（テキスト以外の場合）
    //│ ▼終了：早期リターン
    //│
    //○未取り込みデータを受信
    if (payload == nullptr || length < 1) return;
    //│＼（空の場合）
    //│ ▼終了：早期リターン
    //│
    //○受信データをキューに追加（基底クラスの pushQueue を呼出し）
#if (MODE == MODE_BRIDGE)
    uint8_t num = 0;
#endif
    MY_INSTANS->pushQueue(num, String((char*)payload));
    //┴
  } /* ON_RECIVE() */

//========================================================
// 担務（公開機能）
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterWEB_Socket(MmpContext& argCtx) : AdapterQueueBase(argCtx) {
#if (MODE == MODE_BRIDGE)
    //┬
    //○メッセージ表示
    Serial.println(" [OK] WebSocket Client");
    //┴
#else
    //┬
    //○インスタンスを取得
    MY_INSTANS = this;
    //│
    //○サーバのサービスを開始
    MY_NET = new WebSocketsServer(MY_PORT); // サーバ生成
    MY_NET->onEvent(ON_RECIVE)             ; // コールバック関数登録
    MY_NET->begin()                        ; // サーバ起動
    //│
    //○メッセージ表示
    Serial.printf(" [OK] WebSocket Server -> port %d\n"), MY_PORT);
    //┴
#endif
  } /* constractor AdapterWEB_Socket() */

  //━━━━━━━━━━━━━━━━━
  // 転送受付
  //━━━━━━━━━━━━━━━━━
#if (MODE == MODE_BRIDGE)
  void trans() {
    //┬
    //○クライアントを起動
    if (!MY_NET.isConnected()) {
      String   ip   = ctx.transDat1st;
      uint16_t port = (uint16_t)ctx.transDat2nd.toInt();
      MY_INSTANS = this;
      MY_NET.onEvent(ON_RECIVE); 
      MY_NET.begin(ip.c_str(), port, "/");
      //│
      //○接続するまでまつ。
      unsigned long startTime = millis();
      while(!MY_NET.isConnected() && millis() - startTime < 10000){
        MY_NET.loop();
        delay(200);
      }
      //│
      //○タイムアウトはエラーコードをレスポンス
      if (!MY_NET.isConnected()) {ctx.resMSG = "#TIM!"; return;}
    }
    //│
    //○リクエストを転送
    MY_NET.sendTXT(ctx.strFrame);
    //┴
  };
#endif

  //━━━━━━━━━━━━━━━━━
  // ポーリング用前処理
  //━━━━━━━━━━━━━━━━━
  void handle_begin() override {
    //┬
    //○WebSocketの処理を進める（イベント発火）
#if (MODE == MODE_BRIDGE)
    MY_NET.loop();
#else
    if (MY_NET) MY_NET->loop();
#endif
    //┴
  } /* handle_begin() */
}; /* class AdapterWEB_Socket */

//━━━━━━━━━━━━━━━━━
// インスタンス管理用
//━━━━━━━━━━━━━━━━━
AdapterWEB_Socket* AdapterWEB_Socket::MY_INSTANS = nullptr;