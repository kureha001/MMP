using System;
using System.Threading;

namespace Mmp.Tool
{
    internal static class Program
    {
        static int Main(string[] args)
        {
            string port = args.Length > 0 ? args[0] : null;
            Console.WriteLine("[MMP Tests] Port=" + (port ?? "(auto)"));

            try
            {
                using (var mmp = new Mmp.Core.MmpClient())
                {
                    mmp.Settings.BaudRate = 115200;
                    mmp.Settings.TimeoutAudio = 800;

                    bool connected = (port != null) ? mmp.Connect(port) : mmp.Connect();
                    if (!connected || !mmp.IsOpen)
                    {
                        Console.WriteLine("Connect failed.");
                        return 1;
                    }
                    Console.WriteLine("Connected: " + mmp.PortName + "  VER=" + mmp.Info.Version());

                    // === 必要なテストだけコメントを外して実行してください ===

                    // 0) 基本情報
                    RunInfo(mmp);

                    // 1) アナログ入力
                    //RunAnalog(mmp);

                    // 2) MP3 再生（フォルダ=1、トラック=1..3を3秒おき）
                    RunMp3Playlist(mmp);

                    // 3) MP3 制御（状態取得/一時停止/再開/停止/EQ/音量）
                    //RunMp3Control(mmp);

                    // 4) PWM 生値テスト（ch0=180度型、ch15=連続回転）
                    //RunPwmByValue(mmp);
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
            Console.WriteLine("VER: " + mmp.Info.Version());
            Console.WriteLine("PWX(0): 0x{0:X4}", mmp.Info.Dev.Pwm(0));
            Console.WriteLine("DPX(1): 0x{0:X4}", mmp.Info.Dev.Audio(1));
        }

        //============================================================
        // 1) アナログ入力
        //============================================================
        private static void RunAnalog(Mmp.Core.MmpClient mmp)
        {
            if (!mmp.Analog.Configure(1, 1))
            {
                Console.WriteLine("ANS failed");
                return;
            }
            if (!mmp.Analog.Update())
            {
                Console.WriteLine("ANU failed");
                return;
            }
            int val = mmp.Analog.Read(0, 0);
            Console.WriteLine("AN(0,0): " + val);
        }

        //============================================================
        // 2) MP3：フォルダ1のトラック1～3を3秒おきに再生
        //============================================================
        private static void RunMp3Playlist(Mmp.Core.MmpClient mmp)
        {
            Console.WriteLine("MP3 playlist: device=1, folder=1, tracks=1..3");
            mmp.Audio.Volume(1, 20); // 音量（0～30）

            for (int track = 1; track <= 3; track++)
            {
                Console.WriteLine("Play F=1 T=" + track);
                string resp = mmp.Audio.PlayFolderTrack(1, 1, track);
                Console.WriteLine("DIR resp: " + resp);
                Thread.Sleep(3000);
            }

            mmp.Audio.Stop(1);
        }

        //============================================================
        // 3) MP3 制御：状態取得（DST）/一時停止/再開/停止/EQ/音量
        //============================================================
        private static void RunMp3Control(Mmp.Core.MmpClient mmp)
        {
            Console.WriteLine("MP3 control:");

            mmp.Audio.Volume(1, 20);
            string dir = mmp.Audio.PlayFolderTrack(1, 1, 1);
            Console.WriteLine("DIR resp: " + dir);

            Console.WriteLine("DST State: " + mmp.Audio.ReadPlayState(1));
            Console.WriteLine("DST Volume: " + mmp.Audio.ReadVolume(1));
            Console.WriteLine("DST EQ: " + mmp.Audio.ReadEq(1));
            Console.WriteLine("DST FileCounts: " + mmp.Audio.ReadFileCounts(1));
            Console.WriteLine("DST CurrentFile: " + mmp.Audio.ReadCurrentFileNumber(1));

            bool ok = mmp.Audio.Pause(1);
            Console.WriteLine("DPA: " + ok);
            Thread.Sleep(500);
            Console.WriteLine("State after pause: " + mmp.Audio.ReadPlayState(1));

            ok = mmp.Audio.Resume(1);
            Console.WriteLine("DPR: " + ok);
            Thread.Sleep(500);
            Console.WriteLine("State after resume: " + mmp.Audio.ReadPlayState(1));

            for (int mode = 0; mode <= 5; mode++)
            {
                bool okEq = mmp.Audio.SetEq(1, mode);
                Console.WriteLine("DEF(" + mode + "): " + okEq);
                Thread.Sleep(200);
            }

            for (int v = 15; v <= 25; v += 5)
            {
                bool okVol = mmp.Audio.Volume(1, v);
                Console.WriteLine("VOL(" + v + "): " + okVol);
                Thread.Sleep(300);
            }

            ok = mmp.Audio.Stop(1);
            Console.WriteLine("DSP: " + ok);
            Thread.Sleep(300);
            Console.WriteLine("State after stop: " + mmp.Audio.ReadPlayState(1));
        }

        //============================================================
        // 4) PWM 生値：ch0=180度型、ch15=連続回転
        //============================================================
        private static void RunPwmByValue(Mmp.Core.MmpClient mmp)
        {
            Console.WriteLine("Start servo motion (VALUE)...");
            const int SERVO_MIN = 150;
            const int SERVO_MAX = 600;
            const int SERVO_MID = (SERVO_MIN + SERVO_MAX) / 2;

            int chAngle = 0;
            int chRot = 15;

            mmp.Pwm.Out(chAngle, SERVO_MID);
            mmp.Pwm.Out(chRot, SERVO_MID);
            Thread.Sleep(300);

            int steps = 80;
            int delayMs = 20;
            int rotOffsetMax = 60;

            for (int i = 0; i <= steps; i++)
            {
                int pwmAngle = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i / steps;
                int pwmRot = SERVO_MID + (rotOffsetMax * i) / steps;
                mmp.Pwm.Out(chAngle, pwmAngle);
                mmp.Pwm.Out(chRot, pwmRot);
                Thread.Sleep(delayMs);
            }

            for (int i = steps; i >= 0; i--)
            {
                int pwmAngle = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i / steps;
                int pwmRot = SERVO_MID + (rotOffsetMax * i) / steps;
                mmp.Pwm.Out(chAngle, pwmAngle);
                mmp.Pwm.Out(chRot, pwmRot);
                Thread.Sleep(delayMs);
            }

            mmp.Pwm.Out(chRot, SERVO_MID);
            mmp.Pwm.Out(chAngle, SERVO_MID);
            Console.WriteLine("PWM test done.");
        }
    }
}
