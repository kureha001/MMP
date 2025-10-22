// srvHttp.h
// サーバー：WEB API（サーバインスタンスは内部で保持）

#pragma once
#include <Arduino.h>

namespace srvHttp {
  // 新API（推奨）
  bool start(uint16_t port = 8080);  // ルーティング登録＋listen開始
  void handle();                     // 1回呼び出しで1ステップ処理
}
