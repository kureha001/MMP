#include "bridge.hpp"

// ===================== ブリッジ 実装 =====================

/**
 * @brief グローバル：各 UART ポートのコンテキスト（2系統）
 */
PortCtx* ctxs[2] = {nullptr,nullptr};

/** @brief リング初期化 */
void Ring::init(size_t c){ b=(uint8_t*)malloc(c); cap=c; r=w=n=0; }

/** @brief リングへ 1byte を投入（満杯なら0） */
size_t Ring::push(uint8_t x){ if(n>=cap) return 0; b[w]=x; w=(w+1)%cap; n++; return 1; }

/** @brief リングから最大 len バイトを取り出す（返り値=実取り出しバイト数） */
size_t Ring::pop(uint8_t* p, size_t len){
  size_t m=0; while(m<len && n){ p[m++]=b[r]; r=(r+1)%cap; n--; } return m;
}

/** @brief コンストラクタ：サーバ開始に必要な配列を確保 */
PortCtx::PortCtx(uint16_t port,int maxClients):server(port){
  clients = new WiFiClient[maxClients];
}

// 内部ユーティリティ
static uint8_t tmp256[256];
static uint8_t tmp512[512];
static inline void releaseLock(PortCtx* c){ c->lockOwner=-1; c->lockDeadline=0; }
static inline bool lockExpired(PortCtx* c){ return (c->lockOwner>=0)&&(millis()>c->lockDeadline); }

/**
 * @brief 新規接続受入／切断掃除／ロック期限の解放
 * - hasClient() で新規要求を検知→空きスロットに収容（満員なら破棄）
 * - 切断済みスロットを stop() で掃除
 * - 書き込みロックの期限切れがあれば解放
 */
void acceptClients(PortCtx* c){
  // ★ 新規接続要求
  if (c->server.hasClient()){
    for(int i=0;i<SRV.maxClients;i++){
      if (!c->clients[i] || !c->clients[i].connected()){
        // 空きスロットに収容（古いソケットは明示的に stop）
        if (c->clients[i]) c->clients[i].stop();
        c->clients[i] = c->server.available();
        c->clients[i].setNoDelay(true);
        if(lockExpired(c)) releaseLock(c);  // ★ 期限切れロックを解放
        return; // 1接続ずつ処理
      }
    }
    // ★ 満員 → 新規要求は捨てる
    WiFiClient dump = c->server.available(); dump.stop();
  }

  // ★ 切断済みクライアントの掃除
  for(int i=0;i<SRV.maxClients;i++){
    if (c->clients[i] && !c->clients[i].connected()){
      if (c->lockOwner==i) releaseLock(c); // ★ ロック保有者が離脱→解放
      c->clients[i].stop();
    }
  }
}

/**
 * @brief 全クライアントからの受信を TCP→UART リングに取り込む
 * - writeLock 無効: 全クライアントの入力を受け付ける
 * - writeLock 有効: 先着オーナーのみ書き込み可（期限切れで交代）
 */
void pumpTCPtoRing(PortCtx* c){
  for(int i=0;i<SRV.maxClients;i++){
    WiFiClient& cli = c->clients[i];
    if (!cli || !cli.connected()) continue; // 未接続はスキップ
    while (cli.available()){
      int b = cli.read(); if (b<0) break;
      if (!SRV.writeLock){
        c->tcp2uart.push((uint8_t)b);
      } else {
        if (c->lockOwner<0 || lockExpired(c)){      // ★ オーナー不在/期限切れ
          c->lockOwner = i;
          c->lockDeadline = millis() + SRV.writeLockMs;
        }
        if (c->lockOwner==i) c->tcp2uart.push((uint8_t)b); // ★ オーナーだけ許可
      }
    }
  }
}

/**
 * @brief UART受信を UART→TCP リングに取り込む
 */
void pumpUARTtoRing(PortCtx* c, HardwareSerial* U){
  while (U->available()){
    int b = U->read(); if (b<0) break;
    c->uart2tcp.push((uint8_t)b);
  }
}

/**
 * @brief TCP→UART リング内容を UART に吐き出す
 */
void flushRingToUART(PortCtx* c, HardwareSerial* U){
  size_t n = c->tcp2uart.pop(tmp256, sizeof(tmp256));
  if (n) U->write(tmp256, n);
}

/**
 * @brief UART→TCP リング内容を 全TCPクライアント に配布する
 */
void flushRingToTCP(PortCtx* c){
  if (c->uart2tcp.empty()) return;
  size_t m = c->uart2tcp.pop(tmp512, sizeof(tmp512));
  for (int i=0;i<SRV.maxClients;i++){
    if (c->clients[i] && c->clients[i].connected()){
      c->clients[i].write(tmp512, m);
    }
  }
}
