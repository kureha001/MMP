// filename : connection/member/WEB_Socket.cpp
//========================================================
// 経路アダプタ：WEB Socket
//--------------------------------------------------------
// Ver 1.2.3 (2026/09/06) 
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

  //━━━━━━━━━━━━━━━━━
  // ID取得 (基底クラスの dispatch 処理用)
  //━━━━━━━━━━━━━━━━━
  int getAdpId() const override { return ADP_ID; }

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
    if (ADP_SRV) ADP_SRV->sendTXT(argConn, ctx.resMSG.c_str());
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
    uint8_t   num    , // クライアント番号
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
    MY_INSTANS->pushQueue(num, String((char*)payload));
    //┴
  } /* ON_RECIVE() */

//========================================================
// 公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  AdapterWEB_Socket(MmpContext& argCtx) : AdapterQueueBase(argCtx) {
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
  // ポーリング用前処理
  //━━━━━━━━━━━━━━━━━
  void handle_begin() override {
    //┬
    //○WebSocketサーバの処理を進める（イベント発火）
    if (ADP_SRV) ADP_SRV->loop();
    //┴
  } /* handle_begin() */
}; /* class AdapterWEB_Socket */

//━━━━━━━━━━━━━━━━━
// インスタンス管理用
//━━━━━━━━━━━━━━━━━
AdapterWEB_Socket* AdapterWEB_Socket::MY_INSTANS = nullptr;