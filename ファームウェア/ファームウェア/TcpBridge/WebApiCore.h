// ============================================================
// WebApiCore.h  (compat version for TcpBridge.ino)
// 既存の呼び出し形に合わせたネームスペース関数:
//   - WebApiCore::begin(WebServer&)
//   - WebApiCore::handle(WebServer&)  ← (未使用でもOK)
// ルーティング定義は begin() 時に server へ直接登録します。
// ============================================================

#pragma once
#include <Arduino.h>
#include <WebServer.h>

namespace WebApiCore {

// 既存スケッチの呼び出しと互換
void begin(WebServer& server);

// 任意: 必要ならこちらを呼んでも良い（通常は httpApi.handleClient() を直接呼ぶ）
void handle(WebServer& server);

} // namespace WebApiCore
