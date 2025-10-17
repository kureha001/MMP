// MmpClient.h
//============================================================
// ＭＭＰクライアント（コア機能／ヘッダ）
//============================================================
#pragma once
#include <Arduino.h>
#include <Stream.h>
#include "hardware.h"

//━━━━━━━━━━━━━━━
// ＴＣＰ通信が必要な場合
//━━━━━━━━━━━━━━━
#if defined(MMP_ENABLE_TCP)
#include <WiFi.h>
#endif

//━━━━━━━━━━━━━━━
// ネームスペース
//━━━━━━━━━━━━━━━
namespace Mmp { namespace Core{
class MmpClient{

//━━━━━━━━━━━━━━━
// プロパティ
//━━━━━━━━━━━━━━━
private:

    // I/Oハンドラ（UART/TCPを共通化）
    Stream*         _handle         = nullptr;

    // UART 実体（従来の Serial。必要に応じて &MMP_UART を使う）
    HardwareSerial* _serial         = &MMP_UART;

    // TCP 実体（Pico2W）
    #if defined(USE_PICO2W_WIFI)
        WiFiClient      _tcp;
    #endif

    // 接続状況
    bool    _isOpen             = false;
    String  _connected_port     = "";
    int32_t _connected_baud     = 0;
    String  _lastError          = "";

public:
    //─────────────
    // 接続情報
    //─────────────
    bool    IsOpen()         const { return _isOpen;          }
    String  ConnectedPort()  const { return _connected_port;  }
    int32_t ConnectedBaud()  const { return _connected_baud;  }
    String  LastError()      const { return _lastError;       }

    //─────────────
    // 規定値
    //─────────────
    struct MmpSettings {
        String  PortName       = "";
        int32_t BaudRate       = 921600;
        int32_t TimeoutIo      = 200;
        int32_t TimeoutVerify  = 400;
        int32_t TimeoutGeneral = 400;
        int32_t TimeoutAnalog  = 400;
        int32_t TimeoutPwm     = 400;
        int32_t TimeoutAudio   = 400;
        int32_t TimeoutDigital = 400;
        int32_t TimeoutI2c     = 400;
        int32_t AnalogBits     = 10;
    };
    MmpSettings Settings;


//━━━━━━━━━━━━━━━
// 低レイヤ コマンド
//━━━━━━━━━━━━━━━
//【内部利用】
private:

    bool   Open       (uint32_t baud,         int32_t timeoutIo);
    bool   Verify     (int32_t timeoutVerify                   );
    String SendCommand(const String& command, int32_t timeoutMs);
    inline void ClearInput() {
        if (!_handle) return;
        while (_handle->available() > 0) (void)_handle->read();
    }

    // 追加：TCPオープン（ser2net等）
    bool OpenTcp(
        const char* host        ,
        uint16_t    port        ,
        uint32_t    timeoutIoMs );


//【外部公開】
public:
    //─────────────
    // 接続：TCP + Verify
    //─────────────
    bool ConnectTcp (
        const char* host                    ,
        uint16_t    port                    ,
        uint32_t    ioTimeoutMs     = 300   ,
        uint32_t    verifyTimeoutMs = 600   );

    //─────────────
    // 接続：自動（UART）
    //─────────────
    bool ConnectAutoBaud(
        const uint32_t* cand = MMP_BAUD_CANDIDATES      ,
        size_t          n    = MMP_BAUD_CANDIDATES_LEN  );

    //─────────────
    // 接続：条件指定（UART）
    //─────────────
    bool ConnectWithBaud(
        uint32_t baud                   ,
        int32_t  timeoutIo       = 0    ,
        int32_t  verifyTimeoutMs = 0    );

    //─────────────
    // 切断（UART/TCP共通）
    //─────────────
    void Close();


//━━━━━━━━━━━━━━━
// モジュール コマンド
//━━━━━━━━━━━━━━━
//【外部公開】階層ＡＰＩ
public:
    MmpClient():INFO(this), ANALOG(this), DIGITAL(this), PWM(this), MP3(this), I2C(this) {}

private:
    int RunGetVal(
        int32_t argTime,
        const   String& path,   // 例: F("MP3/INFO")
        const   String& cmd,    // 例: F("TRACK")
        int     arg1 = -1000,
        int     arg2 = -1000,
        int     arg3 = -1000,
        int     arg4 = -1000
        );

    bool RunGetOK(
        int32_t argTime,
        const   String& path,   // 例: F("MP3/INFO")
        const   String& cmd,    // 例: F("TRACK")
        int     arg1 = -1000,
        int     arg2 = -1000,
        int     arg3 = -1000,
        int     arg4 = -1000,
        int     arg5 = -1000,
        int     arg6 = -1000,
        int     arg7 = -1000,
        int     arg8 = -1000
        );

//━━━━━━━━━━━━━━━
// モジュール：システム情報
//━━━━━━━━━━━━━━━
//【内部利用】フラットＡＰＩ
private:
    String   INFO_VERSION();

//【外部公開】階層ＡＰＩ
public:

    class mod_Info {
    MmpClient* _p;
    public:
        explicit mod_Info(MmpClient* p): _p(p) {}

        //─────────────
        // バージョン
        //─────────────
        String VERSION() { return _p->INFO_VERSION(); }
    } INFO;


//━━━━━━━━━━━━━━━
// モジュール：アナログ入力
//━━━━━━━━━━━━━━━
public:
    class mod_Analog {
    MmpClient* _p;
    public:
        explicit mod_Analog(MmpClient* p): _p(p) {}
        int32_t t = _p->Settings.TimeoutAnalog;

        //─────────────
        // 使用範囲設定
        //─────────────
        bool SETUP(
            int chs,    //
            int devs    //
        ){return _p->RunGetOK(t, "ANALOG","SETUP", chs, devs);}

        //─────────────
        // 信号入力(バッファ格納)
        //─────────────
        bool INPUT(){return _p->RunGetOK(t, "ANALOG","INPUT");}

        //─────────────
        // バッファ読取：丸めなし
        //─────────────
        int READ(
            int ch, //
            int dev //
        ){return _p->RunGetVal(t, "ANALOG","READ", ch, dev);}

        //─────────────
        // バッファ読取：中央基準
        //─────────────
        int ROUND(
            int ch,         //
            int dev,        //
            int step,       //
            int bits = 0    //
        ) {
            int raw = _p->RunGetVal(t, "ANALOG","READ", ch, dev);
            return _roundDown(raw, step, bits, _p->Settings.AnalogBits);
        }
        //─────────────
        // バッファ読取：切り上げ
        //─────────────
        int ROUNDU(
            int ch,         //
            int dev,        //
            int step,       //
            int bits = 0    //
        ) {
            int raw = _p->RunGetVal(t, "ANALOG","READ", ch, dev);
            return _roundUp(raw, step, bits, _p->Settings.AnalogBits);
        }
        //─────────────
        // バッファ読取：切り下げ
        //─────────────
        int ROUNDD(
            int ch,         //
            int dev,        //
            int step,       //
            int bits = 0    //
        ) {
            int raw = _p->RunGetVal(t, "ANALOG","READ", ch, dev);
            return _roundNearest(raw, step, bits, _p->Settings.AnalogBits);
        }
    } ANALOG;

//━━━━━━━━━━━━━━━
// モジュール：デジタル入出力
//━━━━━━━━━━━━━━━
public:
    class mod_Digital {
    MmpClient* _p;
    public:
        explicit mod_Digital(MmpClient* p): _p(p) {}
        int32_t t = _p->Settings.TimeoutDigital;

        //─────────────
        // 信号入力
        //─────────────
        int INPUT(
            int gpioId
        ){return _p->RunGetVal(t, "DIGITAL","INPUT" , gpioId);}

        //─────────────
        // 信号出力
        //─────────────
        bool OUTPUT(
            int gpioId,
            int val
        ){return _p->RunGetOK(t, "ANALOG","SETUP", gpioId, ((val & 1) ? 1 : 0));}
    } DIGITAL;

//━━━━━━━━━━━━━━━
// モジュール：ＰＷＭ出力
//━━━━━━━━━━━━━━━
public:
    class mod_PWM {
    MmpClient* _p;
    public:
        explicit mod_PWM(MmpClient* p): _p(p), INFO(p), ANGLE(p) {}
        int32_t t = _p->Settings.TimeoutPwm;

        //─────────────
        // ＰＷＭ値出力
        //─────────────
        bool OUTPUT(
            int ch  ,
            int val
            ){return _p->RunGetOK(t, "PWM/ANGLE","DELETE", ch, val);}

        //─────────────
        // サブ：インフォメーション
        //─────────────
        class sub_Info {
            MmpClient* _p;
            public:
                explicit sub_Info(MmpClient* p): _p(p) {}
                int32_t t = _p->Settings.TimeoutPwm;
            //─────────────
            // デバイス接続状況
            //─────────────
            int CONNECT(
                int dev
            ){return _p->RunGetVal(t, "PWM/INFO","CONNECT" , dev);}
        } INFO;

        //─────────────
        // サブ：角度指定
        //─────────────
        class sub_Angle {

            MmpClient* _p;
            public:
                explicit sub_Angle(MmpClient* p): _p(p) {}
                int32_t t = _p->Settings.TimeoutPwm;
            //─────────────
            // プリセット登録
            //─────────────
            bool SETUP(
                int chFrom,     //
                int chTo,       //
                int degTo,      //
                int pwmFrom,    //
                int pwmTo,      //
                int pwmMiddle   //
            ){return _p->RunGetOK(t, "PWM/ANGLE","SETUP", chFrom, chTo, degTo, pwmFrom, pwmTo, pwmMiddle);}

            //─────────────
            // プリセット削除
            //─────────────
            bool DELETE(
                int chFrom, //
                int chTo    //
            ){return _p->RunGetOK(t, "PWM/ANGLE","DELETE", chFrom, chTo);}

            //─────────────
            // ＰＷＭ出力：通常
            //─────────────
            int OUTPUT(
                int ch,
                int angle
            ){return _p->RunGetVal(t, "PWM/ANGLE","OUTPUT" , ch, angle);}

            //─────────────
            // ＰＷＭ出力：中間
            //─────────────
            int CENTER(
                int ch
            ){return _p->RunGetVal(t, "PWM/ANGLE","CENTER" , ch);}
        } ANGLE;

        //─────────────
        // サブ：回転型サーボ
        //─────────────
            //─────────────
            // プリセット登録
            // プリセット削除
            // ＰＷＭ出力：通常
            // ＰＷＭ出力：停止
            //─────────────
    } PWM;

//━━━━━━━━━━━━━━━
// モジュール：ＭＰ３プレイヤー
//━━━━━━━━━━━━━━━
public:

    class mod_MP3 {
    MmpClient* _p;
    public:
        explicit mod_MP3(MmpClient* p): _p(p), SET(p), TRACK(p), INFO(p) {}

        //─────────────
        // サブ：デバイス設定
        //─────────────
        class sub_Set {
            MmpClient* _p;
            public:
                explicit sub_Set(MmpClient* p): _p(p) {}
                int32_t t = _p->Settings.TimeoutAudio;
            //─────────────
            // 音量
            //─────────────
            bool VOLUME(
                int dev,
                int vol
            ){return _p->RunGetOK(t, "MP3/SET","VOLUME", dev, vol);}

            //─────────────
            // イコライザ
            //─────────────
            bool EQ(
                int dev,
                int mode
            ){return _p->RunGetOK(t, "MP3/SET","EQ", dev, mode);}
        } SET;

        //─────────────
        // サブ：トラック
        //─────────────
        class sub_Track {
            MmpClient* _p;
            public:
                explicit sub_Track(MmpClient* p): _p(p) {}
                int32_t t = _p->Settings.TimeoutAudio;
                //─────────────
                // トラック再生
                //─────────────
                int PLAY(
                    int dev,
                    int dir,
                    int file
                ){return _p->RunGetVal(t, "MP3/TRACK","STOP" , dev, dir, file);}

                //─────────────
                // ループ再生有無
                //─────────────
                int LOOP(
                    int  dev,
                    bool mode
                ){return _p->RunGetVal(t, "MP3/TRACK","STOP" , dev, mode ? 1 : 0);}

                //─────────────
                // 停止,一時停止,再開
                //─────────────
                int STOP (int dev){return _p->RunGetVal(t, "MP3/TRACK","STOP" , dev);}
                int PAUSE(int dev){return _p->RunGetVal(t, "MP3/TRACK","PAUSE", dev);}
                int START(int dev){return _p->RunGetVal(t, "MP3/TRACK","START", dev);}
        } TRACK;

        //─────────────
        // サブ：インフォメーション
        //─────────────
        class sub_Info {
            MmpClient* _p;
            public:
                explicit sub_Info(MmpClient* p): _p(p) {}
                //─────────────
                // デバイス接続状況
                // トラック
                // 音量
                // イコライザ
                // 現在のファイル番号
                // 総ファイル総数
                //─────────────
                int32_t t = _p->Settings.TimeoutAudio;
                int CONNECT (int dev){return _p->RunGetVal(t, "MP3/INFO","CONNECT", dev);}
                int TRACK   (int dev){return _p->RunGetVal(t, "MP3/INFO","TRACK"  , dev);}
                int VOLUME  (int dev){return _p->RunGetVal(t, "MP3/INFO","VOLUME" , dev);}
                int EQ      (int dev){return _p->RunGetVal(t, "MP3/INFO","EQ"     , dev);}
                int FILEID  (int dev){return _p->RunGetVal(t, "MP3/INFO","FILEID" , dev);}
                int FILEIDES(int dev){return _p->RunGetVal(t, "MP3/INFO","FILES"  , dev);}
        } INFO;
    } MP3;

//━━━━━━━━━━━━━━━
// モジュール：Ｉ２Ｃ通信
//━━━━━━━━━━━━━━━
public:
    class mod_I2C {
    MmpClient* _p;
    public:
        explicit mod_I2C(MmpClient* p): _p(p) {}
        int32_t t = _p->Settings.TimeoutI2c;
        //─────────────
        // 書き込み
        //─────────────
        bool WRITE(
            int addr,
            int reg,
            int val
        ){return _p->RunGetOK(t, "I2C","WRITE", addr, reg, val);}

        //─────────────
        // 読み出し
        //─────────────
        int READ(
            int addr,
            int reg
        ){return _p->RunGetVal(t, "I2C","READ", addr, reg);}
    } I2C;


//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//【内部利用】
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
private:
    //─────────────
    // "-999!"～"9999!"判定
    //─────────────
    static inline bool TryParseDecBang5(const String& s, int& val) {

        // データ書式違反
        val = -9999;
        if (s.length() != 5 || s[4] != '!') return false;

        // データ型違反
        if (s[0]=='!' && s[1]=='!' && s[2]=='!' && s[3]=='!'){val = -9990; return false;}

        // エラー応答）
        if (s[0] == '#') {
            char a = s[1], b = s[2], c = s[3];
            if (a=='C' && b=='M' && c=='D'){val = -9901; return false;} // コマンド不明
            if (a=='C' && b=='H' && c=='K'){val = -9902; return false;} // 引数チェックエラー
            if (a=='F' && b=='L' && c=='W'){val = -9910; return false;} // データオーバーフロー
            if (a=='I' && b=='N' && c=='I'){val = -9911; return false;} // データ未初期化
            if (a=='C' && b=='H' && c=='N'){val = -9920; return false;} // チャンネル違反
            if (a=='D' && b=='E' && c=='V'){val = -9921; return false;} // デバイス違反
            val = -9900;
            return false;
        }

        // 移行に異常があれば、データ書式違反(-9999)になる

        // 数値応答
        auto isd = [](char ch){ return (unsigned)(ch - '0') <= 9; };
        char c0 = s[0], c1 = s[1], c2 = s[2], c3 = s[3];

        if (c0 == '-') {
            if (!(isd(c1) && isd(c2) && isd(c3))) return false;
            val = -((c1 - '0') * 100 + (c2 - '0') * 10 + (c3 - '0'));  // -999～0
            return true;
        } else {
            if (!(isd(c0) && isd(c1) && isd(c2) && isd(c3))) return false;
            val = (c0 - '0') * 1000 + (c1 - '0') * 100 + (c2 - '0') * 10 + (c3 - '0'); // 0～9999
            return true;
        }
    }

//━━━━━━━━━━━━━━━
// アナログ入力用ヘルパ
//━━━━━━━━━━━━━━━
private:
    //─────────────
    // Analogビット数 
    //─────────────
    static inline int _clampBits(
        int v           ,
        int bits        ,
        int defaultBits
    )
    {
        int useBits =
        (bits >= 1 && bits <= 30) ? bits :
        (defaultBits >= 1 && defaultBits <= 30 ? defaultBits : 10); // 最終セーフガードとして 10
        const int maxv = (1 << useBits) - 1;
        if (v < 0) return v;
        if (v > maxv) return maxv;
        return v;
    }
    //─────────────
    // Analog丸め 
    //─────────────
    static inline int _roundNearest(
        int raw         ,
        int step        ,
        int bits        ,
        int defaultBits
    ){
        if (raw < 0) return raw;
        if (step <= 0) return _clampBits(raw, bits, defaultBits);
        int useBits =
        (bits >= 1 && bits <= 30) ? bits :
        (defaultBits >= 1 && defaultBits <= 30 ? defaultBits : 10);
        const int maxv = (1 << useBits) - 1;
        if (raw > maxv) raw = maxv;
        int base = (raw / step) * step;
        int delta = raw - base;
        int thresholdUp = (step % 2 == 0) ? (step / 2) : (step / 2 + 1);
        int r = (delta >= thresholdUp) ? base + step : base;
        if (r > maxv) r = maxv;
        return r;
    }
    //─────────────
    // Analog丸め(切り上げ) 
    //─────────────
    static inline int _roundUp(
        int raw         ,
        int step        ,
        int bits        ,
        int defaultBits
    ){
        if (raw < 0) return raw;
        if (step <= 0) return _clampBits(raw, bits, defaultBits);
        int useBits =
        (bits >= 1 && bits <= 30) ? bits :
        (defaultBits >= 1 && defaultBits <= 30 ? defaultBits : 10);
        const int maxv = (1 << useBits) - 1;
        int r = ((raw + step - 1) / step) * step;
        if (r > maxv) r = maxv;
        return r;
    }
    //─────────────
    // Analog丸め(切り下げ) 
    //─────────────
    static inline int _roundDown(
        int raw         ,
        int step        ,
        int bits        ,
        int defaultBits
    ){
        if (raw < 0) return raw;
        if (step <= 0) return _clampBits(raw, bits, defaultBits);
        int useBits =
        (bits >= 1 && bits <= 30) ? bits :
        (defaultBits >= 1 && defaultBits <= 30 ? defaultBits : 10);
        int r = (raw / step) * step;
        if (r < 0) r = 0;
        return r;
    }
};
}} // namespace Mmp