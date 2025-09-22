#pragma once
#include <Arduino.h>

// ===================== Web UI ヘッダ =====================

/**
 * @brief Web サーバを開始し、ルーティング（/, /config, /upload, /status）を登録する
 */
void webui_begin();

/**
 * @brief ループ内で呼び出す HTTP 処理
 */
void webui_handle();

/**
 * @brief /status とシリアルで共用する起動情報テキストを生成する
 */
String buildStartupStatusText();
