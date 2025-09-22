#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "config.hpp"

// ===================== ブリッジ（UART<->TCP）ヘッダ =====================

/**
 * @brief 単純リングバッファ（ロックレス・単プロデューサ/コンシューマ想定）
 */
struct Ring {
  uint8_t* b=nullptr; size_t cap=0, r=0, w=0, n=0;
  void init(size_t c);                    ///< バッファ領域を確保して初期化
  size_t push(uint8_t x);                 ///< 1byte投入（満杯なら0を返す）
  size_t pop(uint8_t* p, size_t len);     ///< 最大lenを取り出す（返り値=実バイト数）
  bool empty() const { return n==0; }     ///< 空判定
};

/**
 * @brief ポート単位の文脈（TCPサーバ・クライアント群・リング・書き込みロック）
 */
struct PortCtx {
  WiFiServer server;
  WiFiClient* clients;
  Ring tcp2uart, uart2tcp;
  int lockOwner=-1; uint32_t lockDeadline=0;
  PortCtx(uint16_t port,int maxClients);  ///< サーバポートと最大クライアント数で初期化
};

extern PortCtx* ctxs[2];

/**
 * @brief 新規クライアント受入／切断掃除／ロック期限解放を行う
 */
void acceptClients(PortCtx* c);

/**
 * @brief TCP→UART リング取り込み（全クライアントから集約）
 */
void pumpTCPtoRing(PortCtx* c);

/**
 * @brief UART→TCP リング取り込み（UART受信を集約）
 */
void pumpUARTtoRing(PortCtx* c, HardwareSerial* U);

/**
 * @brief リング内容を UART に吐き出す
 */
void flushRingToUART(PortCtx* c, HardwareSerial* U);

/**
 * @brief リング内容を 全TCPクライアント へブロードキャスト
 */
void flushRingToTCP(PortCtx* c);
