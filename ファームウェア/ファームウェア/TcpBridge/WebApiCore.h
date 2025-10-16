// WebApiCore.h
#pragma once
#include <Arduino.h>
#include <WebServer.h>

namespace WebApiCore {

    // 初期化（ルーティング登録）
    void begin(WebServer& server);

    // 任意：必要であれば呼び出し
    void handle(WebServer& server);

} // namespace WebApiCore
