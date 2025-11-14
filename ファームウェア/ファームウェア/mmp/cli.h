// filename : cli.h
//========================================================
// クライアント：共通
//  - 経路IDの提供
//  - クライアント管理機能の提供
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/14) α版
//========================================================
#pragma once
#include <string.h>

//━━━━━━━━━━━━━━━━━
// 経路ID
//━━━━━━━━━━━━━━━━━
static constexpr int ROUTE_ID_USB   = 0; // cliSerial.h
static constexpr int ROUTE_ID_UART0 = 1; // cliSerial.h
static constexpr int ROUTE_ID_TCP   = 2; // cliNetTcp.h
static constexpr int ROUTE_ID_HTTP  = 3; // cliNetHttp.h

//━━━━━━━━━━━━━━━━━
// クライアント管理
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 指標値
  //─────────────────
  constexpr int MAX_IP      = 10            ; // ネット経路の最大IP数
  constexpr int MAX_CLIENTS = MAX_IP * 2 + 2; // クライアントの最大数

  //─────────────────
  // IPアドレス管理
  //─────────────────
  struct typeIP {
    uint32_t  ipKey = 0     ; // getIP_key()で値を生成
    bool      inUse = false ; // 有効性判定
  }; /* typeIP */
  static typeIP g_IP_TCP[MAX_IP]  ; // TCPブリッジ用
  static typeIP g_IP_HTTP[MAX_IP] ; // Web-API用

  //─────────────────
  // IPアドレスの配列IDを取得
  // 戻り値：
  //　既存：既にある添え字
  //　新規：新たな添え字
  //　限界：-1
  //─────────────────
  static uint32_t getIP_key(const IPAddress& ip){
    return  ((uint32_t)ip[0]<<24) |
            ((uint32_t)ip[1]<<16) |
            ((uint32_t)ip[2]<< 8) |
            ((uint32_t)ip[3]    ) ;
  } /* getIP_key() */
  //─────────────────
  static int getIP(typeIP* table, const IPAddress& ip){

    // IPアドレスからキーを生成
    uint32_t key = getIP_key(ip);

    // 既存スロットを走査：あれば該当する添え字をリターン
    for (int i=0;i<MAX_IP;i++){
      if (table[i].inUse && table[i].ipKey == key) return i;
    } /* for */

    // 空きスロットを走査：あれば添え字をリターン
    for (int i=0;i<MAX_IP;i++){
      if (!table[i].inUse){
        table[i].inUse = true;
        table[i].ipKey = key;
        return i;
      } /* if */
    } /* for */

    // スロット不足：エラーコードをリターン
    return -1;
  } /* getIP_table() */