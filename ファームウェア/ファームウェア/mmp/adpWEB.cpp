// filename : adpWEB.cpp
//========================================================
// ＷＥＢアダプタ：管理用画面
//--------------------------------------------------------
//【目的】
// ・現在のWiFi設定ファイルを表示する
// ・WiFi設定ファイルをドラック＆ドロップでアップロードする
//   （アップロード後は再起動する）
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/23) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <WiFi.h     > //
  #include <WebServer.h> // ユーザ受付資源
  #include <LittleFS.h > // 設定ファイル
  //│
  //■ＭＭＰシステム
  //┴
//┴

//########################################################
//# 専用の名前空間
//########################################################
namespace adpWEB {
//========================================================
// Ａ．基本情報
//========================================================
  //─────────────────
  // 公開情報
  //─────────────────
  bool ENABLED  = false          ; // 有効性：{有効：true|無効：false}

  //─────────────────
  // アクセス資源を提供するサービス
  //─────────────────
  static WebServer* ADP_SRV  = nullptr; // WEBサーバ
  int               SRV_PORT = 8082   ; // ポート番号

//========================================================
// Ｂ．ルーティング処理（プロセス）
//--------------------------------------------------------
// HANDLE()で明示的に呼び出す
//========================================================
  //─────────────────
  // １．管理画面
  //─────────────────
  static void routeRoot() {
    //┬
    //○HTMLバッファを開始
    String html = "<!DOCTYPE html>";
    //│
    //○HTMLバッファに「ヘッダ～ボディ」を追加
    html += "<html lang=\"ja\"><head><meta charset=\"UTF-8\"><title>MMP Config Uploader</title>";
    html += "<style>";
    html += "body { font-family: sans-serif; max-width: 800px; margin: 30px auto; padding: 0 20px; background: #f4f7f6; color: #333; }";
    html += "h2 { border-bottom: 2px solid #ccc; padding-bottom: 5px; }";
    html += "textarea { width: 100%; height: 250px; font-family: monospace; padding: 10px; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; background: #fff; resize: vertical; }";
    html += ".drop-zone { border: 2px dashed #4CAF50; border-radius: 6px; padding: 30px; text-align: center; background: #e8f5e9; cursor: pointer; margin-top: 20px; transition: background 0.2s; }";
    html += ".drop-zone.dragover { background: #c8e6c9; }";
    html += ".btn { background: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; margin-top: 10px; }";
    html += ".btn:hover { background: #45a049; }";
    html += "#status { margin-top: 15px; font-weight: bold; }";
    html += "</style></head><body>";
    //│
    //○HTMLバッファに「本文」を追加
    html += "<h2>現在の設定 (/config.json)</h2>";
    html += "<textarea id=\"configViewer\" readonly>";
    //│
    //◇┐LittleFSから現在のconfig.jsonを読み込んでテキストエリアに初期表示
    if (LittleFS.exists("/config.json")) {
      //├┐（通常の場合）
        //◇┐既存ファイルを表示する
        File f = LittleFS.open("/config.json", "r");
        if (f) {
        //├┐（通常の場合）
          //○中身をバッファする
          while (f.available()) { html += (char)f.read(); }
          //○ファイルを閉じる
          f.close();
          //┴
        //└┐（その他）
          //○HTMLバッファに「エラーメッセージ」を追加
          } else { html += "ファイルを開けませんでした"; }
          //┴
      //└┐（その他）
        //┴
    //└┐（その他）
      //○HTMLバッファに「エラーメッセージ」を追加
    } else { html += "config.json が存在しません"; }
      //┴
    //│
    //○HTMLバッファに「本文」を追加
    html += "</textarea>";
    html += "<h2>設定ファイルの更新</h2>";
    html += "<div class=\"drop-zone\" id=\"dropZone\">";
    html += "<p>ここに設定ファイル（JSON）をドラッグ＆ドロップ</p>";
    html += "<p>または</p>";
    html += "<button class=\"btn\" onclick=\"document.getElementById('fileInput').click()\">ファイルを選択</button>";
    html += "<input type=\"file\" id=\"fileInput\" style=\"display: none;\" accept=\".json,text/plain\">";
    html += "</div>";
    html += "<div id=\"status\"></div>";
    //│
    //○HTMLバッファに「javascript」を追加
    html += "<script>";
    html += "const dropZone = document.getElementById('dropZone');";
    html += "const fileInput = document.getElementById('fileInput');";
    html += "['dragenter', 'dragover'].forEach(e => dropZone.addEventListener(e, (evt) => { evt.preventDefault(); dropZone.classList.add('dragover'); }, false));";
    html += "['dragleave', 'drop'].forEach(e => dropZone.addEventListener(e, (evt) => { evt.preventDefault(); dropZone.classList.remove('dragover'); }, false));";
    html += "dropZone.addEventListener('drop', (e) => { if (e.dataTransfer.files.length > 0) uploadFile(e.dataTransfer.files[0]); });";
    html += "fileInput.addEventListener('change', (e) => { if (fileInput.files.length > 0) uploadFile(fileInput.files[0]); });";
    html += "function uploadFile(file) {";
    html += "  const statusDiv = document.getElementById('status');";
    html += "  statusDiv.style.color = '#333'; statusDiv.textContent = 'アップロード中...';";
    html += "  const reader = new FileReader();";
    html += "  reader.onload = function(e) {";
    html += "    const content = e.target.result;";
    html += "    document.getElementById('configViewer').value = content;";
    html += "    fetch('/upload', { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: content })";
    html += "    .then(res => res.text().then(text => {";
    html += "      if (res.ok) {";
    html += "        statusDiv.style.color = 'green';";
    html += "        statusDiv.textContent = '成功: ' + text + ' 再起動します...';";
    html += "        window.location.href = '/reboot';";
    html += "      } else {";
    html += "        statusDiv.style.color = 'red'; statusDiv.textContent = '失敗: ' + text;";
    html += "      }";
    html += "    })).catch(err => { statusDiv.style.color = 'red'; statusDiv.textContent = '通信エラー: ' + err; });";
    html += "  };";
    html += "  reader.readAsText(file);";
    html += "}";
    html += "</script></body></html>";
    //│
    //○HTMLバッファをレスポンス
    ADP_SRV->send(200, "text/html", html);
    //┴
  } /* routeRoot() */

  //─────────────────
  // ルート２：設定フィルアップロード
  //─────────────────
  static void routeUpload() {
    //┬
    //○既存ファイルを削除
    if (LittleFS.exists("/config.json")) { LittleFS.remove("/config.json"); }
    //│
    //○新規ファイルを開く
    File configFile = LittleFS.open("/config.json", FILE_WRITE);
    if (!configFile) {
      ADP_SRV->send(500, "text/plain", "Failed to open config file for writing");
      return;
    }
    //│
    //○新規ファイルを作成
    if (ADP_SRV->hasArg("plain")) {
      String body = ADP_SRV->arg("plain");
      configFile.print(body);
    }
    //│
    //○新規ファイルを閉じる
    configFile.close();
    //│
    //○正常終了
    ADP_SRV->send(200, "text/plain", "設定ファイルを保存しました。");
    //┴
  } /* routeUpload() */

  //─────────────────
  // ルート３：再起動
  //─────────────────
  static void routeReboot() {
    String html = "<!DOCTYPE html><html lang=\"ja\"><head><meta charset=\"UTF-8\"><title>Rebooting...</title></head>";
    html += "<body style=\"font-family:sans-serif; text-align:center; padding-top:50px;\">";
    html += "<h2>MMPを再起動しています...</h2>";
    html += "<p>しばらくお待ちいただいた後、トップページへ戻ってください。</p>";
    html += "<p><a href=\"/\">トップ画面へ戻る</a></p>";
    html += "</body></html>";
    ADP_SRV->send(200, "text/html", html);

    // レスポンスの送信完了を確実にするため少し待ってから再起動
    delay(3000);
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_OFF);
    delay(500);
    ESP.restart();
} /* routeReboot() */

//========================================================
// Ｃ．初期化・ポーリング用ハンドル
//========================================================
  //━━━━━━━━━━━━━━━━━
  // WEBサーバのルーティング登録
  //━━━━━━━━━━━━━━━━━
  static void registRoutes(WebServer& server) {
    server.on("/"      , HTTP_GET , routeRoot  ); // アクセスで管理画面
    server.on("/upload", HTTP_POST, routeUpload); // 設定ファイルアップロード
    server.on("/reboot", routeReboot);            // 再起動画面（GET/POST両対応）
  } /* registRoutes() */

  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
  void START() {
    //┬
    //○１．前準備の完了状態を確認
    if (ADP_SRV ) {
    //│ ＼（通信デバイスが起動していない場合）
        //○エラーメッセージを表示
        //○無効化
        //▼終了：異常終了
        Serial.println("　WEB       : ＷＥＢサーバが起動していません ");
        ENABLED = false; // 無効
        return;
    } /* END-if */
    //│
    //○２．対象の通信経路を宣言
    //　➡【該当処理なし】
    //│
    //○３．サーバ資源生成
    ADP_SRV = new WebServer(SRV_PORT); // WebServer
    //│
    //○４．接続管理TBLを作成
    //　➡【該当処理なし】
    //│
    //○５．ルーティング登録
    registRoutes(*ADP_SRV);
    //│
    //○６．サーバ開始
    ADP_SRV->begin();
    //│
    //○┐７．成功終了
      //○成功メッセージ
      //○有効化
      Serial.println(String("　WEB       : OK -> port ") + String(SRV_PORT));
      ENABLED = true; // 有効
      //┴
    //┴
  } /* START() */

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口（ポーリング入口）
  //━━━━━━━━━━━━━━━━━
  void HANDLE() {
   //┬
    //○１．起動チェック
    if (!ENABLED) return;
    if (!ADP_SRV) return;
    //│＼（このアダプタが無効の場合）
    //│ ▼終了：早期リターン
    //│
    //○２．新規接続のスロットを登録
    //　➡【該当処理なし】※通信アダプタが対象
    //│
    //○３．ルーティング処理
    ADP_SRV->handleClient();
    //┴
  } /* HANDLE() */
} //* namespace adpWEB */