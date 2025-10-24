// filename : cliNetTcp.h
//========================================================
// クライアント：ネット(TCP)
//--------------------------------------------------------
// Ver 0.6.0 (2025/xx/xx)
//========================================================
#pragma once
#include <Arduino.h>

// 本体が実装：wire("CMD[:a:b:c]!") → "!!!!!"/"dddd!"/"#xxx!"
// 統一入口（経路番号方式）
extern String MMP_REQUEST(const String& wire, int clientID);

namespace {

  constexpr int kClientIdTcp   = 2;      // 経路番号: TCP
  WiFiServer    g_server(0);
  bool          g_started = false;

  struct Slot {
    WiFiClient cli;                 //
    String     rx;                  // 受信バッファ（! まで貯める）
    uint32_t   lastActive = 0;      //
    bool       used       = false;  //
  };

  Slot*   g_slots     = nullptr;
  int     g_maxSlots  = 0;
  const   uint32_t kIdleMs   = 30000;    // 無通信切断
  const   size_t   kMaxFrame = 256;      // 1フレーム上限

  int findFree() {
    for (int i=0;i<g_maxSlots;i++)
      if (!g_slots[i].used || !g_slots[i].cli.connected()) return i;
    return -1;
  }
  void drop(int i){
    if (i<0 || i>=g_maxSlots) return;
    if (g_slots[i].cli) g_slots[i].cli.stop();
    g_slots[i].rx = "";
    g_slots[i].used = false;
    g_slots[i].lastActive = 0;
  }

  void acceptLoop(){
    while (true) {
      WiFiClient c = g_server.available();
      if (!c) break;
      int idx = findFree();
      if (idx < 0) { c.stop(); continue; }
      g_slots[idx].cli = c;               // move
      g_slots[idx].cli.setNoDelay(true);
      g_slots[idx].rx.reserve(64);
      g_slots[idx].rx = "";
      g_slots[idx].used = true;
      g_slots[idx].lastActive = millis();
    }
  }

  // ‘!’までを切り出し（無ければ空）
  String takeFrame(String& buf){
    int p = buf.indexOf('!');
    if (p < 0) return String();
    String s = buf.substring(0, p+1); // ‘!’含む
    buf.remove(0, p+1);
    return s;
  }

  void handleOne(int i){
    auto& s = g_slots[i];
    auto& c = s.cli;
    if (!c.connected()) { drop(i); return; }

    // 受信
    while (c.available()) {
      char ch = (char)c.read();
      if (s.rx.length() < (int)kMaxFrame) s.rx += ch;
      else {
        // 長すぎ → エラー返信してリセット
        c.print("#CHK!");
        s.rx = "";
      }
      s.lastActive = millis();
    }

    // フレーム処理（! 区切りで複数可）
    while (true) {
      String wire = takeFrame(s.rx);
      if (wire.isEmpty()) break;

      // 最低限の sanity
      if (wire.length() < 2) { c.print("#CMD!"); s.lastActive=millis(); continue; }

      // 本体へ同期呼び出し（TCP=2）
      String five = MMP_REQUEST(wire, kClientIdTcp);
      if (five.length()==0) five = "!!!!!";  // 空なら成功扱いでも可

      // 同じクライアントにそのまま返す
      c.print(five);
      s.lastActive = millis();
    }

    // アイドル切断
    if (millis() - s.lastActive > kIdleMs) drop(i);
  }
} // namespace
//========================================================


//========================================================
namespace srvTcp {

  bool begin(uint16_t port){

    if (g_started) return true;

    g_slots = new Slot[g_SRV.maxClients];
    g_maxSlots = g_SRV.maxClients;

    g_server = WiFiServer(port);
    g_server.begin();
    g_server.setNoDelay(true);
    g_started = true;

    return true;
  }

  void handle(){
    if (!g_started) return;
    acceptLoop();
    for (int i=0;i<g_maxSlots;i++)
      if (g_slots[i].used) handleOne(i);
  }

  int clientCount(){
    int n=0;
    for (int i=0;i<g_maxSlots;i++)
      if (g_slots[i].used && g_slots[i].cli.connected()) n++;
    return n;
  }

  void closeAll(){
    if (!g_started) return;
    for (int i=0;i<g_maxSlots;i++) drop(i);
  }

} // namespace srvTcp
//========================================================
