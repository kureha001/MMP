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
        // ★ Added: 既定ボーレート候補（Arduinoの MMP_BAUD_CANDIDATES に相当）
        public static readonly int[] BAUD_CANDIDATES = new int[] { 115200, 9600, 230400, 921600 };

        private SerialPort _port;

        //================================
        //===== 規定値(構造体データ) =====
        //================================
        /// <summary>設定（既定値の集約）。数値は >0 指定で優先、<=0 は既定にフォールバック。</summary>
        public sealed class MmpSettings
        {
            /// <summary>既定ボーレート</summary>
            public string PortName = null;
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
            /// <summary>アナログ分解能ビット数（例:10→0..1023）</summary>
            public int AnalogBits = 10;
        }

        /// <summary>設定オブジェクト。必要に応じてプロパティを書き換えてください。</summary>
        public MmpSettings Settings { get; } = new MmpSettings();

        //------------------------
        //---- プロパティ実装 ----
        //------------------------
        /// <summary>ポートが開いているか</summary>
        public bool   IsOpen        { get { return _port != null && _port.IsOpen    ; } }

        /// <summary>現在接続中のポート名（未接続なら null）</summary>
        public string PortName      { get { return IsOpen ? _port.PortName : null   ; } }

        /// <summary>現在接続中のボーレート（未接続なら null）</summary>
        public int    ConnectedBaud { get { return IsOpen ? _port.BaudRate : -1     ; } }

        // ========================
        // ==== モジュール実装 ====
        // ========================
        public InfoModule    Info    { get; private set; }
        public AnalogModule  Analog  { get; private set; }
        public PwmModule     Pwm     { get; private set; }
        public AudioModule   Audio   { get; private set; }
        public DigitalModule Digital { get; private set; } 
        public I2cModule     I2c     { get; private set; }

        public MmpClient()
        {
            // C# 7.3 互換で初期化
            Info    = new InfoModule    (this);
            Analog  = new AnalogModule  (this);
            Pwm     = new PwmModule     (this);
            Audio   = new AudioModule   (this);
            Digital = new DigitalModule (this); 
            I2c     = new I2cModule     (this); 
        }

        //=============================
        //===== 低レイヤ コマンド =====
        //=============================
        //------------------------------
        // 自動ボーレート接続
        //------------------------------
        // ★ Added: 候補ボーレートを順次トライして接続
        public bool ConnectAutoBaud(
            out string connectedPort        ,
            int[] cand               = null ,
            int timeoutMsPerTry      = 0    ,
            int verifyTimeoutMs      = 0    ,
            string[] preferredOrder  = null
        ){
            connectedPort = null;

            // 候補リスト（未指定ならデフォルト候補）
            int[] candidates = (cand != null && cand.Length > 0) ? cand : BAUD_CANDIDATES;

            int useIo  = Resolve(timeoutMsPerTry, Settings.TimeoutIo);
            int useVer = Resolve(verifyTimeoutMs, Settings.TimeoutVerify);

            // ポート列挙（既存ロジック踏襲）
            string[] ports = (preferredOrder != null && preferredOrder.Length > 0)
                ? preferredOrder
                : Settings.PreferredPorts ?? System.IO.Ports.SerialPort.GetPortNames();

            Array.Sort(ports, delegate (string a, string b)
            {
                int na = ExtractComNumber(a);
                int nb = ExtractComNumber(b);
                if (na == nb) return string.Compare(a, b, StringComparison.OrdinalIgnoreCase);
                return na.CompareTo(nb);
            });

            foreach (string p in ports)
            {
                foreach (int baud in candidates)
                {
                    try
                    {
                        if (ConnectSpecified(p, baud, useIo, useVer))
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
            }
            return false;
        }

        //------------------------------------------------------------
        // 接続（複数タイプ）
        // Connect: 明示引数(>0)が最優先、0/未指定は Settings を使用
        //------------------------------------------------------------
        /// <summary>統一エントリ。portName=null/空で自動接続、指定で固定ポート接続。</summary>
        public bool Connect(string portName, int baud, int timeoutMs, int verifyTimeoutMs, string[] preferredOrder)
        {
            int useBaud = Resolve(baud,            Settings.BaudRate        );
            int useIo   = Resolve(timeoutMs,       Settings.TimeoutIo       );
            int useVer  = Resolve(verifyTimeoutMs, Settings.TimeoutVerify   );

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

        //------------------------------------------------------------
        // 切断１
        //------------------------------------------------------------
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

        //------------------------------------------------------------
        // 切断２
        //------------------------------------------------------------
        public void Dispose() { Close(); }

        //------------------------------------------------------------
        // ＭＭＰシリアル通信
        //------------------------------------------------------------
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
        private int Resolve(int value, int fallback) { return (value > 0) ? value : fallback; }

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
        public static string Hex1(int v) { return (v & 0xF   ).ToString("X1"); }
        public static string Hex2(int v) { return (v & 0xFF  ).ToString("X2"); }
        public static string Hex3(int v) { return (v & 0x3FF ).ToString("X3"); }
        public static string Hex4(int v) { return (v & 0xFFFF).ToString("X4"); }

        //===============================
        //=====（内部用）フラットAPI=====
        //===============================
        // -----------------------
        // --- 情報モジュール用 ----
        // -----------------------
        private string GetVersion(int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutGeneral);
            return SendCommand("VER!", t);
        }

        private ushort GetPwx(int devId0to15, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutPwm);
            string resp = SendCommand("PWX:" + Hex2(devId0to15) + "!", t);
            ushort v;
            if (!TryParseHex4Bang(resp, out v)) throw new FormatException("PWX bad response.");
            return v;
        }

        private ushort GetDpx(int devId1to4, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DPX:" + Hex2(devId1to4) + "!", t);
            ushort v;
            if (!TryParseHex4Bang(resp, out v)) throw new FormatException("DPX bad response.");
            return v;
        }

        // --------------------------------
        // ---- アナログ モジュール用 -----
        // --------------------------------
        private bool AnalogConfigure(int hc4067chTtl, int hc4067devTtl, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAnalog);
            if (hc4067chTtl  < 1 || hc4067chTtl  > 16) throw new ArgumentOutOfRangeException(nameof(hc4067chTtl));
            if (hc4067devTtl < 1 || hc4067devTtl > 4 ) throw new ArgumentOutOfRangeException(nameof(hc4067devTtl));

            string resp = SendCommand("ANS:" + Hex2(hc4067chTtl) + ":" + Hex2(hc4067devTtl) + "!", t);
            return resp == "!!!!!";
        }

        private bool AnalogUpdate(int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAnalog);
            string resp = SendCommand("ANU!", t);
            return resp == "!!!!!";
        }

        private int AnalogRead(int hc4067ch0to15, int hc4067dev0to3, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAnalog);
            if (hc4067ch0to15 < 0 || hc4067ch0to15 > 15) throw new ArgumentOutOfRangeException(nameof(hc4067ch0to15));
            if (hc4067dev0to3 < 0 || hc4067dev0to3 > 3 ) throw new ArgumentOutOfRangeException(nameof(hc4067dev0to3));

            string resp = SendCommand("ANR:" + Hex2(hc4067ch0to15) + ":" + Hex2(hc4067dev0to3) + "!", t);
            ushort v;
            if (TryParseHex4Bang(resp, out v)) return v;
            return -1;
        }

        // -----------------------------
        // ---- ＰＷＭモジュール用 -----
        // -----------------------------
        private bool PwmValue(int chId0to255, int val0to4095, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutPwm);
            string resp = SendCommand("PWM:" + Hex2(chId0to255) + ":" + Hex4(val0to4095) + "!", t);
            return resp == "!!!!!";
        }

        private bool PwmAngle(int chId0to255, int angle0to180, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutPwm);
            string resp = SendCommand("PWA:" + Hex2(chId0to255) + ":" + Hex3(angle0to180) + "!", t);
            return resp == "!!!!!";
        }

        private bool PwmInit(int angleMin, int angleMax, int pwmMin, int pwmMax, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutPwm);
            string resp = SendCommand("PWI:" + Hex3(angleMin) + ":" + Hex3(angleMax) + ":" + Hex4(pwmMin) + ":" + Hex4(pwmMax) + "!", t);
            return resp == "!!!!!";
        }

        // --------------------------------
        // ---- デジタル モジュール用 -----
        // --------------------------------
        private int DigitalIn(int gpioId, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutDigital);
            string resp = SendCommand("POR:" + Hex2(gpioId) + "!", t);
            ushort v;
            if (TryParseHex4Bang(resp, out v)) return v;
            return 0;
        }

        private bool DigitalOut(int gpioId, int val0or1, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutDigital);
            string resp = SendCommand("POW:" + Hex2(gpioId) + ":" + (((val0or1 & 0x1) != 0) ? "1" : "0") + "!", t);
            return resp == "!!!!!";
        }

        // -----------------------------
        // ---- Ｉ２Ｃモジュール用 -----
        // -----------------------------
        private bool I2cWrite(int addr, int reg, int val, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutI2c);
            string resp = SendCommand("I2W:" + Hex2(addr) + ":" + Hex2(reg) + ":" + Hex2(val) + "!", t);
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

        // ---------------------------
        // ---- 音声モジュール用 -----
        // ---------------------------
        private bool DfStart(int devId1to4, int dir1to255, int file1to255, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DIR:" + Hex2(devId1to4) + ":" + Hex2(dir1to255) + ":" + Hex2(file1to255) + "!", t);
            return resp == "!!!!!";
        }
        private bool DfSetLoop(int devId1to4, int on0or1, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DLP:" + Hex2(devId1to4) + ":" + Hex2(on0or1) + "!", t);
            return resp == "!!!!!";
        }

        private bool DfStop(int devId1to4, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DSP:" + Hex2(devId1to4) + "!", t);
            return resp == "!!!!!";
        }

        private bool DfPause(int devId1to4, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DPA:" + Hex2(devId1to4) + "!", t);
            return resp == "!!!!!";
        }

        private bool DfResume(int devId1to4, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DPR:" + Hex2(devId1to4) + "!", t);
            return resp == "!!!!!";
        }

        private bool DfVolume(int devId1to4, int vol0to30, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("VOL:" + Hex2(devId1to4) + ":" + Hex2(vol0to30) + "!", t);
            return resp == "!!!!!";
        }

        private bool DfSetEq(int devId1to4, int mode0to5, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DEF:" + Hex2(devId1to4) + ":" + Hex2(mode0to5) + "!", t);
            return resp == "!!!!!";
        }

        private int DfQuery(int devId1to4, int kind1to5, int timeoutMs)
        {
            int t = Resolve(timeoutMs, Settings.TimeoutAudio);
            string resp = SendCommand("DST:" + Hex2(devId1to4) + ":" + Hex2(kind1to5) + "!", t);
            ushort v;
            if (TryParseHex4Bang(resp, out v)) return v;
            return -1;
        }

        private int DfReadState     (int devId1to4, int timeoutMs) { return DfQuery(devId1to4, 1, timeoutMs); }
        private int DfReadVolume    (int devId1to4, int timeoutMs) { return DfQuery(devId1to4, 2, timeoutMs); }
        private int DfReadEq        (int devId1to4, int timeoutMs) { return DfQuery(devId1to4, 3, timeoutMs); }
        private int DfReadFileCounts(int devId1to4, int timeoutMs) { return DfQuery(devId1to4, 4, timeoutMs); }
        private int DfReadFileNumber(int devId1to4, int timeoutMs) { return DfQuery(devId1to4, 5, timeoutMs); }


        //=============================
        //===== 低レイヤ コマンド =====
        //=============================
        // 指定接続：指定COMに開いて、VER が返ったら接続
        public bool ConnectSpecified(string portName, int baud, int timeoutMs, int verifyTimeoutMs)
        {
            int useBaud = Resolve(baud, Settings.BaudRate);
            int useIo   = Resolve(timeoutMs, Settings.TimeoutIo);
            int useVer  = Resolve(verifyTimeoutMs, Settings.TimeoutVerify);

            try
            {
                Open(portName, useBaud, useIo);
                try { _port.DiscardInBuffer(); _port.DiscardOutBuffer(); } catch { }
                string ver = GetVersion(useVer); // private 呼び出し
                if (!string.IsNullOrEmpty(ver))
                {
                    Settings.PortName = portName;
                    Settings.BaudRate = useBaud;
                    return true;
                }
            }
            catch
            {
            }
            try { Close(); } catch { }
            return false;
        }

        // 自動接続：利用可能なCOMを順次試し、VER が返った最初のポートへ接続
        public bool ConnectAuto(out string connectedPort, int baud, int timeoutMsPerPort, int verifyTimeoutMs, string[] preferredOrder)
        {
            int useBaud = Resolve(baud, Settings.BaudRate);
            int useIo   = Resolve(timeoutMsPerPort, Settings.TimeoutIo);
            int useVer  = Resolve(verifyTimeoutMs, Settings.TimeoutVerify);

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

        // シリアル通信を開く
        public void Open(string portName, int baud, int timeoutMs)
        {
            if (IsOpen) Close();

            _port = new SerialPort(portName, baud, Parity.None, 8, StopBits.One);
            _port.Encoding     = Encoding.ASCII;
            _port.ReadTimeout  = timeoutMs;
            _port.WriteTimeout = timeoutMs;
            _port.Handshake    = Handshake.None;
            _port.DtrEnable    = true;
            _port.RtsEnable    = true;
            _port.NewLine =    "!";

            _port.Open();

            Thread.Sleep(500);

            try { _port.DiscardInBuffer(); } catch { }
            try { _port.DiscardOutBuffer(); } catch { }
        }

        // COM検出用ヘルパ
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

        //=============================
        //=====（公開用）階層化API=====
        //=============================
        // ------------------------
        // ---- 情報モジュール ----
        // ------------------------
        public sealed class InfoModule
        {
            private readonly MmpClient _p;
            public DevModule Dev { get; private set; }

            public InfoModule(MmpClient parent)
            {
                _p = parent;
                Dev = new DevModule(parent);
            }

            public string Version(int timeoutMs = 0) { return _p.GetVersion(timeoutMs); }

            public sealed class DevModule
            {
                private readonly MmpClient _p;
                public DevModule(MmpClient parent) { _p = parent; }
                public ushort Pwm(int devId0to15, int timeoutMs = 0) { return _p.GetPwx(devId0to15, timeoutMs); }
                public ushort Audio(int devId1to4, int timeoutMs = 0) { return _p.GetDpx(devId1to4, timeoutMs); }
            }
        }

        // -----------------------------
        // ---- アナログ モジュール ----
        // -----------------------------
        public sealed class AnalogModule
        {
            private readonly MmpClient _p;
            public AnalogModule(MmpClient parent) { _p = parent; }

            // ---- 共通ヘルパ（AnalogModule内専用）----
            private int ResolveBits(int bits)
            {
                int b = (bits >= 1 && bits <= 16) ? bits : _p.Settings.AnalogBits;
                if (b < 1 ) b = 1 ;
                if (b > 16) b = 16;
                return b;
            }

            private int ClampMaxForBits(int v, int bits)
            {
                int max = (1 << bits) - 1;
                if (v < 0) return v;         // 生値がエラー(-1等)のときはそのまま伝播
                if (v > max) return max;
                return v;
            }

            private int RoundNearest(int raw, int step, int bits)
            {
                if (raw < 0) return raw;     // エラーはそのまま返す
                int b = ResolveBits(bits);
                raw = ClampMaxForBits(raw, b);
                if (step <= 0) return raw;

                int down = (raw / step) * step;
                int rem = raw - down;
                // half-up（偶数stepでは rem>=step/2 を切り上げ）
                if (rem * 2 >= step)
                {
                    long up = (long)down + step;
                    int max = (1 << b) - 1;
                    if (up > max) up = max;
                    return (int)up;
                }
                return down;
            }

            private int RoundUp(int raw, int step, int bits)
            {
                if (raw < 0) return raw;
                int b = ResolveBits(bits);
                raw = ClampMaxForBits(raw, b);
                if (step <= 0) return raw;

                long q = ((long)raw + step - 1) / step;
                long up = q * step;
                int max = (1 << b) - 1;
                if (up > max) up = max;
                return (int)up;
            }

            private int RoundDown(int raw, int step, int bits)
            {
                if (raw < 0) return raw;
                int b = ResolveBits(bits);
                raw = ClampMaxForBits(raw, b);
                if (step <= 0) return raw;

                int down = (raw / step) * step;
                if (down < 0) down = 0;
                return down;
            }

            // ---- 設定系（既存そのまま）----
            public bool Configure(int hc4067chTtl, int hc4067devTtl, int timeoutMs = 0)
            {
                return _p.AnalogConfigure(hc4067chTtl, hc4067devTtl, timeoutMs);
            }

            public bool Update(int timeoutMs = 0)
            {
                return _p.AnalogUpdate(timeoutMs);
            }

            // ---- 取得系（新API）----

            // 1) 丸めなし（生値を返す）
            public int Read(int hc4067ch0to15, int hc4067dev0to3, int timeoutMs = 0)
            {
                return _p.AnalogRead(hc4067ch0to15, hc4067dev0to3, timeoutMs);
            }

            // 2) 丸めあり：中央値基準の最近傍（偶数stepは half-up）
            public int ReadRound(int hc4067ch0to15, int hc4067dev0to3, int step, int bits = 0, int timeoutMs = 0)
            {
                int v = _p.AnalogRead(hc4067ch0to15, hc4067dev0to3, timeoutMs);
                return RoundNearest(v, step, bits);
            }

            // 3) 丸めあり：切り上げ
            public int ReadRoundUp(int hc4067ch0to15, int hc4067dev0to3, int step, int bits = 0, int timeoutMs = 0)
            {
                int v = _p.AnalogRead(hc4067ch0to15, hc4067dev0to3, timeoutMs);
                return RoundUp(v, step, bits);
            }

            // 4) 丸めあり：切り下げ
            public int ReadRoundDown(int hc4067ch0to15, int hc4067dev0to3, int step, int bits = 0, int timeoutMs = 0)
            {
                int v = _p.AnalogRead(hc4067ch0to15, hc4067dev0to3, timeoutMs);
                return RoundDown(v, step, bits);
            }
        }

        // -----------------------------
        // ---- デジタル モジュール ----
        // -----------------------------
        public sealed class DigitalModule
        {
            private readonly MmpClient _p;
            public DigitalModule(MmpClient parent) { _p = parent; }

            public int In(int gpioId, int timeoutMs = 0) { return _p.DigitalIn(gpioId, timeoutMs); }
            public bool Out(int gpioId, int val0or1, int timeoutMs = 0) { return _p.DigitalOut(gpioId, val0or1, timeoutMs); }
        }

        // --------------------------
        // ---- ＰＷＭモジュール ----
        // --------------------------
        public sealed class PwmModule
        {
            private readonly MmpClient _p;
            public PwmModule(MmpClient parent) { _p = parent; }
            public ushort Info(int devId0to15, int timeoutMs = 0) { return _p.GetPwx(devId0to15, timeoutMs); }
            public bool   Out(int chId0to255, int val0to4095, int timeoutMs = 0) { return _p.PwmValue(chId0to255, val0to4095, timeoutMs); }
            public bool   AngleInit(int angleMin, int angleMax, int pwmMin, int pwmMax, int timeoutMs = 0) { return _p.PwmInit(angleMin, angleMax, pwmMin, pwmMax, timeoutMs); }
            public bool   AngleOut(int chId0to255, int angle0to180, int timeoutMs = 0) { return _p.PwmAngle(chId0to255, angle0to180, timeoutMs); }
        }

        // ------------------------
        // ---- 音声モジュール ----
        // ------------------------
        public sealed class AudioModule
        {
            private readonly MmpClient _p;
            public PlayModule Play { get; private set; }
            public ReadModule Read { get; private set; }

            public AudioModule(MmpClient parent)
            {
                _p = parent;
                Play = new PlayModule(parent);
                Read = new ReadModule(parent);
            }

            // 便宜上、Play.* を直下にも露出
            public bool Start  (int devId1to4, int dir1to255, int file1to255, int timeoutMs = 0)
            { return _p.DfStart(devId1to4, dir1to255, file1to255, timeoutMs); }

            public bool Stop   (int devId1to4, int timeoutMs = 0) { return _p.DfStop(devId1to4, timeoutMs); }
            public bool Pause  (int devId1to4, int timeoutMs = 0) { return _p.DfPause(devId1to4, timeoutMs); }
            public bool Resume (int devId1to4, int timeoutMs = 0) { return _p.DfResume(devId1to4, timeoutMs); }

            // ----------------------
            // ---- 単独コマンド ----
            // ----------------------
            public ushort Info  (int devId1to4,               int timeoutMs = 0) { return _p.GetDpx  (devId1to4,           timeoutMs); }
            public bool   Volume(int devId1to4, int vol0to30, int timeoutMs = 0) { return _p.DfVolume(devId1to4, vol0to30, timeoutMs); }
            public bool   SetEq (int devId1to4, int mode0to5, int timeoutMs = 0) { return _p.DfSetEq (devId1to4, mode0to5, timeoutMs); }


            // ----------------------
            // ---- サブ：再生系 ----
            // ----------------------
            public sealed class PlayModule
            {
                private readonly MmpClient _p;

                public PlayModule(MmpClient parent) { _p = parent; }
                public bool Start  (int devId1to4, int dir1to255, int file1to255,
                                                                int timeoutMs = 0){ return _p.DfStart  (devId1to4, dir1to255, file1to255, timeoutMs); }
                public bool SetLoop(int devId1to4, bool enable, int timeoutMs = 0){ return _p.DfSetLoop(devId1to4, enable ? 1 : 0, timeoutMs); }
                public bool Stop   (int devId1to4,              int timeoutMs = 0){ return _p.DfStop   (devId1to4, timeoutMs); }
                public bool Pause  (int devId1to4,              int timeoutMs = 0){ return _p.DfPause  (devId1to4, timeoutMs); }
                public bool Resume (int devId1to4,              int timeoutMs = 0){ return _p.DfResume (devId1to4, timeoutMs); }
            }

            // ----------------------
            // ---- サブ：参照系 ----
            // ----------------------
            public sealed class ReadModule
            {
                private readonly MmpClient _p;
                public ReadModule(MmpClient parent) { _p = parent; }
                public int State     (int devId1to4, int timeoutMs = 0) { return _p.DfReadState(devId1to4, timeoutMs); }
                public int Volume    (int devId1to4, int timeoutMs = 0) { return _p.DfReadVolume(devId1to4, timeoutMs); }
                public int Eq        (int devId1to4, int timeoutMs = 0) { return _p.DfReadEq(devId1to4, timeoutMs); }
                public int FileCounts(int devId1to4, int timeoutMs = 0) { return _p.DfReadFileCounts(devId1to4, timeoutMs); }
                public int FileNumber(int devId1to4, int timeoutMs = 0) { return _p.DfReadFileNumber(devId1to4, timeoutMs); }
            }
        }

        // --------------------------
        // ---- Ｉ２Ｃモジュール ----
        // --------------------------
        public sealed class I2cModule
        {
            private readonly MmpClient _p;
            public I2cModule(MmpClient parent) { _p = parent; }
            public bool Write(int addr, int reg, int val, int timeoutMs = 0) { return _p.I2cWrite(addr, reg, val, timeoutMs); }
            public int Read(int addr, int reg, int timeoutMs = 0) { return _p.I2cRead(addr, reg, timeoutMs); }
        }
    }
}
