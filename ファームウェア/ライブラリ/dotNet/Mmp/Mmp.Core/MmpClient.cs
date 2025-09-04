using System;
using System.Diagnostics;
using System.IO.Ports;
using System.Text;
using System.Threading;

namespace Mmp.Core
{
    /// <summary>
    /// MMP クライアント（シリアル経由）
    /// ・プロトコル：固定5文字（HEX4 + '!' または "!!!!!" など）、末尾 '!' 終端
    /// ・エンコーディング：ASCII
    /// </summary>
    public sealed class MmpClient : IDisposable
    {
        private SerialPort _port;

        //============================================================
        // 設定
        //============================================================
        /// <summary>設定（既定値の集約）。数値は >0 指定で優先、<=0 は既定にフォールバック。</summary>
        public sealed class MmpSettings
        {
            /// <summary>既定ボーレート</summary>
            public int BaudRate = 115200;

            /// <summary>Open() 時の Read/Write タイムアウト（ミリ秒）</summary>
            public int TimeoutIo = 200;

            /// <summary>接続確認（VER 応答待ち）のタイムアウト（ミリ秒）</summary>
            public int TimeoutVerify = 400;

            /// <summary>一般情報系（VER など）の既定タイムアウト</summary>
            public int TimeoutGeneral = 400;

            /// <summary>アナログ系の既定タイムアウト</summary>
            public int TimeoutAnalog = 400;

            /// <summary>PWM 系の既定タイムアウト</summary>
            public int TimeoutPwm = 400;

            /// <summary>オーディオ（DFPlayer）系の既定タイムアウト</summary>
            public int TimeoutAudio = 400;

            /// <summary>デジタル I/O の既定タイムアウト</summary>
            public int TimeoutDigital = 400;

            /// <summary>I2C 系の既定タイムアウト</summary>
            public int TimeoutI2c = 400;

            /// <summary>自動接続時の優先ポート順（null/空なら OS 取得順）</summary>
            public string[] PreferredPorts = null;
        }

        /// <summary>設定オブジェクト。必要に応じてプロパティを書き換えてください。</summary>
        public MmpSettings Settings { get; } = new MmpSettings();

        /// <summary>ポートが開いているか</summary>
        public bool IsOpen { get { return _port != null && _port.IsOpen; } }

        /// <summary>接続済みか（IsOpen の別名）</summary>
        public bool IsConnected { get { return IsOpen; } }

        /// <summary>現在接続中のポート名（未接続なら null）</summary>
        public string PortName { get { return IsOpen ? _port.PortName : null; } }

        //============================================================
        // 階層APIモジュール
        //============================================================
        public InfoModule Info { get; private set; }
        public AnalogModule Analog { get; private set; }
        public PwmModule Pwm { get; private set; }
        public AudioModule Audio { get; private set; }
        public DigitalModule Digital { get; private set; }  // ★ 追加
        public I2cModule I2c { get; private set; }  // ★ 追加

        public MmpClient()
        {
            // C# 7.3 互換で初期化
            Info = new InfoModule(this);
            Analog = new AnalogModule(this);
            Pwm = new PwmModule(this);
            Audio = new AudioModule(this);
            Digital = new DigitalModule(this); // ★ 追加
            I2c = new I2cModule(this);     // ★ 追加
        }

        //============================================================
        // Connect（統一版）: 明示引数(>0)が最優先、0/未指定は Settings を使用
        //============================================================
        private int Resolve(int value, int fallback) { return (value > 0) ? value : fallback; }

        /// <summary>統一エントリ。portName=null/空で自動接続、指定で固定ポート接続。</summary>
        public bool Connect(string portName, int baud, int timeoutMs, int verifyTimeoutMs, string[] preferredOrder)
        {
            int useBaud = Resolve(baud, Settings.BaudRate);
            int useIo = Resolve(timeoutMs, Settings.TimeoutIo);
            int useVer = Resolve(verifyTimeoutMs, Settings.TimeoutVerify);
            string[] order = (preferredOrder != null && preferredOrder.Length > 0)
                ? preferredOrder
                : Settings.PreferredPorts;

            if (!string.IsNullOrEmpty(portName))
            {
                return ConnectSpecified(portName, useBaud, useIo, useVer);
            }
            else
            {
                string found;
                return ConnectAuto(out found, useBaud, useIo, useVer, order);
            }
        }

        /// <summary>Settings を使用して自動接続</summary>
        public bool Connect()
        {
            return Connect(null, 0, 0, 0, null);
        }

        /// <summary>Settings を使用して指定/自動接続</summary>
        public bool Connect(string portNameOrNull)
        {
            return Connect(portNameOrNull, 0, 0, 0, null);
        }

        /// <summary>Settings 併用の接続（0 指定は Settings へフォールバック）</summary>
        public bool Connect(string portNameOrNull, int baud, int timeoutMs, int verifyTimeoutMs)
        {
            return Connect(portNameOrNull, baud, timeoutMs, verifyTimeoutMs, null);
        }

        /// <summary>ポートを閉じる</summary>
        public void Close()
        {
            try
            {
                if (_port != null)
                {
                    if (_port.IsOpen) _port.Close();
                    _port.Dispose();
                }
            }
            catch { }
            finally
            {
                _port = null;
            }
        }

        public void Dispose() { Close(); }

        //============================================================
        // 送受信（共通）
        //============================================================

        /// <summary>
        /// 固定5文字フレーム（"hhhh!" or "!!!!!"）を厳密に待つ
        /// </summary>
        public string SendCommand(string command, int timeoutMs)
        {
            if (!IsOpen) throw new InvalidOperationException("Port is not open.");
            if (string.IsNullOrEmpty(command)) throw new ArgumentException("command is empty.", nameof(command));
            if (!command.EndsWith("!")) command += "!";

            try { _port.DiscardInBuffer(); } catch { } // ★ 念のため前ゴミ除去
            _port.Write(command);

            var sw = Stopwatch.StartNew();
            var sb = new StringBuilder(8);

            while (sw.ElapsedMilliseconds < timeoutMs)
            {
                try
                {
                    if (_port.BytesToRead > 0)
                    {
                        var chunk = _port.ReadExisting();
                        if (!string.IsNullOrEmpty(chunk))
                        {
                            sb.Append(chunk);

                            // ★ 末尾5文字だけ残す（複数フレーム結合/前ゴミ対策）
                            if (sb.Length > 5) sb.Remove(0, sb.Length - 5);

                            // ★ 5文字そろい、末尾が '!' になったら確定
                            if (sb.Length == 5 && sb[4] == '!')
                            {
                                // （必要なら IsFiveBang(sb.ToString()) で形式再確認も可）
                                return sb.ToString(); // ★ 固定5文字を返す
                            }
                        }
                    }
                }
                catch (TimeoutException)
                {
                    // ★ 1回のReadタイムアウトは無視。全体はtimeoutMsで管理
                }

                Thread.Sleep(2); // ★ 応答待ちの細かいポーリング
            }

            throw new TimeoutException("No 5-char response ending with '!'."); // ★ 明確なメッセージ
        }


        //============================================================
        // ヘルパー
        //============================================================

        /// <summary>「5文字 & 最後が '!'」なら true（"!!!!!" も含む）</summary>
        public static bool IsFiveBang(string s)
        {
            return !string.IsNullOrEmpty(s) && s.Length == 5 && s[4] == '!';
        }

        /// <summary>"hhhh!" を 16進数に変換（失敗時は false）。"!!!!!" は false。</summary>
        private static bool TryParseHex4Bang(string s, out ushort value)
        {
            value = 0;
            if (string.IsNullOrEmpty(s) || s.Length != 5 || s[4] != '!') return false;
            int v = 0;
            for (int i = 0; i < 4; i++)
            {
                char c = s[i];
                int d;
                if (c >= '0' && c <= '9') d = c - '0';
                else if (c >= 'A' && c <= 'F') d = 10 + (c - 'A');
                else if (c >= 'a' && c <= 'f') d = 10 + (c - 'a');
                else return false;
                v = (v << 4) | d;
            }
            value = (ushort)v;
            return true;
        }

        /// <summary>応答 "hhhh!" の先頭4桁を 16進数として数値化（前提: 形式妥当）</summary>
        public static ushort ParseHex4(string s)
        {
            // C# 7.3互換：AsSpanは使わない
            return Convert.ToUInt16(s.Substring(0, 4), 16);
        }

        public static string Hex1(int v) { return (v & 0xF).ToString("X1"); }
        public static string Hex2(int v) { return (v & 0xFF).ToString("X2"); }
        public static string Hex3(int v) { return (v & 0x3FF).ToString("X3"); }
        public static string Hex4(int v) { return (v & 0xFFFF).ToString("X4"); }

        //============================================================
        // （★private化）フラットAPI：実装は温存し、階層APIからのみ使用
        //============================================================

        // ---- 情報 ----
        private string GetVersion(int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutGeneral);
            return SendCommand("VER!", t);
        }

        private ushort GetPwx(int deviceId, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutPwm);
            string resp = SendCommand("PWX:" + Hex2(deviceId) + "!", t);
            ushort v;
            if (!TryParseHex4Bang(resp, out v)) throw new FormatException("PWX bad response.");
            return v;
        }

        private ushort GetDpx(int id1to4, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DPX:" + Hex2(id1to4) + "!", t);
            ushort v;
            if (!TryParseHex4Bang(resp, out v)) throw new FormatException("DPX bad response.");
            return v;
        }

        // ---- アナログ ----
        private bool AnalogConfigure(int players, int switches, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAnalog);
            if (players < 1 || players > 16) throw new ArgumentOutOfRangeException("players");
            if (switches < 1 || switches > 4) throw new ArgumentOutOfRangeException("switches");

            string resp = SendCommand("ANS:" + Hex2(players) + ":" + Hex2(switches) + "!", t);
            return resp == "!!!!!";
        }

        private bool AnalogUpdate(int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAnalog);
            string resp = SendCommand("ANU!", t);
            return resp == "!!!!!";
        }

        private int AnalogRead(int playerIndex0to15, int switchIndex0to3, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAnalog);
            if (playerIndex0to15 < 0 || playerIndex0to15 > 15) throw new ArgumentOutOfRangeException("playerIndex0to15");
            if (switchIndex0to3 < 0 || switchIndex0to3 > 3) throw new ArgumentOutOfRangeException("switchIndex0to3");

            string resp = SendCommand("ANR:" + Hex2(playerIndex0to15) + ":" + Hex2(switchIndex0to3) + "!", t);
            ushort v;
            if (TryParseHex4Bang(resp, out v)) return v;
            return -1;
        }

        // ---- PWM ----
        private string PwmValue(int channel, int value0to4095, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutPwm);
            return SendCommand("PWM:" + Hex2(channel) + ":" + Hex4(value0to4095) + "!", t);
        }

        private string PwmAngle(int channel, int angle0to180, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutPwm);
            return SendCommand("PWA:" + Hex2(channel) + ":" + Hex3(angle0to180) + "!", t);
        }

        private bool PwmInit(int angleMin, int angleMax, int pwmMin, int pwmMax, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutPwm);
            string resp = SendCommand("PWI:" + Hex3(angleMin) + ":" + Hex3(angleMax) + ":" + Hex4(pwmMin) + ":" + Hex4(pwmMax) + "!", t);
            return resp == "!!!!!";
        }

        // ---- デジタル I/O ----
        private int DigitalIn(int portId, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutDigital);
            string resp = SendCommand("POR:" + Hex2(portId) + "!", t);
            ushort v;
            if (TryParseHex4Bang(resp, out v)) return v;
            return 0;
        }

        private bool DigitalOut(int portId, int value0or1, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutDigital);
            string resp = SendCommand("POW:" + Hex2(portId) + ":" + (((value0or1 & 0x1) != 0) ? "1" : "0") + "!", t);
            return resp == "!!!!!";
        }

        private int DigitalIo(int portId, int value0or1, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutDigital);
            string resp = SendCommand("IO:" + Hex2(portId) + ":" + (((value0or1 & 0x1) != 0) ? "1" : "0") + "!", t);
            ushort v;
            if (TryParseHex4Bang(resp, out v)) return v;
            return 0;
        }

        // ---- I2C ----
        private bool I2cWrite(int addr, int reg, int value, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutI2c);
            string resp = SendCommand("I2W:" + Hex2(addr) + ":" + Hex2(reg) + ":" + Hex2(value) + "!", t);
            return resp == "!!!!!";
        }

        private int I2cRead(int addr, int reg, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutI2c);
            string resp = SendCommand("I2R:" + Hex2(addr) + ":" + Hex2(reg) + "!", t);
            ushort v;
            if (TryParseHex4Bang(resp, out v)) return v;
            return 0;
        }

        // ---- MP3（DFPlayer） ----
        private string DfPlayFolderTrack(int deviceId1to4, int folder1to255, int track1to255, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            return SendCommand("DIR:" + Hex2(deviceId1to4) + ":" + Hex2(folder1to255) + ":" + Hex2(track1to255) + "!", t);
        }

        private bool DfStop(int deviceId1to4, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DSP:" + Hex2(deviceId1to4) + "!", t);
            return resp == "!!!!!";
        }

        private bool DfPause(int deviceId1to4, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DPA:" + Hex2(deviceId1to4) + "!", t);
            return resp == "!!!!!";
        }

        private bool DfResume(int deviceId1to4, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DPR:" + Hex2(deviceId1to4) + "!", t);
            return resp == "!!!!!";
        }

        private bool DfVolume(int deviceId1to4, int vol0to30, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("VOL:" + Hex2(deviceId1to4) + ":" + Hex2(vol0to30) + "!", t);
            return resp == "!!!!!";
        }

        private bool DfSetEq(int deviceId1to4, int eqMode0to5, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DEF:" + Hex2(deviceId1to4) + ":" + Hex2(eqMode0to5) + "!", t);
            return resp == "!!!!!";
        }

        private int DfQuery(int deviceId1to4, int kind1to5, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DST:" + Hex2(deviceId1to4) + ":" + Hex2(kind1to5) + "!", t);
            ushort v;
            if (TryParseHex4Bang(resp, out v)) return v;
            return -1;
        }

        private int DfReadPlayState(int deviceId1to4, int timeoutMs) { return DfQuery(deviceId1to4, 1, timeoutMs); }
        private int DfReadVolume(int deviceId1to4, int timeoutMs) { return DfQuery(deviceId1to4, 2, timeoutMs); }
        private int DfReadEq(int deviceId1to4, int timeoutMs) { return DfQuery(deviceId1to4, 3, timeoutMs); }
        private int DfReadFileCounts(int deviceId1to4, int timeoutMs) { return DfQuery(deviceId1to4, 4, timeoutMs); }
        private int DfReadCurrentFileNumber(int deviceId1to4, int timeoutMs) { return DfQuery(deviceId1to4, 5, timeoutMs); }

        //============================================================
        // 指定接続：指定COMに開いて VER 応答を確認
        //============================================================
        public bool ConnectSpecified(string portName, int baud, int timeoutMs, int verifyTimeoutMs)
        {
            int useBaud = Resolve(baud, Settings.BaudRate);
            int useIo = Resolve(timeoutMs, Settings.TimeoutIo);
            int useVer = Resolve(verifyTimeoutMs, Settings.TimeoutVerify);

            try
            {
                Open(portName, useBaud, useIo);
                try { _port.DiscardInBuffer(); _port.DiscardOutBuffer(); } catch { }
                string ver = GetVersion(useVer); // private 呼び出し
                if (!string.IsNullOrEmpty(ver))
                    return true;
            }
            catch
            {
            }
            try { Close(); } catch { }
            return false;
        }

        //============================================================
        // 自動接続：利用可能なCOMを順次試し、VER が返った最初のポートへ接続
        //============================================================
        public bool ConnectAuto(out string connectedPort, int baud, int timeoutMsPerPort, int verifyTimeoutMs, string[] preferredOrder)
        {
            int useBaud = Resolve(baud, Settings.BaudRate);
            int useIo = Resolve(timeoutMsPerPort, Settings.TimeoutIo);
            int useVer = Resolve(verifyTimeoutMs, Settings.TimeoutVerify);

            connectedPort = null;

            string[] ports = (preferredOrder != null && preferredOrder.Length > 0)
                ? preferredOrder
                : SerialPort.GetPortNames();

            Array.Sort(ports, delegate (string a, string b)
            {
                int na = ExtractComNumber(a);
                int nb = ExtractComNumber(b);
                if (na == nb) return string.Compare(a, b, StringComparison.OrdinalIgnoreCase);
                return na.CompareTo(nb);
            });

            foreach (string p in ports)
            {
                try
                {
                    if (ConnectSpecified(p, useBaud, useIo, useVer))
                    {
                        connectedPort = p;
                        return true;
                    }
                }
                catch
                {
                }
                finally
                {
                    if (!IsOpen)
                    {
                        try { Close(); } catch { }
                    }
                }
            }
            return false;
        }

        public void Open(string portName, int baud, int timeoutMs)
        {
            if (IsOpen) Close();

            _port = new SerialPort(portName, baud, Parity.None, 8, StopBits.One);
            _port.Encoding = Encoding.ASCII;
            _port.ReadTimeout = timeoutMs;
            _port.WriteTimeout = timeoutMs;
            _port.Handshake = Handshake.None;
            _port.DtrEnable = true;
            _port.RtsEnable = true;
            _port.NewLine = "!";

            _port.Open();

            Thread.Sleep(500);

            try { _port.DiscardInBuffer(); } catch { }
            try { _port.DiscardOutBuffer(); } catch { }
        }

        private static int ExtractComNumber(string portName)
        {
            if (string.IsNullOrEmpty(portName)) return int.MaxValue;
            int i = 0;
            while (i < portName.Length && !char.IsDigit(portName[i])) i++;
            int j = i;
            while (j < portName.Length && char.IsDigit(portName[j])) j++;
            int n;
            if (i < j && int.TryParse(portName.Substring(i, j - i), out n)) return n;
            return int.MaxValue;
        }

        //============================================================
        // 階層API：Info
        //============================================================
        public sealed class InfoModule
        {
            private readonly MmpClient _p;
            public DevModule Dev { get; private set; }

            public InfoModule(MmpClient parent)
            {
                _p = parent;
                Dev = new DevModule(parent);
            }

            public string Version() { return _p.GetVersion(0); }
            public string Version(int timeoutMs) { return _p.GetVersion(timeoutMs); }

            public sealed class DevModule
            {
                private readonly MmpClient _p;
                public DevModule(MmpClient parent) { _p = parent; }
                public ushort Pwm(int deviceId) { return _p.GetPwx(deviceId, 0); }
                public ushort Pwm(int deviceId, int timeoutMs) { return _p.GetPwx(deviceId, timeoutMs); }
                public ushort Audio(int id1to4) { return _p.GetDpx(id1to4, 0); }
                public ushort Audio(int id1to4, int timeoutMs) { return _p.GetDpx(id1to4, timeoutMs); }
            }
        }

        //============================================================
        // 階層API：Analog
        //============================================================
        public sealed class AnalogModule
        {
            private readonly MmpClient _p;
            public AnalogModule(MmpClient parent) { _p = parent; }
            public bool Configure(int players, int switches) { return _p.AnalogConfigure(players, switches, 0); }
            public bool Configure(int players, int switches, int timeoutMs) { return _p.AnalogConfigure(players, switches, timeoutMs); }
            public bool Update() { return _p.AnalogUpdate(0); }
            public bool Update(int timeoutMs) { return _p.AnalogUpdate(timeoutMs); }
            public int Read(int playerIndex0to15, int switchIndex0to3) { return _p.AnalogRead(playerIndex0to15, switchIndex0to3, 0); }
            public int Read(int playerIndex0to15, int switchIndex0to3, int timeoutMs) { return _p.AnalogRead(playerIndex0to15, switchIndex0to3, timeoutMs); }
        }

        //============================================================
        // 階層API：Pwm
        //============================================================
        public sealed class PwmModule
        {
            private readonly MmpClient _p;
            public PwmModule(MmpClient parent) { _p = parent; }
            public ushort Info(int deviceId) { return _p.GetPwx(deviceId, 0); }
            public ushort Info(int deviceId, int timeoutMs) { return _p.GetPwx(deviceId, timeoutMs); }
            public string Out(int channel, int value0to4095) { return _p.PwmValue(channel, value0to4095, 0); }
            public string Out(int channel, int value0to4095, int timeoutMs) { return _p.PwmValue(channel, value0to4095, timeoutMs); }
            public bool AngleInit(int angleMin, int angleMax, int pwmMin, int pwmMax) { return _p.PwmInit(angleMin, angleMax, pwmMin, pwmMax, 0); }
            public bool AngleInit(int angleMin, int angleMax, int pwmMin, int pwmMax, int timeoutMs) { return _p.PwmInit(angleMin, angleMax, pwmMin, pwmMax, timeoutMs); }
            public string AngleOut(int channel, int angle0to180) { return _p.PwmAngle(channel, angle0to180, 0); }
            public string AngleOut(int channel, int angle0to180, int timeoutMs) { return _p.PwmAngle(channel, angle0to180, timeoutMs); }
        }

        //============================================================
        // 階層API：Audio
        //============================================================
        public sealed class AudioModule
        {
            private readonly MmpClient _p;
            public PlayModule Play { get; private set; }

            public AudioModule(MmpClient parent)
            {
                _p = parent;
                Play = new PlayModule(parent);
            }

            public ushort Info(int id1to4) { return _p.GetDpx(id1to4, 0); }
            public ushort Info(int id1to4, int timeoutMs) { return _p.GetDpx(id1to4, timeoutMs); }

            public bool Volume(int deviceId1to4, int vol0to30) { return _p.DfVolume(deviceId1to4, vol0to30, 0); }
            public bool Volume(int deviceId1to4, int vol0to30, int timeoutMs) { return _p.DfVolume(deviceId1to4, vol0to30, timeoutMs); }

            public bool SetEq(int deviceId1to4, int eq0to5) { return _p.DfSetEq(deviceId1to4, eq0to5, 0); }
            public bool SetEq(int deviceId1to4, int eq0to5, int timeoutMs) { return _p.DfSetEq(deviceId1to4, eq0to5, timeoutMs); }

            public int ReadPlayState(int deviceId1to4) { return _p.DfReadPlayState(deviceId1to4, 0); }
            public int ReadPlayState(int deviceId1to4, int timeoutMs) { return _p.DfReadPlayState(deviceId1to4, timeoutMs); }
            public int ReadVolume(int deviceId1to4) { return _p.DfReadVolume(deviceId1to4, 0); }
            public int ReadVolume(int deviceId1to4, int timeoutMs) { return _p.DfReadVolume(deviceId1to4, timeoutMs); }
            public int ReadEq(int deviceId1to4) { return _p.DfReadEq(deviceId1to4, 0); }
            public int ReadEq(int deviceId1to4, int timeoutMs) { return _p.DfReadEq(deviceId1to4, timeoutMs); }
            public int ReadFileCounts(int deviceId1to4) { return _p.DfReadFileCounts(deviceId1to4, 0); }
            public int ReadFileCounts(int deviceId1to4, int timeoutMs) { return _p.DfReadFileCounts(deviceId1to4, timeoutMs); }
            public int ReadCurrentFileNumber(int deviceId1to4) { return _p.DfReadCurrentFileNumber(deviceId1to4, 0); }
            public int ReadCurrentFileNumber(int deviceId1to4, int timeoutMs) { return _p.DfReadCurrentFileNumber(deviceId1to4, timeoutMs); }

            // 便宜上、Play.* を直下にも露出
            public string PlayFolderTrack(int deviceId1to4, int folder1to255, int track1to255) { return _p.DfPlayFolderTrack(deviceId1to4, folder1to255, track1to255, 0); }
            public string PlayFolderTrack(int deviceId1to4, int folder1to255, int track1to255, int timeoutMs) { return _p.DfPlayFolderTrack(deviceId1to4, folder1to255, track1to255, timeoutMs); }
            public bool Stop(int deviceId1to4) { return _p.DfStop(deviceId1to4, 0); }
            public bool Stop(int deviceId1to4, int timeoutMs) { return _p.DfStop(deviceId1to4, timeoutMs); }
            public bool Pause(int deviceId1to4) { return _p.DfPause(deviceId1to4, 0); }
            public bool Pause(int deviceId1to4, int timeoutMs) { return _p.DfPause(deviceId1to4, timeoutMs); }
            public bool Resume(int deviceId1to4) { return _p.DfResume(deviceId1to4, 0); }
            public bool Resume(int deviceId1to4, int timeoutMs) { return _p.DfResume(deviceId1to4, timeoutMs); }

            public sealed class PlayModule
            {
                private readonly MmpClient _p;
                public PlayModule(MmpClient parent) { _p = parent; }
                public string FolderTrack(int deviceId1to4, int folder1to255, int track1to255) { return _p.DfPlayFolderTrack(deviceId1to4, folder1to255, track1to255, 0); }
                public string FolderTrack(int deviceId1to4, int folder1to255, int track1to255, int timeoutMs) { return _p.DfPlayFolderTrack(deviceId1to4, folder1to255, track1to255, timeoutMs); }
                public bool Stop(int deviceId1to4) { return _p.DfStop(deviceId1to4, 0); }
                public bool Stop(int deviceId1to4, int timeoutMs) { return _p.DfStop(deviceId1to4, timeoutMs); }
                public bool Pause(int deviceId1to4) { return _p.DfPause(deviceId1to4, 0); }
                public bool Pause(int deviceId1to4, int timeoutMs) { return _p.DfPause(deviceId1to4, timeoutMs); }
                public bool Resume(int deviceId1to4) { return _p.DfResume(deviceId1to4, 0); }
                public bool Resume(int deviceId1to4, int timeoutMs) { return _p.DfResume(deviceId1to4, timeoutMs); }
            }
        }

        //============================================================
        // 階層API：Digital  ★ 追加
        //============================================================
        public sealed class DigitalModule
        {
            private readonly MmpClient _p;
            public DigitalModule(MmpClient parent) { _p = parent; }

            public int In(int portId) { return _p.DigitalIn(portId, 0); }
            public int In(int portId, int timeoutMs) { return _p.DigitalIn(portId, timeoutMs); }

            public bool Out(int portId, int value0or1) { return _p.DigitalOut(portId, value0or1, 0); }
            public bool Out(int portId, int value0or1, int timeoutMs) { return _p.DigitalOut(portId, value0or1, timeoutMs); }

            public int Io(int portId, int value0or1) { return _p.DigitalIo(portId, value0or1, 0); }
            public int Io(int portId, int value0or1, int timeoutMs) { return _p.DigitalIo(portId, value0or1, timeoutMs); }
        }

        //============================================================
        // 階層API：I2c  ★ 追加
        //============================================================
        public sealed class I2cModule
        {
            private readonly MmpClient _p;
            public I2cModule(MmpClient parent) { _p = parent; }

            public bool Write(int addr, int reg, int value) { return _p.I2cWrite(addr, reg, value, 0); }
            public bool Write(int addr, int reg, int value, int timeoutMs) { return _p.I2cWrite(addr, reg, value, timeoutMs); }

            public int Read(int addr, int reg) { return _p.I2cRead(addr, reg, 0); }
            public int Read(int addr, int reg, int timeoutMs) { return _p.I2cRead(addr, reg, timeoutMs); }
        }
    }
}
