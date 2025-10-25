// filename : cliNetTcp.h
//========================================================
// クライアント：ネット(TCP)
//--------------------------------------------------------
// Ver 0.6.0 (2025/xx/xx)
//========================================================
#pragma once
#include <Arduino.h>

// 統一入口(fnPerser.h)
extern String MMP_REQUEST(const String& wire, int clientID);


//========================================================
// 受信バッファリング → コマンド実行 → レスポンス送信
//========================================================
namespace {

  constexpr int g_ID_TCP  = 2       ; // 経路番号: TCP
  WiFiServer    g_server(0)         ; // サーバ
  bool          g_STARTED = false   ; // 起動有無 

  struct Slot {
    WiFiClient cli                  ; // Wifi接続
    String     rx                   ; // 受信バッファ     ※ !まで貯める）
    uint32_t   lastActive = 0       ; // 最終更新時刻(ms) ※ タイムアウトで使用
    bool       used       = false   ; // 使用状況
  };

  Slot*   g_slots     = nullptr     ; // 接続スロット
  int     g_MAX_SLOTS = 0           ; // 最大接続数 
  const   uint32_t kIdleMs   = 30000; // 無通信切断
  const   size_t   kMaxFrame = 256  ; // 1フレーム上限

//━━━━━━━━━━━━━━━━━
// ヘルパー：接続スロット制御
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 指定の接続スロットをリセット
  //─────────────────
  void dropSlot(int idx){
    if ( idx < 0 || idx >= g_MAX_SLOTS ) return;
    if ( g_slots[idx].cli ) g_slots[idx].cli.stop();
    g_slots[idx].rx         = ""    ; //
    g_slots[idx].used       = false ; //
    g_slots[idx].lastActive = 0     ; //
  } // dropSlot()
  //─────────────────
  // 空き接続スロットのIDを照会
  //─────────────────
  int getFeeSlotID() {
    for (int idx = 0 ; idx < g_MAX_SLOTS ; idx++)
      if ( !g_slots[idx].used || !g_slots[idx].cli.connected() ) return idx;
    return -1;
  } // getFeeSlotID()
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
      int idx = getFeeSlotID();
      if (idx < 0) { c.stop(); continue; }

      // 接続スロットの情報を更新
      g_slots[idx].cli        = c       ; // move
      g_slots[idx].cli.setNoDelay(true) ;
      g_slots[idx].rx.reserve(64)       ;
      g_slots[idx].rx         = ""      ;
      g_slots[idx].used       = true    ;
      g_slots[idx].lastActive = millis();
    } // while
  } // updateSlot()

//━━━━━━━━━━━━━━━━━
// ヘルパー：ＭＭＰコマンド編集
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ‘!’までを切り出し（無ければ空）
  //─────────────────
  String takeFrame(String& buf){
    int p = buf.indexOf('!');
    if (p < 0) return String();
    String s = buf.substring(0, p+1); // ‘!’含む
    buf.remove(0, p+1);
    return s;
  }

//━━━━━━━━━━━━━━━━━
// ルート別処理
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ＭＭＰコマンド
  //─────────────────
  void routeMMP(int i){

    // 0) 前処理
    auto& s = g_slots[i];
    auto& c = s.cli;
    if (!c.connected()) { dropSlot(i); return; }

    // 1) 受信バッファリング
    while (c.available()) {

      char ch = (char)c.read();
      if (s.rx.length() < (int)kMaxFrame) {
        s.rx += ch;
      } else {
        // 長すぎ → エラー返信してリセット
        c.print("#CHK!");
        s.rx = "";
      } // if

      s.lastActive = millis();

    } // while

    // フレーム処理（! 区切りで複数可）
    while (true) {

      String wire = takeFrame(s.rx);

      if (wire.isEmpty()) break;

      // 最低限の sanity
      if (wire.length() < 2) {
        c.print("#CMD!");
        s.lastActive = millis();
        continue;
      } // if

    // 2) コマンドパーサーへリクエスト
      String five = MMP_REQUEST(wire, g_ID_TCP);
      if (five.length()==0) five = "!!!!!";  // 空なら成功扱いでも可

      // 3) レスポンスを編集
      // (該当処理なし)

      // 4) 通信経路にレスポンスする
      c.print(five);
      s.lastActive = millis();
    }

    // アイドル切断
    if (millis() - s.lastActive > kIdleMs) dropSlot(i);
  } // while

//━━━━━━━━━━━━━━━━━
// ルーティング登録
//━━━━━━━━━━━━━━━━━
// (該当処理なし)

} // namespace (No Name)


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
      g_slots     = new Slot[g_SRV.maxClients]; // 最大接続数で確保
      g_MAX_SLOTS  = g_SRV.maxClients         ; // 最大接続数

    // 2) サーバを起動
    g_server      = WiFiServer(port)          ; // サーバ登録
    g_server.begin()                          ; // サーバ起動
    g_server.setNoDelay(true)                 ;
    g_STARTED     = true                      ;

    // 3) 正常終了
    return true;
  } // start()

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
    for (int idx = 0; idx < g_MAX_SLOTS ; idx++)
      if (g_slots[idx].used) routeMMP(idx);
  } // handle()

} // namespace srvTcp
