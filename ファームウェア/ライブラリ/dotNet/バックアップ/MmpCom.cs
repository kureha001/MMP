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

        bool IsOpen { get; }
        string LastError { get; }

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

        public bool IsOpen { get { return _cli.IsOpen; } }
        public string LastError { get { return _lastError; } }

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

        //================ 接続 =====================
        public string Connect(string portName, int baud, int timeoutMs, int verifyTimeoutMs)
        {
            return Guard<string>(delegate ()
            {
                // ★ 引数を Settings に書き込まず、そのまま MmpClient.Connect に渡す
                //    0 指定は MmpClient 側で Settings へフォールバック（既存仕様）
                bool ok = _cli.Connect(string.IsNullOrEmpty(portName) ? null : portName,
                                       baud, timeoutMs, verifyTimeoutMs); // ★
                return ok ? (_cli.PortName ?? "") : "";
            }, "");
        }

        public void Close()
        {
            Guard(delegate () { _cli.Close(); });
        }

        //================ 階層 API プロキシの提供 ======================
        private InfoCom     _info;
        private SettingsCom _settings;
        private AnalogCom   _analog;
        private DigitalCom  _digital;
        private PwmCom      _pwm;
        private AudioCom    _audio;
        private I2cCom      _i2c;

        public object Info     { get { if (_info     == null) _info     = new InfoCom    (this); return _info;      } }
        public object Settings { get { if (_settings == null) _settings = new SettingsCom(this); return _settings;  } }
        public object Analog   { get { if (_analog   == null) _analog   = new AnalogCom  (this); return _analog;    } }
        public object Digital  { get { if (_digital  == null) _digital  = new DigitalCom (this); return _digital;   } }
        public object Pwm      { get { if (_pwm      == null) _pwm      = new PwmCom     (this); return _pwm;       } }
        public object Audio    { get { if (_audio    == null) _audio    = new AudioCom   (this); return _audio;     } }
        public object I2c      { get { if (_i2c      == null) _i2c      = new I2cCom     (this); return _i2c;       } }

        //============================================================
        // 階層 API: Info
        //============================================================
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

                public int Pwm(int deviceId, int timeoutMs = 0)
                {
                    return _owner.Guard<int>(delegate ()
                    {
                        return _owner._cli.Info.Dev.Pwm(deviceId, timeoutMs);
                    }, -1);
                }

                public int Audio(int id1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<int>(delegate ()
                    {
                        return _owner._cli.Info.Dev.Audio(id1to4, timeoutMs);
                    }, -1);
                }
            }
        }

        //============================================================
        // 階層 API: Settings
        //============================================================
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
        }

        //============================================================
        // 階層 API: Analog
        //============================================================
        [ComVisible(true)]
        [Guid("A6A9B8A1-9ED1-4F6B-90C9-6F5E2BB5E0C1")]
        [ClassInterface(ClassInterfaceType.AutoDual)]
        public class AnalogCom
        {
            private readonly MmpCom _owner;
            public AnalogCom(MmpCom owner) { _owner = owner; }

            public bool Configure(int players, int switches, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Analog.Configure(players, switches, timeoutMs);
                }, false);
            }

            public bool Update(int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Analog.Update(timeoutMs);
                }, false);
            }

            public int Read(int playerIndex0to15, int switchIndex0to3, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Analog.Read(playerIndex0to15, switchIndex0to3, timeoutMs);
                }, -1);
            }
        }

        //============================================================
        // 階層 API: Digital
        //============================================================
        [ComVisible(true)]
        [Guid("B9C3C0A8-AB21-4E9D-9C3B-0B5D99B5E1D3")]
        [ClassInterface(ClassInterfaceType.AutoDual)]
        public class DigitalCom
        {
            private readonly MmpCom _owner;
            public DigitalCom(MmpCom owner) { _owner = owner; }

            public int In(int portId, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Digital.In(portId, timeoutMs);
                }, 0);
            }

            public bool Out(int portId, int value0or1, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Digital.Out(portId, value0or1, timeoutMs);
                }, false);
            }

            public int Io(int portId, int value0or1, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Digital.Io(portId, value0or1, timeoutMs);
                }, 0);
            }
        }

        //============================================================
        // 階層 API: Pwm
        //============================================================
        [ComVisible(true)]
        [Guid("7C7C6D8E-4B02-4C5D-8C6E-3A1B3FB2A0A2")]
        [ClassInterface(ClassInterfaceType.AutoDual)]
        public class PwmCom
        {
            private readonly MmpCom _owner;
            public PwmCom(MmpCom owner) { _owner = owner; }

            public int Info(int deviceId, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Pwm.Info(deviceId, timeoutMs);
                }, -1);
            }

            public string Out(int channel, int value0to4095, int timeoutMs = 0)
            {
                return _owner.Guard<string>(delegate ()
                {
                    return _owner._cli.Pwm.Out(channel, value0to4095, timeoutMs);
                }, "");
            }

            public bool AngleInit(int angleMin, int angleMax, int pwmMin, int pwmMax, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Pwm.AngleInit(angleMin, angleMax, pwmMin, pwmMax, timeoutMs);
                }, false);
            }

            public string AngleOut(int channel, int angle0to180, int timeoutMs = 0)
            {
                return _owner.Guard<string>(delegate ()
                {
                    return _owner._cli.Pwm.AngleOut(channel, angle0to180, timeoutMs);
                }, "");
            }
        }

        //============================================================
        // 階層 API: Audio
        //============================================================
        [ComVisible(true)]
        [Guid("C4A2B8E7-1E43-4E52-9D5B-8A3A5E1C4F77")]
        [ClassInterface(ClassInterfaceType.AutoDual)]
        public class AudioCom
        {
            private readonly MmpCom _owner;
            private readonly PlayCom _play;
            public AudioCom(MmpCom owner) { _owner = owner; _play = new PlayCom(owner); }

            public int Info(int id1to4, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Audio.Info(id1to4, timeoutMs);
                }, -1);
            }

            public bool Volume(int deviceId1to4, int vol0to30, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Audio.Volume(deviceId1to4, vol0to30, timeoutMs);
                }, false);
            }

            public bool SetEq(int deviceId1to4, int eq0to5, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Audio.SetEq(deviceId1to4, eq0to5, timeoutMs);
                }, false);
            }

            public int ReadPlayState(int deviceId1to4, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Audio.ReadPlayState(deviceId1to4, timeoutMs);
                }, -1);
            }

            public int ReadVolume(int deviceId1to4, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Audio.ReadVolume(deviceId1to4, timeoutMs);
                }, -1);
            }

            public int ReadEq(int deviceId1to4, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Audio.ReadEq(deviceId1to4, timeoutMs);
                }, -1);
            }

            public int ReadFileCounts(int deviceId1to4, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Audio.ReadFileCounts(deviceId1to4, timeoutMs);
                }, -1);
            }

            public int ReadCurrentFileNumber(int deviceId1to4, int timeoutMs = 0)
            {
                return _owner.Guard<int>(delegate ()
                {
                    return _owner._cli.Audio.ReadCurrentFileNumber(deviceId1to4, timeoutMs);
                }, -1);
            }

            public bool Stop(int deviceId1to4, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Audio.Stop(deviceId1to4, timeoutMs);
                }, false);
            }

            public bool Pause(int deviceId1to4, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Audio.Pause(deviceId1to4, timeoutMs);
                }, false);
            }

            public bool Resume(int deviceId1to4, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.Audio.Resume(deviceId1to4, timeoutMs);
                }, false);
            }

            public object Play { get { return _play; } }

            //============================================================
            // 階層 API: Audio.Play
            //============================================================
            [ComVisible(true)]
            [Guid("E9C5D1A3-24B5-4B5E-879C-3B4D5E6F7A88")]
            [ClassInterface(ClassInterfaceType.AutoDual)]
            public class PlayCom
            {
                private readonly MmpCom _owner;
                public PlayCom(MmpCom owner) { _owner = owner; }

                public string FolderTrack(int deviceId1to4, int folder1to255, int track1to255, int timeoutMs = 0)
                {
                    return _owner.Guard<string>(delegate ()
                    {
                        return _owner._cli.Audio.Play.FolderTrack(deviceId1to4, folder1to255, track1to255, timeoutMs);
                    }, "");
                }

                public bool Stop(int deviceId1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<bool>(delegate ()
                    {
                        return _owner._cli.Audio.Play.Stop(deviceId1to4, timeoutMs);
                    }, false);
                }

                public bool Pause(int deviceId1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<bool>(delegate ()
                    {
                        return _owner._cli.Audio.Play.Pause(deviceId1to4, timeoutMs);
                    }, false);
                }

                public bool Resume(int deviceId1to4, int timeoutMs = 0)
                {
                    return _owner.Guard<bool>(delegate ()
                    {
                        return _owner._cli.Audio.Play.Resume(deviceId1to4, timeoutMs);
                    }, false);
                }
            }
        }

        //============================================================
        // 階層 API: I2c
        //============================================================
        [ComVisible(true)]
        [Guid("D1E4F3B2-2A1C-4E9B-8F0A-AB2E7D0155B1")]
        [ClassInterface(ClassInterfaceType.AutoDual)]
        public class I2cCom
        {
            private readonly MmpCom _owner;
            public I2cCom(MmpCom owner) { _owner = owner; }

            public bool Write(int addr, int reg, int value, int timeoutMs = 0)
            {
                return _owner.Guard<bool>(delegate ()
                {
                    return _owner._cli.I2c.Write(addr, reg, value, timeoutMs);
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
