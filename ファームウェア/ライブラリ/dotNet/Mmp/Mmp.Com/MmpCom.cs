using System;
using System.Runtime.InteropServices;

namespace Mmp.Com
{
    //============================================================
    // COM インターフェース
    //============================================================
    [ComVisible(true)]
    [Guid("2A2C2E2F-2F23-4F9E-8B2C-6A3C8B0A6E01")]
    [InterfaceType(ComInterfaceType.InterfaceIsDual)]
    public interface IMmpCom
    {
        // 接続：portName が空/Null → 自動接続、指定文字列 → 指定接続
        // 成功時: 実際のポート名（例 "COM85"）／失敗: "" を返す
        string Connect(string portName, int baud, int timeoutMs, int verifyTimeoutMs);
        void Close();

        bool   IsOpen   { get; }
        string LastError{ get; }

        // 階層APIエントリ
        object Info     { get; }
        object Settings { get; }
        object Analog   { get; }
        object Digital  { get; }
        object Pwm      { get; }
        object Audio    { get; }
        object I2c      { get; }
    }

    //============================================================
    // COM クラス本体
    //  - ProgId: "Mmp.Com"（VBA: CreateObject("Mmp.Com")）
    //============================================================
    [ComVisible(true)]
    [Guid("B6B4A62F-B1E3-4E58-A6E3-4E8B1E4C9F12")]
    [ProgId("Mmp.Com")]
    [ClassInterface(ClassInterfaceType.None)]
    public class MmpCom : IMmpCom
    {
        internal readonly Mmp.Core.MmpClient _cli = new Mmp.Core.MmpClient();
        private string _lastError = "";

        public bool IsOpen       { get { return _cli.IsOpen; } }
        public string LastError  { get { return _lastError;  } }

        // 例外を LastError へ吸収するヘルパ
        private T Guard<T>(Func<T> f, T fallback)
        {
            try { _lastError = ""; return f(); }
            catch (Exception ex) { _lastError = ex.Message; return fallback; }
        }
        private void Guard(Action a)
        {
            try { _lastError = ""; a(); }
            catch (Exception ex) { _lastError = ex.Message; }
        }

        //=============================
        //===== 低レイヤ コマンド =====
        //=============================
        // -----
        // 接続
        // -----
        public string Connect(string portName, int baud, int timeoutMs, int verifyTimeoutMs)
        {
            return Guard<string>(delegate ()
            {
                // ★ 引数を Settings に書き込まず、そのまま MmpClient.Connect に渡す
                //    0 指定は MmpClient 側で Settings へフォールバック（既存仕様）
                bool ok = _cli.Connect(string.IsNullOrEmpty(portName) ? null : portName,
                                       baud, timeoutMs, verifyTimeoutMs);
                return ok ? (_cli.PortName ?? "") : "";
            }, "");
        }

        // -----
        // 切断
        // -----
        public void Close()
        {
            Guard(delegate () { _cli.Close(); });
        }

        // ========================
        // ==== モジュール提供 ====
        // ========================
        private InfoCom     _info;
        private SettingsCom _settings;
        private AnalogCom   _analog;
        private DigitalCom  _digital;
        private PwmCom      _pwm;
        private AudioCom    _audio;
        private I2cCom      _i2c;

        public object Info    { get{ if (_info     == null) _info     = new InfoCom(this)    ; return _info;     } }
        public object Settings{ get{ if (_settings == null) _settings = new SettingsCom(this); return _settings; } }
        public object Analog  { get{ if (_analog   == null) _analog   = new AnalogCom(this)  ; return _analog;   } }
        public object Digital { get{ if (_digital  == null) _digital  = new DigitalCom(this) ; return _digital;  } }
        public object Pwm     { get{ if (_pwm      == null) _pwm      = new PwmCom(this)     ; return _pwm;      } }
        public object Audio   { get{ if (_audio    == null) _audio    = new AudioCom(this)   ; return _audio;    } }
        public object I2c     { get{ if (_i2c      == null) _i2c      = new I2cCom(this)     ; return _i2c;      } }

        //===================
        //=====階層化API=====
        //===================
        // --------------------------
        // ---- 規定値モジュール ----
        // --------------------------
        [ComVisible(true)]
        [Guid("0B5A6DFF-7F21-49E2-9C2B-8A7C9E1D2B33")]
        [ClassInterface(ClassInterfaceType.AutoDual)]
        public class SettingsCom
        {
            private readonly MmpCom _owner;
            public SettingsCom(MmpCom owner) { _owner = owner; }

            // 主要設定（VBA から読み書き）
            public int BaudRate { get { return _owner._cli.Settings.BaudRate; } set { _owner._cli.Settings.BaudRate = value; } }
            public int TimeoutIo { get { return _owner._cli.Settings.TimeoutIo; } set { _owner._cli.Settings.TimeoutIo = value; } }
            public int TimeoutVerify { get { return _owner._cli.Settings.TimeoutVerify; } set { _owner._cli.Settings.TimeoutVerify = value; } }
            public int TimeoutGeneral { get { return _owner._cli.Settings.TimeoutGeneral; } set { _owner._cli.Settings.TimeoutGeneral = value; } }
            public int TimeoutAnalog { get { return _owner._cli.Settings.TimeoutAnalog; } set { _owner._cli.Settings.TimeoutAnalog = value; } }
            public int TimeoutPwm { get { return _owner._cli.Settings.TimeoutPwm; } set { _owner._cli.Settings.TimeoutPwm = value; } }
            public int TimeoutAudio { get { return _owner._cli.Settings.TimeoutAudio; } set { _owner._cli.Settings.TimeoutAudio = value; } }
            public int TimeoutDigital { get { return _owner._cli.Settings.TimeoutDigital; } set { _owner._cli.Settings.TimeoutDigital = value; } }
            public int TimeoutI2c { get { return _owner._cli.Settings.TimeoutI2c; } set { _owner._cli.Settings.TimeoutI2c = value; } }
            public int AnalogBits { get { return _owner._cli.Settings.AnalogBits; } set { _owner._cli.Settings.AnalogBits = value; } }

            // PreferredPortsをSafeArray(BSTR)として公開
            // ※使用例：m.Settings.PreferredPorts = Array("COM3","COM8")
            public string[] PreferredPortsArray
            {
                get { return _owner._cli.Settings.PreferredPorts; }
                set { _owner._cli.Settings.PreferredPorts = value; }
            }

            // VBAで扱いやすいCSV版
            // ※使用例：m.Settings.PreferredPortsCsv = "COM3,COM8"
            public string PreferredPortsCsv
            {
                get
                {
                    var a = _owner._cli.Settings.PreferredPorts;
                    return (a == null || a.Length == 0) ? "" : string.Join(",", a);
                }
                set
                {
                    _owner._cli.Settings.PreferredPorts =
                        string.IsNullOrWhiteSpace(value)
                        ? null
                        : Array.ConvertAll(value.Split(','), s => s.Trim());
                }
            }
        }

        // -----------------------
        // --- 情報モジュール ----
        // -----------------------
        [ComVisible(true)]
        [Guid("E5F1F8B1-7E10-4D0F-9E8B-2F3E4E0F1D01")]
        [ClassInterface(ClassInterfaceType.AutoDual)]
        public class InfoCom
        {
            private readonly MmpCom _owner;
            private readonly DevCom _dev;
            public InfoCom(MmpCom owner) { _owner = owner; _dev = new DevCom(owner); }

            public string Version(int timeoutMs = 0)
            {
                return _owner.Guard<string>(delegate ()
                {
                    // 0 → 既定使用（MmpClient 内で解決）
                    return _owner._cli.Info.Version(timeoutMs);
                }, "");
            }

            public object Dev { get { return _dev; } }

            [ComVisible(true)]
            [Guid("4B2BE33C-1C4F-4A8D-87A9-6A2E1F3F2B10")]
            [ClassInterface(ClassInterfaceType.AutoDual)]
            public class DevCom
            {
                private readonly MmpCom _owner;
                public DevCom(MmpCom owner) { _owner = owner; }

                public int Pwm(int devId0to15, int timeoutMs = 0)
                {
                    return _owner.Guard<int>(delegate ()
                    {
                        return _owner._cli.Info.Dev.Pwm(devId0to15, timeoutMs);
                    }, -1);
                }

                public int Audio(int devId1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<int>(delegate ()
                    {
                        return _owner._cli.Info.Dev.Audio(devId1to4, timeoutMs);
                    }, -1);
                }
            }
        }

        // -----------------------------
        // ---- アナログ モジュール ----
        // -----------------------------
        [ComVisible(true)]
        [Guid("A6A9B8A1-9ED1-4F6B-90C9-6F5E2BB5E0C1")]
        [ClassInterface(ClassInterfaceType.AutoDual)]
        public class AnalogCom
        {
            private readonly MmpCom _owner;
            public AnalogCom(MmpCom owner) { _owner = owner; }

            public bool Configure(int hc4067chTtl, int hc4067devTtl, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Analog.Configure(hc4067chTtl, hc4067devTtl, timeoutMs);
                }, false);
            }

            public bool Update(int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Analog.Update(timeoutMs);
                }, false);
            }

            // 1) 丸めなし
            public int Read(int hc4067ch0to15, int hc4067dev0to3, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Analog.Read(hc4067ch0to15, hc4067dev0to3, timeoutMs);
                }, -1);
            }

            // 2) 丸めあり：中央値基準の最近傍（偶数stepは half-up）
            public int ReadRound(int hc4067ch0to15, int hc4067dev0to3, int step, int bits = 0, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Analog.ReadRound(hc4067ch0to15, hc4067dev0to3, step, bits, timeoutMs);
                }, -1);
            }

            // 3) 丸めあり：切り上げ
            public int ReadRoundUp(int hc4067ch0to15, int hc4067dev0to3, int step, int bits = 0, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Analog.ReadRoundUp(hc4067ch0to15, hc4067dev0to3, step, bits, timeoutMs);
                }, -1);
            }

            // 4) 丸めあり：切り下げ
            public int ReadRoundDown(int hc4067ch0to15, int hc4067dev0to3, int step, int bits = 0, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Analog.ReadRoundDown(hc4067ch0to15, hc4067dev0to3, step, bits, timeoutMs);
                }, -1);
            }
        }

        // -----------------------------
        // ---- デジタル モジュール ----
        // -----------------------------
        [ComVisible(true)]
        [Guid("B9C3C0A8-AB21-4E9D-9C3B-0B5D99B5E1D3")]
        [ClassInterface(ClassInterfaceType.AutoDual)]
        public class DigitalCom
        {
            private readonly MmpCom _owner;
            public DigitalCom(MmpCom owner) { _owner = owner; }

            public int In(int gpioId, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Digital.In(gpioId, timeoutMs);
                }, 0);
            }

            public bool Out(int gpioId, int val0or1, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Digital.Out(gpioId, val0or1, timeoutMs);
                }, false);
            }
        }

        // --------------------------
        // ---- ＰＷＭモジュール ----
        // --------------------------
        [ComVisible(true)]
        [Guid("7C7C6D8E-4B02-4C5D-8C6E-3A1B3FB2A0A2")]
        [ClassInterface(ClassInterfaceType.AutoDual)]
        public class PwmCom
        {
            private readonly MmpCom _owner;
            public PwmCom(MmpCom owner) { _owner = owner; }

            public int Info(int devId0to15, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Pwm.Info(devId0to15, timeoutMs);
                }, -1);
            }

            public bool Out(int chId0to255, int val0to4095, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Pwm.Out(chId0to255, val0to4095, timeoutMs);
                }, false);
            }

            public bool AngleInit(int angleMin, int angleMax, int pwmMin, int pwmMax, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Pwm.AngleInit(angleMin, angleMax, pwmMin, pwmMax, timeoutMs);
                }, false);
            }

            public bool AngleOut(int chId0to255, int angle0to180, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Pwm.AngleOut(chId0to255, angle0to180, timeoutMs);
                }, false);
            }
        }

        // ------------------------
        // ---- 音声モジュール ----
        // ------------------------
        [ComVisible(true)]
        [Guid("C4A2B8E7-1E43-4E52-9D5B-8A3A5E1C4F77")]
        [ClassInterface(ClassInterfaceType.AutoDual)]
        public class AudioCom
        {
            private readonly MmpCom _owner;
            private readonly PlayCom _play;
            private readonly ReadCom _read;
            public AudioCom(MmpCom owner) { _owner = owner; _play = new PlayCom(owner); _read = new ReadCom(owner); }

            // ----------------------
            // ---- 単独コマンド ----
            // ----------------------
            public int Info(int devId1to4, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Audio.Info(devId1to4, timeoutMs);
                }, -1);
            }

            public bool Volume(int devId1to4, int vol0to30, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Audio.Volume(devId1to4, vol0to30, timeoutMs);
                }, false);
            }

            public bool SetEq(int devId1to4, int mode0to5, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Audio.SetEq(devId1to4, mode0to5, timeoutMs);
                }, false);
            }

            // ------------------------
            // ---- サブモジュール ----
            // ------------------------
            public object Play { get { return _play; } }
            public object Read { get { return _read; } }

            // ----------------------
            // ---- サブ：再生系 ----
            // ----------------------
            [ComVisible(true)]
            [Guid("E9C5D1A3-24B5-4B5E-879C-3B4D5E6F7A88")]
            [ClassInterface(ClassInterfaceType.AutoDual)]
            public class PlayCom
            {
                private readonly MmpCom _owner;
                public PlayCom(MmpCom owner) { _owner = owner; }

                public bool Start(int devId1to4, int dir1to255, int file1to255, int timeoutMs = 0)
                {
                    return _owner.Guard<bool>(delegate ()
                    {
                        return _owner._cli.Audio.Play.Start(devId1to4, dir1to255, file1to255, timeoutMs);
                    }, false);
                }
                public bool SetLoop(int devId1to4, int on0or1, int timeoutMs = 0)
                {
                    return _owner.Guard<bool>(delegate ()
                    {
                        return _owner._cli.Audio.Play.SetLoop(devId1to4, on0or1 != 0, timeoutMs);
                    }, false);
                }

                public bool Stop(int devId1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<bool>(delegate ()
                    {
                        return _owner._cli.Audio.Play.Stop(devId1to4, timeoutMs);
                    }, false);
                }

                public bool Pause(int devId1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<bool>(delegate ()
                    {
                        return _owner._cli.Audio.Play.Pause(devId1to4, timeoutMs);
                    }, false);
                }

                public bool Resume(int devId1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<bool>(delegate ()
                    {
                        return _owner._cli.Audio.Play.Resume(devId1to4, timeoutMs);
                    }, false);
                }
            }

            // ----------------------
            // ---- サブ：参照系 ----
            // ----------------------
            [ComVisible(true)]
            [Guid("A1B5C6D7-89E0-4F12-A345-6789ABCDEF12")]
            [ClassInterface(ClassInterfaceType.AutoDual)]
            public class ReadCom
            {
                private readonly MmpCom _owner;
                public ReadCom(MmpCom owner) { _owner = owner; }
                public int State(int devId1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<int>(delegate ()
                    {
                        return _owner._cli.Audio.Read.State(devId1to4, timeoutMs);
                    }, -1);
                }

                public int Volume(int devId1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<int>(delegate ()
                    {
                        return _owner._cli.Audio.Read.Volume(devId1to4, timeoutMs);
                    }, -1);
                }

                public int Eq(int devId1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<int>(delegate ()
                    {
                        return _owner._cli.Audio.Read.Eq(devId1to4, timeoutMs);
                    }, -1);
                }

                public int FileCounts(int devId1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<int>(delegate ()
                    {
                        return _owner._cli.Audio.Read.FileCounts(devId1to4, timeoutMs);
                    }, -1);
                }

                public int FileNumber(int devId1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<int>(delegate ()
                    {
                        return _owner._cli.Audio.Read.FileNumber(devId1to4, timeoutMs);
                    }, -1);
                }
            }
        }

        // --------------------------
        // ---- Ｉ２Ｃモジュール ----
        // --------------------------
        [ComVisible(true)]
        [Guid("D1E4F3B2-2A1C-4E9B-8F0A-AB2E7D0155B1")]
        [ClassInterface(ClassInterfaceType.AutoDual)]
        public class I2cCom
        {
            private readonly MmpCom _owner;
            public I2cCom(MmpCom owner) { _owner = owner; }

            public bool Write(int addr, int reg, int val, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.I2c.Write(addr, reg, val, timeoutMs);
                }, false);
            }

            public int Read(int addr, int reg, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.I2c.Read(addr, reg, timeoutMs);
                }, 0);
            }
        }
    }
}
