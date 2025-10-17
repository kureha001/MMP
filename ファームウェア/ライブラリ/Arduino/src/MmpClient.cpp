// MmpClient.cpp
//============================================================
// ＭＭＰクライアント（コア機能／実態）
//============================================================
#include "MmpClient.h"

//━━━━━━━━━━━━━━━
// ＴＣＰ通信が必要な場合
//━━━━━━━━━━━━━━━
#if defined(MMP_ENABLE_TCP)
  #include <WiFi.h>
  // この翻訳単位内で完結する TCP ハンドル（メンバ不要）
  static WiFiClient s_mmp_tcp;
#endif


//━━━━━━━━━━━━━━━
// ネームスペース
//━━━━━━━━━━━━━━━
using namespace Mmp::Core;

//━━━━━━━━━━━━━━━
// 低レイヤ コマンド：シリアル用
//━━━━━━━━━━━━━━━
//【内部利用】
    //─────────────
    // 接続：条件指定（UART）
    //─────────────
    bool MmpClient::Open(
        uint32_t baud,
        int32_t /*timeoutIo*/
    ){

        // 既に開いている場合は閉じる
        Close();

        // 指定の通信速度でポートを開く。
        _serial = &MMP_UART;
        _serial->begin(baud);
        delay(100);

        // I/O ハンドルを差し替え
        _handle = _serial;

        // 入力バッファを消去する。
        ClearInput();

        // 接続情報を更新する。
        _isOpen = true;

        return true;
}

    //─────────────
    // バージョンチェック
    //─────────────
    bool MmpClient::Verify(
        int32_t timeoutVerify
    ){
        String s = INFO_VERSION();
        return (s.length() == 5 && s[4] == '!');
    }

    //─────────────
    // ＭＭＰシリアル通信
    //─────────────
    String MmpClient::SendCommand(
        const String& command,
        int32_t timeoutMs
    ){
        _lastError = "";
        if (!_isOpen || !_handle) { _lastError = "I/O not open."; return String(); }

        // 必ず '!' 終端にする
        String cmd = command;
        if (cmd.length() == 0 || cmd.charAt(cmd.length() - 1) != '!') {cmd += '!';}

        // 送信前に入力をクリア
        ClearInput();

        // 送信
        _handle->print(cmd);

        // 応答待ち（固定 5 文字）。末尾 5 文字だけ保持。
        String buf;
        unsigned long deadline = millis() + (timeoutMs > 0 ? (unsigned long)timeoutMs : 1UL);

        while ((long)(deadline - millis()) > 0) {

            while (_handle->available() > 0) {
                char c = (char)_handle->read();

                // プロトコルは 5 文字固定フレームなので改行は無視
                if (c == '\r' || c == '\n') {continue;}

                buf += c;
                if (buf.length() > 5) buf.remove(0, buf.length() - 5);

                // 戻り値の書式が正しければ、正常で返す。
                if (buf.length() == 5 && buf[4] == '!') {return buf;}
                            // 戻り値 → [正常]
            }

            // 少し待つ
            delay(1);
        }

        // 接続情報を更新
        _lastError = "Timeout or invalid 5-char reply";
        return "";           // 戻り値 → [失敗]
    }

//━━━━━━━━━━━━━━━
// 低レイヤ コマンド：ＴＣＰ用
//━━━━━━━━━━━━━━━
#if defined(MMP_ENABLE_TCP)
    //─────────────
    // 接続：条件指定（TCP）
    //─────────────
    bool MmpClient::OpenTcp(
        const char* host,
        uint16_t    port,
        uint32_t    timeoutIoMs
    ){
        // 既に開いている場合は閉じる
        Close();

        _lastError = "";
        if (!host || !*host) { _lastError = "host empty"; return false; }

        if (!s_mmp_tcp.connect(host, port)) {
            _lastError = "connect failed";
            return false;
        }
        s_mmp_tcp.setTimeout(timeoutIoMs);

        // I/O ハンドルを差し替え
        _handle = &s_mmp_tcp;

        // 入力バッファを消去する。
        ClearInput();

        // 接続情報を更新
        _isOpen           = true;
        _connected_port   = String("tcp://") + host + ":" + String(port);
        _connected_baud   = 0; // TCP なので実意味なし（記録のみ）

        return true;
    }

    //─────────────
    // 接続：TCP + Verify
    //─────────────
    bool MmpClient::ConnectTcp(
        const char* host,
        uint16_t    port,
        uint32_t    ioTimeoutMs,
        uint32_t    verifyTimeoutMs
    ){
        // 接続情報を初期化する。
         _isOpen            = false;
        _connected_port     = "";
        _connected_baud     = 0;
        _lastError          = "";

        if (!OpenTcp(host, port, ioTimeoutMs)) {
            _lastError = "OpenTcp failed.";
            return false;
        }
        if (!Verify(verifyTimeoutMs)) {
            Close();
            _lastError = "Verify failed.";
            return false;
        }
        Settings.PortName = _connected_port;
        Settings.BaudRate = _connected_baud;
        _lastError        = "";
        return true;
    }
#endif

//【外部公開】
    //─────────────
    // 接続：自動（UART）
    //─────────────
    bool MmpClient::ConnectAutoBaud(
        const uint32_t* cand,   // ①通信速度一覧
        size_t n                // ②
    ){                          // 戻り値：戻り値：True=成功／False=失敗

        // 接続情報を初期化する。
         _isOpen            = false;
        _connected_port     = "";
        _connected_baud     = 0;
        _lastError          = "";

        // 通信速度一覧の指定が無い場合、規定値を用いる。
        if (cand == nullptr || n == 0) {
            cand = MMP_BAUD_CANDIDATES;
            n = MMP_BAUD_CANDIDATES_LEN;
        }

        // 通信速度一覧の通信速度を用いて、総当たりで接続を試みる。
        for (size_t i = 0; i < n; ++i) {
            if (ConnectWithBaud(
                cand[i],
                Settings.TimeoutIo,
                Settings.TimeoutVerify)
            ) {return true;}        // 戻り値 → [成功]
        }
    
        // 接続情報を更新する。
        _lastError = "No baud matched.";
        return false;               // 戻り値 → [失敗]
    }

    //─────────────
    // 接続：条件指定（UART）
    //─────────────
    bool MmpClient::ConnectWithBaud(
        uint32_t baud           ,   // ① 通信速度
        int32_t timeoutIo       ,   // ② 一般用タイムアウト(単位；ミリ秒)
        int32_t verifyTimeoutMs     // ③ ベリファイ用タイムアウト(単位；ミリ秒)
    ){                              // 戻り値：True=成功／False=失敗

        // 接続情報を初期化する。
         _isOpen             = false;
        _connected_port     = "";
        _connected_baud     = 0;
        _lastError          = "";

        // 指定の通信速度でポートを開く。
        if (!Open(baud, timeoutIo)) {

            // 接続情報を更新する。
            _lastError = "Open failed.";
            return false;       // 戻り値 → [失敗]
        }

        // バージョンを取得して、ベリファイを実行する。
        if (!Verify(verifyTimeoutMs)) {

            // 通信を切断する。
            Close();

            // 接続情報を更新する。
            _lastError = "Verify failed.";

            return false;       // 戻り値 → [失敗]
        }

        // 接続情報を更新する。
        _connected_port   = String("UART1");
        _connected_baud   = baud;
        Settings.PortName = _connected_port;
        Settings.BaudRate = _connected_baud;
        _lastError        = "";

        return true;            // 戻り値 → [成功]
    }

    //─────────────
    // 切断（UART/TCP共通）
    //─────────────
    void MmpClient::Close(){
        // ポートが開いていたら閉じる。
        if (_isOpen) {

            // TCP の場合
            #if defined(MMP_ENABLE_TCP)
                if (_handle == (Stream*)&s_mmp_tcp) {
                    s_mmp_tcp.stop();
                }
            #endif

            // UART の場合
            if (_serial && _handle == _serial) {
                _serial->flush();
                _serial->end();
            }

            // 接続情報を更新する。
            _handle = nullptr;
            _isOpen = false;
        }
    }

//━━━━━━━━━━━━━━━
// モジュール：システム情報
//━━━━━━━━━━━━━━━
    //─────────────
    // バージョン
    //─────────────
    String MmpClient::INFO_VERSION(){
        return SendCommand("INFO/VERSION!", Settings.TimeoutGeneral);
    }

//━━━━━━━━━━━━━━━
// モジュール：汎用
//━━━━━━━━━━━━━━━
    //─────────────
    // 数値取得型
    //─────────────
    int MmpClient::RunGetVal(
        int32_t argTime,        //
        const   String& path,   // 例: F("MP3/INFO")
        const   String& cmd ,   // 例: F("TRACK")
        int     arg1 /*-1000*/,
        int     arg2 /*-1000*/,
        int     arg3 /*-1000*/,
        int     arg4 /*-1000*/
    ){
        // リクエスト(URI)を作成する
        String uri;
        uri.reserve(32);
        uri += path; uri += '/'; uri += cmd;
        const int args[4] = {arg1, arg2, arg3, arg4};
        for (int i = 0; i < 4; ++i) {
        if (args[i] != -1000) { uri += ':'; uri += args[i]; }
        }
        uri += '!';

        // 作成したURIでMMPにリクエストし、レスポンスを受け取る
        String resp = SendCommand(uri, argTime);

        // レスポンスから値を抽出する
        // （数値：true／エラー：false＋-9000番台コード）
        int v; bool ok = TryParseDecBang5(resp, v);

        // 抽出結果が異常の場合、エラーメッセージを残す
        if (!ok){_lastError = "[" + resp + "] " + uri;}

        // 正常・異常に関わらず、値を返す
        return v;
    }

    //─────────────
    // 合否型
    //─────────────
    bool MmpClient::RunGetOK(
        int32_t argTime,        //
        const   String& path,   // 例: F("MP3/INFO")
        const   String& cmd ,   // 例: F("TRACK")
        int     arg1 /*-1000*/,
        int     arg2 /*-1000*/,
        int     arg3 /*-1000*/,
        int     arg4 /*-1000*/,
        int     arg5 /*-1000*/,
        int     arg6 /*-1000*/,
        int     arg7 /*-1000*/,
        int     arg8 /*-1000*/
    ){
        // リクエスト(URI)を作成する
        String uri;
        uri.reserve(32);
        uri += path; uri += '/'; uri += cmd;
        const int args[8] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8};
        for (int i = 0; i < 8; ++i) {
        if (args[i] != -1000) { uri += ':'; uri += args[i]; }
        }
        uri += '!';

        // 作成したURIでMMPにリクエストし、レスポンスを受け取る
        String resp = SendCommand(uri, argTime);

        // 正常値を返す
        if (resp == "!!!!!") return true;

        // 抽出結果が異常の場合、エラーメッセージを残す
        // 異常値を返す
        _lastError = "[" + resp + "] " + uri;
        return false;
    }
