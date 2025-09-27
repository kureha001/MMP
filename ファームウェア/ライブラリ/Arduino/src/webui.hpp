// webui.hpp
//=======================================
// Web UI ヘッダ
//=======================================
#pragma once
#include <Arduino.h>

// Web サーバを開始し、ルーティング
// （/, /config, /upload, /status）を登録する
void webui_begin();

// ループ内で呼び出す HTTP 処理
void webui_handle();

// 起動情報テキストを生成する
String buildStartupStatusText();