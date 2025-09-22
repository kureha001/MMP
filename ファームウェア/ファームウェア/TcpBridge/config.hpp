#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>

// ===================== 設定関連 ヘッダ =====================

/**
 * @brief Wi-Fi の接続候補
 * - ssid: アクセスポイント名
 * - pass: パスワード（空ならオープン）
 * - timeout_ms: この候補を諦めるまでの待機時間
 */
struct WifiCand { String ssid, pass; uint32_t timeout_ms; };

/**
 * @brief AP フォールバックの設定
 * - enabled: 失敗時に AP モードへ落とすか
 * - ssid/pass: AP モード時の SSID/パス
 * - hold_seconds: 将来拡張用（APを維持する秒数）
 */
struct WifiAP   { bool enabled; String ssid, pass; uint32_t hold_seconds; };

/**
 * @brief Wi-Fi 全体設定
 * - hostname: デバイスのホスト名
 * - cand: 接続候補（最大6件）
 * - candN: 実際の候補数
 * - apfb: AP フォールバック設定
 */
struct WifiCfg {
  String hostname;
  WifiCand cand[6];
  int candN = 0;
  WifiAP apfb { true, "m5-bridge-setup", "", 0 };
};

/**
 * @brief UART と TCPブリッジの設定
 * - port: どの HardwareSerial を使うか
 * - rx/tx: GPIO ピン番号
 * - baud: ボーレート
 * - tcpPort: リッスンする TCP ポート番号
 */
struct UartCfg { HardwareSerial* port; uint8_t rx, tx; uint32_t baud; uint16_t tcpPort; };

/**
 * @brief ブリッジサーバ全体の設定
 * - maxClients: 同時接続数
 * - writeLock: 同時書き込み排他を有効にするか
 * - writeLockMs: 排他継続時間（ミリ秒）
 */
struct ServerCfg { int maxClients; bool writeLock; uint32_t writeLockMs; };

// ---- グローバル（config.cppで定義） ----
extern WifiCfg  WIFI;
extern UartCfg  UARTS[2];
extern int      NUM_UARTS;
extern ServerCfg SRV;

// ---- API ----

/**
 * @brief /config.json を読み込む（無ければ最小構成で自動生成）
 * @return true: 成功 / false: 失敗
 *
 * 動作:
 * - LittleFS を開始
 * - 初回は hostname と UART/Server 既定値で /config.json を作成
 * - 読み込んだ JSON を構造体へ反映（wifi.candidates, uart[], server）
 */
bool loadConfig();

/**
 * @brief ホスト名と UART 設定を適用する
 * - WiFi.setHostname()
 * - Serial1/2 の begin()（GPIO, ボーレート）
 */
void applyUartConfig();
