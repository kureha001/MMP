#include "MmpClient.h"

using namespace Mmp::Core;

//============================================================
// 低レイヤ コマンドの実装
//============================================================
//【内部利用】
    //-------------------
    // 接続：条件指定
    //-------------------
    bool MmpClient::Open(
        uint32_t baud,
        int32_t /*timeoutIo*/
    ){

        // 既に開いている場合は閉じる
        Close();

        // 指定の通信速度でポートを開く。
        _uart = &MMP_UART;
        _uart->begin(baud);
        delay(100);

        // 入力バッファを消去する。
        ClearInput();

        // 接続情報を更新する。
        _isOpen = true;

        return true;
}

    //-------------------
    // バージョンチェック
    //-------------------
    bool MmpClient::Verify(
        int32_t timeoutVerify
    ){
        // VER! に対し 5 文字（末尾 '!'）が返れば接続 OK とみなす（内容は数値前提にしない）
        String s = GetVersion(timeoutVerify);
        return (s.length() == 5 && s[4] == '!');
    }

    //-------------------
    // ＭＭＰシリアル通信
    //-------------------
    String MmpClient::SendCommand(
        const String& command,
        int32_t timeoutMs
    ){
        _lastError = "";
        if (!_isOpen) { _lastError = "UART not open."; return String(); }

        // 必ず '!' 終端にする
        String cmd = command;
        if (cmd.length() == 0 || cmd.charAt(cmd.length() - 1) != '!') {cmd += '!';}

        // 送信前に入力をクリア
        ClearInput();

        // 送信
        _uart->print(cmd);

        // 応答待ち（固定 5 文字）。末尾 5 文字だけ保持。
        String buf;
        unsigned long deadline = millis() + (timeoutMs > 0 ? (unsigned long)timeoutMs : 1UL);

        while ((long)(deadline - millis()) > 0) {

            while (_uart->available() > 0) {
                char c = (char)_uart->read();

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

        // 接続情報を更新する。
        _lastError = "Timeout or invalid 5-char reply";

        return "";           // 戻り値 → [失敗]
    }

//【外部公開】
    //-------------------
    // 接続：自動
    //-------------------
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

    //-------------------
    // 接続：条件指定
    //-------------------
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

    //-------------------
    // 切断
    //-------------------
    void MmpClient::Close(){
        // ポートが開いていたら閉じる。
        if (_isOpen) {

            // ポートを閉じる。
            _uart->flush();
            _uart->end();

            // 接続情報を更新する。
            _isOpen = false;
        }
    }

//============================================================
//【内部利用】フラットＡＰＩの実装
//============================================================

    //===========================
    // << 情報モジュール >>
    //===========================
        //----------------------
        // １．バージョン
        //----------------------
        String MmpClient::GetVersion(
            int32_t timeoutMs
        ){
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutGeneral);
            return SendCommand("VER!", t);
        }

        //----------------------
        // 2-1.ＰＷＭデバイス
        //----------------------
        uint16_t MmpClient::GetPwx(
            int32_t devId0to15,
            int32_t timeoutMs
        ){
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutPwm);
            String resp = SendCommand("PWX:" + Hex2(devId0to15) + "!", t);
            uint16_t v = 0;
            if (!TryParseHex4Bang(resp, v)) {
                _lastError = "PWX bad response.";
                return 0;
        }
        return v;
        }

        //----------------------
        // 2-2.音声デバイス
        //----------------------
        uint16_t MmpClient::GetDpx(
            int32_t devId1to4,
            int32_t timeoutMs
        ){
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
            String resp = SendCommand("DPX:" + Hex2(devId1to4) + "!", t);
            uint16_t v = 0;

            if (!TryParseHex4Bang(resp, v)) {
                _lastError = "DPX bad response.";
                return 0;
            }
            return v;
        }

    //===========================
    // << アナログ モジュール >>
    //===========================
        //-----------------------------------------
        // １．使用範囲設定
        //-----------------------------------------
        bool MmpClient::AnalogConfigure(
            int32_t hc4067chTtl,
            int32_t hc4067devTtl,
            int32_t timeoutMs
        ){
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutAnalog);
            if (hc4067chTtl  < 1 || hc4067chTtl  > 16) { _lastError = "AnalogConfigure: hc4067chTtl out of range." ; return false; }
            if (hc4067devTtl < 1 || hc4067devTtl > 4 ) { _lastError = "AnalogConfigure: hc4067devTtl out of range."; return false; }

            String resp = SendCommand("ANS:" + Hex2(hc4067chTtl) + ":" + Hex2(hc4067devTtl) + "!", t);
            return (resp == "!!!!!");
        }

        //-----------------------------------------
        // ２．測定値バッファ更新
        //-----------------------------------------
        bool MmpClient::AnalogUpdate(
            int32_t timeoutMs
        ){
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutAnalog);
            String resp = SendCommand("ANU!", t);
            return (resp == "!!!!!");
        }

        //-----------------------------------------
        // ３．測定値バッファ読取（丸めなし）
        // ４．測定値バッファ読取（中央値基準の丸め）
        // ５．測定値バッファ読取（切り上げ丸め）
        // ６．測定値バッファ読取（切り捨て丸め）
        //-----------------------------------------
        int32_t MmpClient::AnalogRead(
            int32_t hc4067ch0to15,  //
            int32_t hc4067dev0to3,  //
            int32_t timeoutMs       //
        ){
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutAnalog);
            if (hc4067ch0to15 < 0 || hc4067ch0to15 > 15) { _lastError = "AnalogRead: hc4067ch0to15 out of range."; return -1; }
            if (hc4067dev0to3 < 0 || hc4067dev0to3 > 3 ) { _lastError = "AnalogRead: hc4067dev0to3 out of range."; return -1; }

            String resp = SendCommand("ANR:" + Hex2(hc4067ch0to15) + ":" + Hex2(hc4067dev0to3) + "!", t);
            uint16_t v = 0;
            if (TryParseHex4Bang(resp, v)) return (int32_t)v;
            _lastError = "AnalogRead: bad response.";
            return -1;
        }


    //===========================
    // << デジタル モジュール >>
    //===========================
        //---------------------
        // １．入力
        //---------------------
        int32_t MmpClient::DigitalIn(
            int32_t gpioId  ,   //
            int32_t timeoutMs   //
        ){
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutDigital);
            String resp = SendCommand("POR:" + Hex2(gpioId) + "!", t);
            uint16_t v = 0;
            if (TryParseHex4Bang(resp, v)) return (int32_t)v;
            _lastError = "DigitalIn: bad response.";
            return 0;
        }

        //---------------------
        // ２．出力
        //---------------------
        bool MmpClient::DigitalOut(
            int32_t gpioId  ,   //
            int32_t val0or1 ,   //
            int32_t timeoutMs   //
        ){
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutDigital);
            String resp = SendCommand("POW:" + Hex2(gpioId) + ":" + ((val0or1 & 1) ? "1" : "0") + "!", t);
            return (resp == "!!!!!");
        }


    //===========================
    // << ＰＷＭモジュール >>
    //===========================
        //---------------------
        // １．デバイス情報
        //---------------------

        //---------------------
        // ２．出力
        //---------------------
        bool MmpClient::PwmValue(
            int32_t chId0to255, //
            int32_t val0to4095, //
            int32_t timeoutMs   //
        ){                      //
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutPwm);
            String resp = SendCommand("PWM:" + Hex2(chId0to255) + ":" + Hex4(val0to4095) + "!", t);
            return (resp == "!!!!!");
        }

        //---------------------
        // ３．角度設定
        //---------------------
        bool MmpClient::PwmInit(
            int32_t angleMin,   //
            int32_t angleMax,   //
            int32_t pwmMin  ,   //
            int32_t pwmMax  ,   //
            int32_t timeoutMs   //
        ){                      //
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutPwm);
            String resp = SendCommand("PWI:" + Hex3(angleMin) + ":" + Hex3(angleMax) + ":" + Hex4(pwmMin) + ":" + Hex4(pwmMax) + "!", t);
            return (resp == "!!!!!");
        }

        //---------------------
        // ４．角度指定
        //---------------------
        bool MmpClient::PwmAngle(
            int32_t chId0to255  ,   //
            int32_t angle0to180 ,   //
            int32_t timeoutMs       //
        ){                          //
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutPwm);
            String resp = SendCommand("PWA:" + Hex2(chId0to255) + ":" + Hex3(angle0to180) + "!", t);
            return (resp == "!!!!!");
        }


    //===========================
    // << 音声モジュール >>
    //===========================
        //----------------------
        // １. 単独コマンド
        //----------------------
            //----------------------
            // 1-1. デバイス情報
            //----------------------

            //----------------------
            // 1-2. 音量設定
            //----------------------
            bool MmpClient::DfVolume(
                int32_t devId1to4   ,   //
                int32_t vol0to30    ,   // 
                int32_t timeoutMs       //
            ){                          //
                const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
                String resp = SendCommand(
                    "VOL:" + Hex2(devId1to4)
                    + ":" + Hex2(vol0to30)
                    + "!", t
                );
                return (resp == "!!!!!");
            }

            //----------------------
            // 1-3.イコライザ設定
            //----------------------
            bool MmpClient::DfSetEq(
                int32_t devId1to4   ,   //
                int32_t mode0to5    ,   //
                int32_t timeoutMs       //
            ){                          //
                const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
                String resp = SendCommand(
                    "DEF:" + Hex2(devId1to4)
                    + ":" + Hex2(mode0to5)
                    + "!", t
                );
                return (resp == "!!!!!");
            }

        //------------------------
        // ２. 再生サブモジュール
        //------------------------
            //----------------------
            // 2-1. 再生（Start）
            //----------------------
            bool MmpClient::DfStart(
                int32_t devId1to4   ,   //
                int32_t dir1to255   ,   //
                int32_t file1to255  ,   //
                int32_t timeoutMs       //
            ){                          //
                const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
                String resp = SendCommand(
                    "DIR:" + Hex2(devId1to4)
                    + ":" + Hex2(dir1to255)
                    + ":" + Hex2(file1to255) 
                    + "!", t
                );
            return (resp == "!!!!!");
            }

            //----------------------
            // 2-2. ループ再生指定
            //----------------------
            bool MmpClient::DfSetLoop(
                int32_t devId1to4   ,   //
                int32_t on0or1      ,   //
                int32_t timeoutMs       //
            ){                          //
                const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
                String resp = SendCommand(
                    "DLP:"+ Hex2(devId1to4)
                    + ":" + Hex2(on0or1 ? 1 : 0)
                    + "!", t
                );
                return (resp == "!!!!!");
            }

            //----------------------
            // 2-3. 停止
            //----------------------
            bool MmpClient::DfStop(
                int32_t devId1to4,  //
                int32_t timeoutMs   //
            ){                      //
                const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
                String resp = SendCommand(
                    "DSP:" + Hex2(devId1to4)
                    + "!", t
                );
                return (resp == "!!!!!");
            }

            //----------------------
            // 2-4. 一時停止
            //----------------------
            bool MmpClient::DfPause(
                int32_t devId1to4,  //
                int32_t timeoutMs   //
            ){                      //
                const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
                String resp = SendCommand(
                    "DPA:" + Hex2(devId1to4)
                    + "!", t
                );
                return (resp == "!!!!!");
            }

            //----------------------
            // 2-5. 再開
            //----------------------
            bool MmpClient::DfResume(
                int32_t devId1to4,  //
                int32_t timeoutMs   //
            ){                      //
                const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
                String resp = SendCommand(
                    "DPR:" + Hex2(devId1to4)
                    + "!", t
                );
                return (resp == "!!!!!");
            }

        //------------------------
        // ３. 参照サブモジュール
        //------------------------
            //----------------------
            // 3-1. 再生状況
            //----------------------
            int32_t MmpClient::DfReadState(
                int32_t devId1to4,  //
                int32_t timeoutMs   //
            ){                      //
                const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
                String resp = SendCommand(
                    "DST:" + Hex2(devId1to4)
                    + ":" + Hex2(1) + "!", t
                );
                uint16_t v = 0;
                if (TryParseHex4Bang(resp, v)) return (int32_t)v;
                _lastError = "DfReadState: bad response.";
                return -1;
            }

            //----------------------
            // 3-2. ボリューム
            //----------------------
            int32_t MmpClient::DfReadVolume(
                int32_t devId1to4,  //
                int32_t timeoutMs   //
            ){                      //
                const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
                String resp = SendCommand(
                    "DST:" + Hex2(devId1to4)
                    + ":" + Hex2(2)
                    + "!", t
                );
                uint16_t v = 0;
                if (TryParseHex4Bang(resp, v)) return (int32_t)v;
                _lastError = "DfReadVolume: bad response.";
                return -1;
            }

            //----------------------
            // 3-3. イコライザのモード
            //----------------------
            int32_t MmpClient::DfReadEq(
                int32_t devId1to4,  //
                int32_t timeoutMs   //
            ){                      //
                const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
                String resp = SendCommand(
                    "DST:" + Hex2(devId1to4)
                    + ":" + Hex2(3)
                    + "!", t
                );
                uint16_t v = 0;
                if (TryParseHex4Bang(resp, v)) return (int32_t)v;
                _lastError = "DfReadEq: bad response.";
                return -1;
            }

            //----------------------
            // 3-4. 総ファイル総数
            //----------------------
            int32_t MmpClient::DfReadFileCounts(
                int32_t devId1to4,
                int32_t timeoutMs
            ){
                const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
                String resp = SendCommand(
                    "DST:" + Hex2(devId1to4)
                    + ":" + Hex2(4)
                    + "!", t
                );
                uint16_t v = 0;
                if (TryParseHex4Bang(resp, v)) return (int32_t)v;
                _lastError = "DfReadFileCounts: bad response.";
                return -1;
            }

            //----------------------
            // 3-5. 現在のファイル番号
            //----------------------
            int32_t MmpClient::DfReadFileNumber(
                int32_t devId1to4,
                int32_t timeoutMs
            ){
                const int32_t t = Resolve(timeoutMs, Settings.TimeoutAudio);
                String resp = SendCommand(
                    "DST:" + Hex2(devId1to4)
                    + ":" + Hex2(5)
                    + "!", t
                );
                uint16_t v = 0;
                if (TryParseHex4Bang(resp, v)) return (int32_t)v;
                _lastError = "DfReadFileNumber: bad response.";
                return -1;
            }


    //===========================
    // << Ｉ２Ｃモジュール >>
    //===========================
        //----------------------
        // １．書き込み
        //----------------------
        bool MmpClient::I2cWrite(
            int32_t addr,       //
            int32_t reg ,       //
            int32_t val ,       //
            int32_t timeoutMs   //
        ){                      //
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutI2c);
            String resp = SendCommand(
                "I2W:" + Hex2(addr)
                + ":" + Hex2(reg)
                + ":" + Hex2(val)
                + "!", t
            );
            return (resp == "!!!!!");
        }

        //----------------------
        // ２．読み出し
        //----------------------
        int32_t MmpClient::I2cRead (
            int32_t addr    ,   //
            int32_t reg     ,   //
            int32_t timeoutMs   //
        ){                      //
            const int32_t t = Resolve(timeoutMs, Settings.TimeoutI2c);
            String resp = SendCommand(
                "I2R:" + Hex2(addr)
                + ":" + Hex2(reg)
                + "!", t
            );
            uint16_t v = 0;
            if (TryParseHex4Bang(resp, v)) return (int32_t)v;
            _lastError = "I2cRead: bad response.";
            return 0;
        }