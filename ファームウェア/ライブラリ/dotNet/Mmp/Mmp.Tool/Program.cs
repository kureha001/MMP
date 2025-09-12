using System;
using System.Data;
using System.Runtime.Remoting.Messaging;
using System.Threading;

namespace Mmp.Tool
{
    internal static class Program
    {
        static int Main(string[] args)
        {
            string port = args.Length > 0 ? args[0] : null;
            Console.WriteLine("<< ＭＭＰライブラリ for .Net with C#>>\n");
            Console.Write("接続中...");

            try
            {
                using (var mmp = new Mmp.Core.MmpClient())
                {
                    //mmp.Settings.BaudRate = 115200;
                    //mmp.Settings.TimeoutAudio = 800;
                    bool connected = (port == null) ? mmp.ConnectAutoBaud(out port) : mmp.Connect(port);
                    if (!connected || !mmp.IsOpen)
                    {
                        Console.WriteLine("ＭＭＰとの接続に失敗しました...");
                        return 1;
                    }

                    Console.WriteLine("");
                    RunInfo(mmp); // 情報系

                    // === 必要なテストだけコメントを外して実行してください ===
                    Console.WriteLine("\n＝＝＝ ＭＭＰ ＡＰＩテスト［開始］＝＝＝\n");
                    //RunAnalog(mmp)     ; // アナログ入力
                    //RunDigital(mmp)    ; // デジタル入出力
                    //RunMp3Playlist(mmp); // MP3プレイヤー(基本)
                    //RunMp3Control(mmp) ; // MP3プレイヤー(制御)
                    //RunPwm(mmp, true ) ; // PWM出力
                    //RunPwm(mmp, false) ; // I2C→PCA9685 直接制御
                    Console.WriteLine("＝＝＝ ＭＭＰ ＡＰＩテスト［終了］＝＝＝");
                }
                return 0;
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("ERR: " + ex.GetType().Name + " - " + ex.Message);
                return 1;
            }
        }

        //============================================================
        // 0) 基本情報
        //============================================================
        private static void RunInfo(Mmp.Core.MmpClient mmp)
        {
            Console.WriteLine("　・通信ポート  : " + mmp.PortName);
            Console.WriteLine("　・接続速度    : " + mmp.ConnectedBaud + "bps");
            Console.WriteLine("　・バージョン  : " + mmp.Info.Version());
            Console.WriteLine("　・PCA9685 [0] : 0x{0:X4}", mmp.Info.Dev.Pwm(0));
            Console.WriteLine("　・DFPlayer[1] : 0x{0:X4}", mmp.Info.Dev.Audio(1));
        }

        //============================================================
        // 1) アナログ入力
        //============================================================
        private static void RunAnalog(Mmp.Core.MmpClient mmp)
        {
            Console.WriteLine("１.アナログ入力（ HC4067：JoyPad1,2 ）");

            bool ok = mmp.Analog.Configure(2, 4);
            Console.WriteLine("　・アクセス範囲指定 → [2,4]  : " + ok);
            if (!ok)
            {
                Console.WriteLine("　<<中断>>");
                return;
            }
 
            ok = mmp.Analog.Update();
            Console.WriteLine("　・アナログ値をバッファに格納 : " + ok);
            if (!ok)
            {
                Console.WriteLine("　<<中断>>");
                return;
            }

            Console.WriteLine("　・バッファを参照");
            for (int x = 0; x <= 1; x += 1)
            {
                Console.WriteLine("　　JoyPad[" + (x + 1) + "]");
                for (int y = 0; y <= 3; y += 1)
                {
                    //Console.WriteLine("　　← [" + y + "] = " + mmp.Analog.Read         (x, y         ) );
                    //Console.WriteLine("　　← [" + y + "] = " + mmp.Analog.ReadRound    (x, y, 100, 10) );
                    //Console.WriteLine("　　← [" + y + "] = " + mmp.Analog.ReadRoundUp  (x, y, 100, 10) );
                    Console.WriteLine  ("　　← [" + y + "] = " + mmp.Analog.ReadRoundDown(x, y, 100, 10) );
                }
            }

            Console.WriteLine("　[終了]\n");

        }

        //============================================================
        // 2) デジタル入出力
        //============================================================
        private static void RunDigital(Mmp.Core.MmpClient mmp)
        {
            Console.WriteLine("２.デジタル入出力（ GPIO ）");
            Console.WriteLine("　・入力");
            Console.WriteLine("　　←[2] = " + int2strOnOff(mmp.Digital.In(2)) );
            Console.WriteLine("　　←[6] = " + int2strOnOff(mmp.Digital.In(6)) );
            Console.WriteLine("　　←[7] = " + int2strOnOff(mmp.Digital.In(7)) );

            Console.WriteLine("　・出力[3]" );
            for (int cnt = 0; cnt <= 5; cnt++)
            {
                Console.WriteLine("　　→ HIGH : " + mmp.Digital.Out(3, 1) ); Thread.Sleep(250);
                Console.WriteLine("　　→ LOW  : " + mmp.Digital.Out(3, 0) ); Thread.Sleep(250);
            }

            Console.WriteLine("　[終了]\n");
        }

        //============================================================
        // 3) MP3：フォルダ1のトラック再生,リピート再生
        //============================================================
        private static void RunMp3Playlist(Mmp.Core.MmpClient mmp)
        {
            Console.WriteLine("３.ＭＰ３再生（ DFPlayer ）");

            Console.WriteLine("　・音量 → 20 : "    + mmp.Audio.Volume(1, 20)          );
            Console.WriteLine("　・ループ → OFF : " + mmp.Audio.Play.SetLoop(1, false) );

            Console.WriteLine("　・再生");
            for (int track = 1; track <= 3; track++)
            {
                Console.Write("　　→ F=1,T=" + track + " : " + mmp.Audio.Play.Start(1, 1, track) );
                Console.WriteLine(" : 状況 = "                + mmp.Audio.Read.State(1)           );
                Thread.Sleep(3000);
            }

            Console.Write    ("　・停止 : " + mmp.Audio.Play.Stop(1));
            Console.WriteLine(" : 状況 = "  + mmp.Audio.Read.State(1));

            Console.WriteLine("　・再生 → F=2,T=102 : " + mmp.Audio.Play.Start(1, 2, 102) );
            Console.Write    ("　・ループ → ON : "      + mmp.Audio.Play.SetLoop(1, true) );
            Console.WriteLine(" : 状況 = "               + mmp.Audio.Read.State(1)         );
            Thread.Sleep(10000);

            Console.Write    ("　・停止 : "  + mmp.Audio.Play.Stop(1)  );
            Console.WriteLine(" : 状況 = "   + mmp.Audio.Read.State(1) );

            Console.WriteLine("　[終了]\n");
        }

        //============================================================
        // 4) MP3 制御：状態取得（DST）/一時停止/再開/停止/EQ/音量
        //============================================================
        private static void RunMp3Control(Mmp.Core.MmpClient mmp)
        {
            Console.WriteLine("４.ＭＰ３制御（ DFPlayer ）");

            Console.WriteLine("　・音量 → 20 : "      + mmp.Audio.Volume(1, 20)          );
            Console.WriteLine("　・再生 → F=4,T=1 : " + mmp.Audio.Play.Start(1, 4, 1)    );
            Console.WriteLine("　・ループ → OFF : "   + mmp.Audio.Play.SetLoop(1, false) );

            Console.WriteLine("　・参照");
            Console.WriteLine("　　← 状況         = " + mmp.Audio.Read.State(1)  );
            Console.WriteLine("　　← 音量         = " + mmp.Audio.Read.Volume(1)     );
            Console.WriteLine("　　← イコライザ   = " + mmp.Audio.Read.Eq(1)         );
            Console.WriteLine("　　← 総ファイル数 = " + mmp.Audio.Read.FileCounts(1) );
            Console.WriteLine("　　← 現在ファイル = " + mmp.Audio.Read.FileNumber(1) );

            Console.Write    ("　・一時停止 : "        + mmp.Audio.Play.Pause(1) );
            Console.WriteLine(" : 状況 = "             + mmp.Audio.Read.State(1) );
            Thread.Sleep(2000);

            Console.Write    ("　・再開 : "            + mmp.Audio.Play.Resume(1) );
            Console.WriteLine(" : 状況 = "             + mmp.Audio.Read.State(1)  );

            Console.WriteLine("　・イコライザー ");
            for (int cnt_eq = 0; cnt_eq <= 5; cnt_eq++)
            {
                Console.WriteLine("　　→ " + cnt_eq + " : "
                                                       + mmp.Audio.SetEq(1, cnt_eq)     );
                Thread.Sleep(3000);
            }

            Console.WriteLine("　・音量");
            for (int cnt_vol = 0; cnt_vol <= 30; cnt_vol += 5)
            {
                Console.WriteLine("　　→ " + cnt_vol + " : "
                                                       + mmp.Audio.Volume(1, cnt_vol)   );
                Thread.Sleep(1000);
            }

            Console.Write    ("　・停止 : "            + mmp.Audio.Play.Stop(1)  );
            Console.WriteLine(" : 状況 = "             + mmp.Audio.Read.State(1) );

            Console.WriteLine("　[終了]\n");
        }

        //============================================================
        // 5) PWM 生値：ch0=180度型、ch15=連続回転型
        //============================================================
        private static void RunPwm(Mmp.Core.MmpClient mmp, bool mode)
        {
            Console.WriteLine(
                mode ? "５.ＰＷＭ" : "６.Ｉ２Ｃ"
                + "（ PCA9685：サーボモータ180度型,連続回転型 ）"
            );

            const int SERVO_MIN     = 150; // CA9685 12bitの生値（例: 150）
            const int SERVO_MAX     = 600; // 同上              （例: 600）
            const int SERVO_MID     = (SERVO_MIN + SERVO_MAX) / 2;
            const int OffsetMax360  = 60;
            const int STEPS         = 80;
            const int STEP          = 2;
            const int STEP_DELAY_MS = 0;
            const int CH_180        = 0;
            const int CH_360        = 15;
            const int PCA_ADDR      = 0x40;

            void RunI2C(int ch, int ticks)
            {
                int base_reg = 0x06 + 4 * ch;
                mmp.I2c.Write(PCA_ADDR, base_reg + 2, (ticks)      & 0xFF);
                mmp.I2c.Write(PCA_ADDR, base_reg + 3, (ticks >> 8) & 0x0F);
            }

            Console.WriteLine("　・初期位置");
            if (mode)
            {
                mmp.Pwm.Out(CH_180, SERVO_MID);
                mmp.Pwm.Out(CH_360, SERVO_MID);
            }
            else
            {
                RunI2C(CH_180, SERVO_MID);
                RunI2C(CH_360, SERVO_MID);
            }
            Thread.Sleep(300);

            Console.WriteLine("　・正転,加速");
            for (int i = 0; i <= STEPS; i+=STEP)
            {
                int pwm180 = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i  / STEPS;
                int pwm360 = SERVO_MID + (OffsetMax360           * i) / STEPS;
                if (mode)
                {
                    mmp.Pwm.Out(CH_180, pwm180);
                    mmp.Pwm.Out(CH_360, pwm360);
                }
                else
                {
                    RunI2C(CH_180, pwm180);
                    RunI2C(CH_360, pwm360);
                }
                Thread.Sleep(STEP_DELAY_MS);
            }

            Console.WriteLine("　・逆転,減速");
            for (int i = STEPS; i >= 0; i-=STEP)
            {
                int pwm180 = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i   / STEPS;
                int pwm360 = SERVO_MID + (OffsetMax360           * i ) / STEPS;
                if (mode)
                {
                    mmp.Pwm.Out(CH_180, pwm180);
                    mmp.Pwm.Out(CH_360, pwm360);
                }
                else
                {
                    RunI2C(CH_180, pwm180);
                    RunI2C(CH_360, pwm360);
                }
                Thread.Sleep(STEP_DELAY_MS);
            }

            Console.WriteLine("　・初期位置");
            if (mode)
            {
                mmp.Pwm.Out(CH_180, SERVO_MID);
                mmp.Pwm.Out(CH_360, SERVO_MID);
            }
            else
            {
                RunI2C(CH_180, SERVO_MID);
                RunI2C(CH_360, SERVO_MID);
            }

            Console.WriteLine("　[終了]\n");
        }

        //============================================================
        // ヘルパ
        //============================================================
        static string int2strOnOff(int source) { return (source == 0 ? "ON" : "OFF"); }
    }
}
