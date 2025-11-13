// filename : cliNetTcp.h
//========================================================
// クライアント：ネット(TCP)
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/11) 初版
//========================================================
#pragma once
#include "cli.h"  // クライアント：共通ユーティリティ

//━━━━━━━━━━━━━━━━━
// グローバル資源(宣言)
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 統一入口：fnPerser.hで定義
  //─────────────────
  extern String MMP_REQUEST(const String& path, int clientID);

//─────────────────
// サーバ情報
//─────────────────
struct typeServer {
  int      maxClients   = 0     ; // 最大接続数
  bool     writeLock    = false ; // 書込ロック有無
  uint32_t writeLockMs  = 0     ; // 書込ロック時間[単位:ms]
};
typeServer  g_SRV_TCP;


//========================================================
// メイン処理
//========================================================
namespace {
//━━━━━━━━━━━━━━━━━
// 事前データ
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // サーバー
  //─────────────────
  WiFiServer      g_server(0)       ; // サーバ
  bool            g_STARTED = false ; // 起動有無 
  const uint32_t  kIdleMs   = 30000 ; // 無通信切断
  const size_t    kMaxFrame = 256   ; // 1フレーム上限

  //─────────────────
  // スロット管理
  //─────────────────
  struct Slot {
    WiFiClient  cli               ; // Wifi接続
    String      rx                ; // 受信バッファ     ※ !まで貯める
    uint32_t    lastActive = 0    ; // 最終更新時刻(ms) ※ タイムアウトで使用
    bool        used       = false; // 使用状況
  };
  Slot* g_SLOT = nullptr; // 接続スロット

//━━━━━━━━━━━━━━━━━
// ヘルパー：接続スロット制御
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 指定の接続スロットをリセット
  //─────────────────
  void dropSlot(int idx){
    if ( idx < 0 || idx >= g_SRV_TCP.maxClients ) return;
    if ( g_SLOT[idx].cli ) g_SLOT[idx].cli.stop();
    g_SLOT[idx].rx         = ""    ; //
    g_SLOT[idx].used       = false ; //
    g_SLOT[idx].lastActive = 0     ; //
  } /* dropSlot() */
  //─────────────────
  // 空き接続スロットのIDを照会
  //─────────────────
  int getFreeSlotID() {
    for (int idx = 0 ; idx < g_SRV_TCP.maxClients ; idx++)
      if ( !g_SLOT[idx].used || !g_SLOT[idx].cli.connected() ) return idx;
    return -1;
  } /* getFreeSlotID() */
  //─────────────────
  // 接続スロットの情報を更新
  //─────────────────
  void updateSlot(){
    // 接続スロットが空いたら情報を更新
    while (true) {
      // Wifiの有効性を取得：→ 無効であれば中断
      WiFiClient c = g_server.available();
      if (!c) break;

      // 空き接続スロットのIDを取得：→ 無効であればスキップ
      int idx = getFreeSlotID();
      if (idx < 0) { c.stop(); continue; }

      // 接続スロットの情報を更新
      g_SLOT[idx].cli        = c       ; // move
      g_SLOT[idx].cli.setNoDelay(true) ;
      g_SLOT[idx].rx.reserve(64)       ;
      g_SLOT[idx].rx         = ""      ;
      g_SLOT[idx].used       = true    ;
      g_SLOT[idx].lastActive = millis();
    } // while
  } /* updateSlot() */

//━━━━━━━━━━━━━━━━━
// ヘルパー：ＭＭＰコマンド編集
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ‘!’までを切り出し
  //─────────────────
  String takeFrame(String& buf){
    int p = buf.indexOf('!')        ; // !の位置を取得
    if (p < 0) return String()      ; // 無ければ空
    String s = buf.substring(0, p+1); // !含む
    buf.remove(0, p+1)              ; // バッファから該当分を削除
    return s                        ; // フレームを返す
  } /* takeFrame() */

//━━━━━━━━━━━━━━━━━
// ルート別処理
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ＭＭＰコマンド実行
  //─────────────────
  void routeMMP(int argSlotIDX){
    //┬
    //○前処理
    auto& s = g_SLOT[ argSlotIDX ];
    auto& c = s.cli;
    if (!c.connected()){dropSlot(argSlotIDX); return; }
    // ＼（通信断の場合）
      //↓
      //○スロットをリセット
      //▼RETURN
    //│
    //◎┐受信バッファリング
    while (c.available()) {
      //○バイト単位で取得
      char ch = (char)c.read();
      if (s.rx.length() < (int)kMaxFrame) {s.rx += ch;}
      else {
      // ＼（長すぎ）
        //↓
        //○エラーをセット
        c.print("#CHK!");
        s.rx = "";
        //┴
      } /* if */
      //│
      //○
      s.lastActive = millis();
      //┴
    } /* while */
    //│
    //◎┐フレームを切り出す
    while (true) {
      //○コマンドパスを取得
      String path = takeFrame(s.rx);
      if (path.isEmpty()) break;
      // ＼（空の場合）
        //↓
        //▼１：ループ脱出
    //│
    //○┐コマンド実行
      //○IPアドレスの配列IDを取得
      int ipKey = getIP(g_IP_TCP, c.remoteIP());
      if (ipKey < 0) {
      // ＼（スロットを使い切った）
        //↓
        //○エラーをセット
        //▼０:スキップ
        c.print("#MEM!");
        s.lastActive = millis();
        continue;
      }
      //│
      //○コマンドパーサーへリクエスト
      String resp = MMP_REQUEST(path, ROUTE_ID_TCP);  // コマンド実行
      //│
      //○通信経路にレスポンス
      c.print(resp);
      s.lastActive = millis();
      //┴
    } /* while */
    //│
    //○アイドル切断
    if (millis() - s.lastActive > kIdleMs) dropSlot(argSlotIDX);
    //┴
  } /* routeMMP() */

//━━━━━━━━━━━━━━━━━
// ルーティング登録
//━━━━━━━━━━━━━━━━━
// (該当処理なし)

} /* namespace(匿名) */


//========================================================
// ハンドラ関連処理
//========================================================
namespace srvTcp {
  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  // - cliNet.hから実行
  //━━━━━━━━━━━━━━━━━
  bool start(uint16_t port){

    // 1) 前処理
      // 二重起動チェック： → 起動済みの場合は中断
      if (g_STARTED) return true;

      // 接続スロットの情報を確保
      g_SLOT  = new Slot[g_SRV_TCP.maxClients]; // 最大接続数で確保

    // 2) サーバを起動
    g_server  = WiFiServer(port)  ; // サーバ登録
    g_server.begin()              ; // サーバ起動
    g_server.setNoDelay(true)     ;
    g_STARTED = true              ; // 起動有無

    // 3) 正常終了
    return true;
  } /* start() */

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口（ポーリング入口）
  // - スケッチのloop()から実行
  // - 実処理はrouteMMP()
  //━━━━━━━━━━━━━━━━━
  void handle(){

    // 1) 起動チェック
    if (!g_STARTED) return;

    // 2) 接続スロットの情報を更新
    updateSlot();

    // 3) すべての接続スロットを処理
    for (int idx = 0; idx < g_SRV_TCP.maxClients ; idx++)
      if (g_SLOT[idx].used) routeMMP(idx);
  } /* handle() */

} /* namespace srvTcp */
