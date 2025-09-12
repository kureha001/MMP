//============================================================
// Ａｒｄｕｉｎｏ用：ＭＭＰコア機能
//------------------------------------------------------------
// ＭＭＰシリアルコマンドを直接扱う
//------------------------------------------------------------
// [インストール方法]
// １．プロジェクトと同ディレクトリに格納
//============================================================
#pragma once
#include <Arduino.h>
#include "MmpHardware.h"

namespace Mmp { namespace Core{
class MmpClient{

//========================
// プロパティ
//========================
private:
    HardwareSerial* _uart = &MMP_UART;
    bool    _isOpen             = false;
    String  _connected_port     = "";
    int32_t _connected_baud     = 0;
    String  _lastError          = "";

public:
    //----------------
    // 接続情報
    //----------------
    bool    IsOpen()         const { return _isOpen;          }
    String  ConnectedPort()  const { return _connected_port;  }
    int32_t ConnectedBaud()  const { return _connected_baud;  }
    String  LastError()      const { return _lastError;       }

    //----------------
    // 規定値
    //----------------
    struct MmpSettings {
        String  PortName       = "";
        int32_t BaudRate       = 115200;
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


//========================
// 低レイヤ コマンド
//========================
//【内部利用】
private:
    bool   Open       (uint32_t baud,         int32_t timeoutIo);
    bool   Verify     (int32_t timeoutVerify                   );
    String SendCommand(const String& command, int32_t timeoutMs);
    inline void ClearInput() { while (_uart->available() > 0) (void)_uart->read(); }

//【外部公開】
public:
    //----------------
    // 接続：自動
    //----------------
    bool ConnectAutoBaud(
        const uint32_t* cand = MMP_BAUD_CANDIDATES      ,
        size_t n             = MMP_BAUD_CANDIDATES_LEN
    );

    //----------------
    // 接続：条件指定
    //----------------
    bool ConnectWithBaud(
        uint32_t baud               ,
        int32_t  timeoutIo       = 0,
        int32_t  verifyTimeoutMs = 0
    );
    //----------------
    // 切断
    //----------------
    void Close();


//===========================
// モジュール コマンド
//===========================
//【外部公開】階層ＡＰＩ
public:
    MmpClient(): Info(this), Analog(this), Digital(this), Pwm(this), Audio(this), I2c(this) {}


//===========================
// << 情報モジュール >>
//===========================
//【内部利用】フラットＡＰＩ
private:
    String   GetVersion(int32_t timeoutMs);
    uint16_t GetPwx    (int32_t devId0to15, int32_t timeoutMs);
    uint16_t GetDpx    (int32_t devId1to4,  int32_t timeoutMs);

//【外部公開】階層ＡＰＩ
public:
    class InfoModule {
    MmpClient* _p;
    public:
        explicit InfoModule(MmpClient* p): _p(p), Dev(p) {}

        //----------------------
        // １．バージョン
        //----------------------
        String Version(
            int32_t timeoutMs = 0
        ) { return _p->GetVersion(timeoutMs); }

        //----------------------
        // ２．デバイス情報
        //----------------------
        class DevModule {
            MmpClient* _p;
            public:
                explicit DevModule(MmpClient* p): _p(p) {}

                //----------------------
                // 2-1.ＰＷＭデバイス
                //----------------------
                uint16_t Pwm(
                    int32_t devId0to15,
                    int32_t timeoutMs = 0
                ) { return _p->GetPwx(devId0to15, timeoutMs); }

                //----------------------
                // 2-2.音声デバイス
                //----------------------
                uint16_t Audio(
                    int32_t devId1to4, 
                    int32_t timeoutMs = 0
                ) { return _p->GetDpx(devId1to4, timeoutMs); }
        } Dev;
    } Info;


//===========================
// << アナログ モジュール >>
//===========================
//【内部利用】フラットＡＰＩ
private:
    bool     AnalogConfigure(int32_t hc4067chTtl,   int32_t hc4067devTtl,  int32_t timeoutMs);
    bool     AnalogUpdate   (                                              int32_t timeoutMs);
    int32_t  AnalogRead     (int32_t hc4067ch0to15, int32_t hc4067dev0to3, int32_t timeoutMs);

//【外部公開】階層ＡＰＩ
public:
    class AnalogModule {
    MmpClient* _p;
    public:
        explicit AnalogModule(MmpClient* p): _p(p) {}

        //-----------------------------------------
        // １．使用範囲設定
        //-----------------------------------------
        bool Configure(
            int32_t hc4067chTtl     ,
            int32_t hc4067devTtl    ,
            int32_t timeoutMs = 0
        ) { return _p->AnalogConfigure(hc4067chTtl, hc4067devTtl, timeoutMs); }

        //-----------------------------------------
        // ２．測定値バッファ更新
        //-----------------------------------------
        bool Update(
            int32_t timeoutMs = 0
        ) { return _p->AnalogUpdate(timeoutMs); }

        //-----------------------------------------
        // ３．測定値バッファ読取（丸めなし）
        //-----------------------------------------
        int32_t Read(
            int32_t hc4067ch0to15   ,
            int32_t hc4067dev0to3   ,
            int32_t timeoutMs = 0
        ) { return _p->AnalogRead(hc4067ch0to15, hc4067dev0to3, timeoutMs); }

        //-----------------------------------------
        // ４．測定値バッファ読取（中央値基準の丸め）
        //-----------------------------------------
        int32_t ReadRound(
            int32_t hc4067ch0to15   ,
            int32_t hc4067dev0to3   ,
            int32_t step            ,
            int32_t bits        = 0 ,
            int32_t timeoutMs   = 0
        ) {
            int32_t raw = _p->AnalogRead(hc4067ch0to15, hc4067dev0to3, timeoutMs);
            return _roundNearest(raw, step, bits, _p->Settings.AnalogBits);
        }

        //-----------------------------------------
        // ５．測定値バッファ読取（切り上げ丸め）
        //-----------------------------------------
        int32_t ReadRoundUp(
            int32_t hc4067ch0to15   ,
            int32_t hc4067dev0to3   ,
            int32_t step            ,
            int32_t bits        = 0 ,
            int32_t timeoutMs   = 0
        ) {
            int32_t raw = _p->AnalogRead(hc4067ch0to15, hc4067dev0to3, timeoutMs);
            return _roundUp(raw, step, bits, _p->Settings.AnalogBits);
        }

        //-----------------------------------------
        // ６．測定値バッファ読取（切り捨て丸め）
        //-----------------------------------------
        int32_t ReadRoundDown(
            int32_t hc4067ch0to15   ,
            int32_t hc4067dev0to3   ,
            int32_t step            ,
            int32_t bits        = 0 ,
            int32_t timeoutMs   = 0
        ) {
            int32_t raw = _p->AnalogRead(hc4067ch0to15, hc4067dev0to3, timeoutMs);
            return _roundDown(raw, step, bits, _p->Settings.AnalogBits);
        }
    } Analog;

//===========================
// << デジタル モジュール >>
//===========================
//【内部利用】フラットＡＰＩ
private:
    int32_t  DigitalIn (int32_t gpioId,                  int32_t timeoutMs);
    bool     DigitalOut(int32_t gpioId, int32_t val0or1, int32_t timeoutMs);

//【外部公開】階層ＡＰＩ
public:
    class DigitalModule {
    MmpClient* _p;
    public:
        explicit DigitalModule(MmpClient* p): _p(p) {}

        //---------------------
        // １．入力
        //---------------------
        int32_t In(
            int32_t gpioId      ,
            int32_t timeoutMs = 0
        ) { return _p->DigitalIn(gpioId, timeoutMs); }

        //---------------------
        // ２．出力
        //---------------------
        bool Out(
            int32_t gpioId,
            int32_t val0or1,
            int32_t timeoutMs = 0
        ) { return _p->DigitalOut(gpioId, val0or1, timeoutMs); }
    } Digital;

//===========================
// << ＰＷＭモジュール >>
//===========================
//【内部利用】フラットＡＰＩ
private:
    bool PwmValue(int32_t chId0to255, int32_t val0to4095,                             int32_t timeoutMs);
    bool PwmInit (int32_t angleMin, int32_t angleMax, int32_t pwmMin, int32_t pwmMax, int32_t timeoutMs);
    bool PwmAngle(int32_t chId0to255, int32_t angle0to180,                            int32_t timeoutMs);

//【外部公開】階層ＡＰＩ
public:
    class PwmModule {
    MmpClient* _p;
    public:
        explicit PwmModule(MmpClient* p): _p(p) {}

        //---------------------
        // １．デバイス情報
        //---------------------
        uint16_t Info(
            int32_t devId0to15  ,
            int32_t timeoutMs = 0
        ) { return _p->GetPwx  (devId0to15, timeoutMs); }

        //---------------------
        // ２．出力
        //---------------------
        bool Out(
            int32_t chId0to255  ,
            int32_t val0to4095  , 
            int32_t timeoutMs = 0
        ) { return _p->PwmValue(chId0to255, val0to4095, timeoutMs); }

        //---------------------
        // ３．角度設定
        //---------------------
        bool AngleInit(
            int32_t angleMin    ,
            int32_t angleMax    ,
            int32_t pwmMin      ,
            int32_t pwmMax      ,
            int32_t timeoutMs = 0
        ) { return _p->PwmInit (angleMin, angleMax, pwmMin, pwmMax, timeoutMs); }

        //---------------------
        // ４．角度指定
        //---------------------
        bool AngleOut(
            int32_t chId0to255  ,
            int32_t angle0to180 ,
            int32_t timeoutMs = 0
        ) { return _p->PwmAngle(chId0to255, angle0to180, timeoutMs); }
    } Pwm;

//===========================
// << 音声モジュール >>
//===========================
//【内部利用】フラットＡＰＩ
private:
    //----------------------
    // １. 単独コマンド
    //----------------------
    bool     DfVolume        (int32_t devId1to4, int32_t vol0to30,                      int32_t timeoutMs);
    bool     DfSetEq         (int32_t devId1to4, int32_t mode0to5,                      int32_t timeoutMs);
    //------------------------
    // ２. 再生サブモジュール
    //------------------------
    bool     DfStart         (int32_t devId1to4, int32_t dir1to255, int32_t file1to255, int32_t timeoutMs);
    bool     DfSetLoop       (int32_t devId1to4, int32_t on0or1,                        int32_t timeoutMs);
    bool     DfStop          (int32_t devId1to4,                                        int32_t timeoutMs);
    bool     DfPause         (int32_t devId1to4,                                        int32_t timeoutMs);
    bool     DfResume        (int32_t devId1to4,                                        int32_t timeoutMs);
    //------------------------
    // ３. 参照サブモジュール
    //------------------------
    int32_t  DfReadState     (int32_t devId1to4,                                        int32_t timeoutMs);
    int32_t  DfReadVolume    (int32_t devId1to4,                                        int32_t timeoutMs);
    int32_t  DfReadEq        (int32_t devId1to4,                                        int32_t timeoutMs);
    int32_t  DfReadFileCounts(int32_t devId1to4,                                        int32_t timeoutMs);
    int32_t  DfReadFileNumber(int32_t devId1to4,                                        int32_t timeoutMs);

//【外部公開】階層ＡＰＩ
public:
    class AudioModule {
    MmpClient* _p;
    public:
        explicit AudioModule(MmpClient* p): _p(p), Play(p), Read(p) {}

        //----------------------
        // １. 単独コマンド
        //----------------------
        // 1-1. デバイス情報
        //----------------------
        uint16_t Info(
            int32_t devId1to4   ,
            int32_t timeoutMs = 0
        ) { return _p->GetDpx  (devId1to4, timeoutMs); }

        //----------------------
        // 1-2. 音量設定
        //----------------------
        bool Volume(
            int32_t devId1to4   ,
            int32_t vol0to30    ,
            int32_t timeoutMs = 0
        ) { return _p->DfVolume(devId1to4, vol0to30, timeoutMs); }

        //----------------------
        // 1-3.イコライザ設定
        //----------------------
        bool SetEq(
            int32_t devId1to4   ,
            int32_t mode0to5    ,
            int32_t timeoutMs = 0
        ) { return _p->DfSetEq (devId1to4, mode0to5, timeoutMs); }

        //------------------------
        // ２. 再生サブモジュール
        //------------------------
        class PlayModule {
            MmpClient* _p;
            public:
                explicit PlayModule(MmpClient* p): _p(p) {}

                //----------------------
                // 2-1. 再生（Start）
                //----------------------
                bool Start(
                    int32_t devId1to4   ,
                    int32_t dir1to255   ,
                    int32_t file1to255  ,
                    int32_t timeoutMs = 0
                ) { return _p->DfStart  (devId1to4, dir1to255, file1to255, timeoutMs); }

                //----------------------
                // 2-2. ループ再生指定
                //----------------------
                bool SetLoop(
                    int32_t devId1to4   ,
                    bool enable         ,
                    int32_t timeoutMs = 0
                ) { return _p->DfSetLoop(devId1to4, enable ? 1 : 0, timeoutMs); }

                //----------------------
                // 2-3. 停止
                //----------------------
                bool Stop(
                    int32_t devId1to4,
                    int32_t timeoutMs = 0
                ) { return _p->DfStop   (devId1to4, timeoutMs); }

                //----------------------
                // 2-4. 一時停止
                //----------------------
                bool Pause(
                    int32_t devId1to4,
                    int32_t timeoutMs = 0
                ) { return _p->DfPause  (devId1to4, timeoutMs); }

                //----------------------
                // 2-5. 再開
                //----------------------
                bool Resume(
                    int32_t devId1to4,
                    int32_t timeoutMs = 0
                ) { return _p->DfResume (devId1to4, timeoutMs); }
        } Play;

        //------------------------
        // ３. 参照サブモジュール
        //------------------------
        class ReadModule {
            MmpClient* _p;
            public:
                explicit ReadModule(MmpClient* p): _p(p) {}

                //----------------------
                // 3-1. 再生状況
                //----------------------
                int32_t State(
                    int32_t devId1to4   ,
                    int32_t timeoutMs = 0
                ) { return _p->DfReadState(devId1to4, timeoutMs ); }

                //----------------------
                // 3-2. ボリューム
                //----------------------
                int32_t Volume(
                    int32_t devId1to4   ,
                    int32_t timeoutMs = 0
                ) { return _p->DfReadVolume(devId1to4, timeoutMs ); }

                //----------------------
                // 3-3. イコライザのモード
                //----------------------
                int32_t Eq(
                    int32_t devId1to4   ,
                    int32_t timeoutMs = 0
                ) { return _p->DfReadEq(devId1to4, timeoutMs ); }

                //----------------------
                // 3-4. 総ファイル総数
                //----------------------
                int32_t FileCounts(
                    int32_t devId1to4   ,
                    int32_t timeoutMs = 0
                ) { return _p->DfReadFileCounts(devId1to4, timeoutMs ); }

                //----------------------
                // 3-5. 現在のファイル番号
                //----------------------
                int32_t FileNumber(
                    int32_t devId1to4   ,
                    int32_t timeoutMs = 0
                ) { return _p->DfReadFileNumber(devId1to4, timeoutMs ); }
        } Read;
    } Audio;


//===========================
// << Ｉ２Ｃモジュール >>
//===========================
//【内部利用】フラットＡＰＩ
private:
    bool     I2cWrite(int32_t addr, int32_t reg, int32_t val, int32_t timeoutMs);
    int32_t  I2cRead (int32_t addr, int32_t reg,              int32_t timeoutMs);

//【外部公開】階層ＡＰＩ
public:
    class I2cModule {
    MmpClient* _p;
    public:
        explicit I2cModule(MmpClient* p): _p(p) {}

        //----------------------
        // １．書き込み
        //----------------------
        bool Write(
            int32_t addr        ,
            int32_t reg         ,
            int32_t val         ,
            int32_t timeoutMs = 0
        ) { return _p->I2cWrite(addr, reg, val, timeoutMs); }

        //----------------------
        // ２．読み出し
        //----------------------
        int32_t Read(
            int32_t addr        ,
            int32_t reg         ,
            int32_t timeoutMs = 0
        ) { return _p->I2cRead (addr, reg, timeoutMs); }
    } I2c;



//======================================================
//【内部利用】
//======================================================

//=============================
// 送受信データ用ヘルパ
//=============================
private:
    //---------------------------
    // 
    //---------------------------
    static inline int32_t Resolve(int32_t v, int32_t fb) { return (v > 0) ? v : fb; }

    //---------------------------
    // 
    //---------------------------
    static inline int     HexDigit(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        return -1;
    }

    //---------------------------
    // "!!!!!"判定
    //---------------------------
    static inline bool TryParseHex4Bang(const String& s, uint16_t& value) {
        if (s.length() != 5 || s[4] != '!') return false;
        uint16_t v = 0;
        for (int i = 0; i < 4; ++i) {
        int d = HexDigit(s[i]);
        if (d < 0) return false;
        v = (uint16_t)((v << 4) | (uint16_t)d);
        }
        value = v; return true;
    }
    //---------------------------
    // HEX値変換(個別桁)
    //---------------------------
    static inline String Hex2(int v) { char b[2+1]; snprintf(b, sizeof(b), "%02X", (unsigned)(v & 0xFF));   return String(b); }
    static inline String Hex3(int v) { char b[3+1]; snprintf(b, sizeof(b), "%03X", (unsigned)(v & 0x3FF));  return String(b); }
    static inline String Hex4(int v) { char b[4+1]; snprintf(b, sizeof(b), "%04X", (unsigned)(v & 0xFFFF)); return String(b); }


//=============================
// アナログ入力用ヘルパ
//=============================
private:
    //--------------------
    // Analogビット数 
    //--------------------
    static inline int32_t _clampBits(
        int32_t v           ,
        int32_t bits        ,
        int32_t defaultBits
    )
    {
        int32_t useBits =
        (bits >= 1 && bits <= 30) ? bits :
        (defaultBits >= 1 && defaultBits <= 30 ? defaultBits : 10); // 最終セーフガードとして 10
        const int32_t maxv = (1 << useBits) - 1;
        if (v < 0) return v;
        if (v > maxv) return maxv;
        return v;
    }

    //--------------------
    // Analog丸め 
    //--------------------
    static inline int32_t _roundNearest(
        int32_t raw         ,
        int32_t step        ,
        int32_t bits        ,
        int32_t defaultBits
    )
    {
        if (raw < 0) return raw;
        if (step <= 0) return _clampBits(raw, bits, defaultBits);
        int32_t useBits =
        (bits >= 1 && bits <= 30) ? bits :
        (defaultBits >= 1 && defaultBits <= 30 ? defaultBits : 10);
        const int32_t maxv = (1 << useBits) - 1;
        if (raw > maxv) raw = maxv;
        int32_t base = (raw / step) * step;
        int32_t delta = raw - base;
        int32_t thresholdUp = (step % 2 == 0) ? (step / 2) : (step / 2 + 1);
        int32_t r = (delta >= thresholdUp) ? base + step : base;
        if (r > maxv) r = maxv;
        return r;
    }

    //--------------------
    // Analog丸め(切り上げ) 
    //--------------------
    static inline int32_t _roundUp(
        int32_t raw         ,
        int32_t step        ,
        int32_t bits        ,
        int32_t defaultBits
    )
    {
        if (raw < 0) return raw;
        if (step <= 0) return _clampBits(raw, bits, defaultBits);
        int32_t useBits =
        (bits >= 1 && bits <= 30) ? bits :
        (defaultBits >= 1 && defaultBits <= 30 ? defaultBits : 10);
        const int32_t maxv = (1 << useBits) - 1;
        int32_t r = ((raw + step - 1) / step) * step;
        if (r > maxv) r = maxv;
        return r;
    }

    //--------------------
    // Analog丸め(切り下げ) 
    //--------------------
    static inline int32_t _roundDown(
        int32_t raw         ,
        int32_t step        ,
        int32_t bits        ,
        int32_t defaultBits
    )
    {
        if (raw < 0) return raw;
        if (step <= 0) return _clampBits(raw, bits, defaultBits);
        int32_t useBits =
        (bits >= 1 && bits <= 30) ? bits :
        (defaultBits >= 1 && defaultBits <= 30 ? defaultBits : 10);
        int32_t r = (raw / step) * step;
        if (r < 0) r = 0;
        return r;
    }
};
}} // namespace
