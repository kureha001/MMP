# -*- coding: utf-8 -*-
import time

#============================================================
# 出力文字ヘルパ
#============================================================
def tf(b): return "True" if b else "False"


#============================================================
# 0) 基本情報(バージョン/PCA9685/DFPlayer)
#============================================================
def RunInfo(mmp):

    print("０.システム情報")
    print("　・バージョン  : {}".format(mmp.Info.Version()))
    print("　・PCA9685 [0] : 0x{:04X}".format(mmp.Info.Dev.Pwm(0)))
    print("　・DFPlayer[1] : 0x{:04X}".format(mmp.Info.Dev.Audio(1)))
    print("　[終了]\n")


#============================================================
# 1) アナログ入力(HC4067)
#============================================================
def RunAnalog(mmp):

    print("１.アナログ入力（ HC4067：JoyPad1,2 ）")

    ok = mmp.Analog.Configure(2, 4)
    print("　・アクセス範囲指定 → [2,4]  : {}".format(tf(ok)))
    if not ok:
        print("　 <<中断>>\n")
        return

    ok = mmp.Analog.Update()
    print("　・アナログ値をバッファに格納 : {}".format(tf(ok)))
    if not ok:
        print("　 <<中断>>\n")
        return

    print("　・バッファを参照")
    for x in range(0, 2):
        print("　　JoyPad[{}]".format(x + 1))
        for y in range(0, 4):
            print("　　　← [{}] = {}".format(y, mmp.Analog.Read(x, y)))

    print("　[終了]\n")


#============================================================
# 2) デジタル入出力
#============================================================
def RunDigital(mmp):
    print("２.デジタル入出力（ GPIO ）")
    print("　・入力")
    print("　　←[2] = {}".format("ON" if mmp.Digital.In(2) == 0 else "OFF"))
    print("　　←[6] = {}".format("ON" if mmp.Digital.In(6) == 0 else "OFF"))
    print("　　←[7] = {}".format("ON" if mmp.Digital.In(7) == 0 else "OFF"))

    print("　・出力[3]")
    for _ in range(6):
        print("　　→ HIGH : {}".format(tf(mmp.Digital.Out(3, 1)))); time.sleep(0.5)
        print("　　→ LOW  : {}".format(tf(mmp.Digital.Out(3, 0)))); time.sleep(0.5)
    print("　[終了]\n")


#============================================================
# 3) MP3：フォルダ1のトラック再生,リピート再生
#============================================================
def RunMp3Playlist(mmp):

    print("３.ＭＰ３再生（ DFPlayer ）")

    print("　・音量 → 20 : {}".format(tf(mmp.Audio.Volume(1, 20))))
    print("　・ループ → OFF : {}".format(tf(mmp.Audio.Play.SetLoop(1, 0))))

    print("　・再生")
    for track in range(1, 4):

        ok = mmp.Audio.Play.FolderTrack(1, 1, track)
        print("　　→ F=1,T={} : {}".format(track, tf(ok)))
        time.sleep(0.1)

        print("　　　: 状況 = {}".format(mmp.Audio.Read.PlayState(1)))
        time.sleep(3.0)

    ok = mmp.Audio.Play.Stop(1)
    time.sleep(0.1)
    print("　・停止 : {} : 状況 = {}".format(tf(ok), mmp.Audio.Read.PlayState(1)))

    ok = mmp.Audio.Play.FolderTrack(1, 2, 102)
    print("　・再生 → F=2,T=102 : {}".format(tf(ok)))

    ok = mmp.Audio.Play.SetLoop(1, 1)
    time.sleep(0.1)
    print("　・ループ → ON : {} : 状況 = {}".format(tf(ok), mmp.Audio.Read.PlayState(1)))
    time.sleep(10.0)

    ok = mmp.Audio.Play.Stop(1)
    time.sleep(0.1)
    print("　・停止 : {} : 状況 = {}".format(tf(ok), mmp.Audio.Read.PlayState(1)))
    print("　[終了]\n")


#============================================================
# 4) MP3 制御：状態取得/一時停止/再開/停止/EQ/音量
#============================================================
def RunMp3Control(mmp):

    print("４.ＭＰ３制御（ DFPlayer ）")

    print("　・音量 → 20 : {}".format(tf(mmp.Audio.Volume(1, 20))))
    print("　・再生 → F=4,T=1 : {}".format(tf(mmp.Audio.Play.FolderTrack(1, 4, 1))))
    print("　・ループ → OFF : {}".format(tf(mmp.Audio.Play.SetLoop(1, 0))))

    print("　・参照")
    print("　　← 状況         = {}".format(mmp.Audio.Read.PlayState(1)))
    print("　　← 音量         = {}".format(mmp.Audio.Read.Volume(1)))
    print("　　← イコライザ   = {}".format(mmp.Audio.Read.Eq(1)))
    print("　　← 総ファイル数 = {}".format(mmp.Audio.Read.FileCounts(1)))
    print("　　← 現在ファイル = {}".format(mmp.Audio.Read.CurrentFileNumber(1)))

    ok = mmp.Audio.Play.Pause(1); time.sleep(0.1)
    print("　・一時停止 : {} : 状況 = {}".format(tf(ok), mmp.Audio.Read.PlayState(1)))
    time.sleep(2.0)

    ok = mmp.Audio.Play.Resume(1); time.sleep(0.1)
    print("　・再開 : {} : 状況 = {}".format(tf(ok), mmp.Audio.Read.PlayState(1)))

    print("　・イコライザー")
    for mode in range(0, 6):
        print("　　→ {} : {}".format(mode, tf(mmp.Audio.SetEq(1, mode))))
        time.sleep(3.0)

    print("　・音量")
    for v in range(0, 31, 5):
        print("　　→ {} : {}".format(v, tf(mmp.Audio.Volume(1, v))))
        time.sleep(1.0)

    ok = mmp.Audio.Play.Stop(1); time.sleep(0.1)
    print("　・停止 : {} : 状況 = {}".format(tf(ok), mmp.Audio.Read.PlayState(1)))
    print("　[終了]\n")


#============================================================
# 5) PWM 生値：ch0=180度型、ch15=連続回転型
#============================================================
def RunPwmByValue(mmp):

    print("５.ＰＷＭ（ PCA9685：サーボモータ180度型,連続回転型 ）")

    SERVO_MIN = 150
    SERVO_MAX = 600
    SERVO_MID = (SERVO_MIN + SERVO_MAX) // 2
    chAngle = 0
    chRot   = 15

    print("　・初期位置")
    mmp.Pwm.Out(chAngle, SERVO_MID)
    mmp.Pwm.Out(chRot,   SERVO_MID)
    time.sleep(0.3)

    steps        = 80
    delayMs      = 0.02
    rotOffsetMax = 60

    print("　・正転,加速")
    for i in range(0, steps + 1):
        pwmAngle = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // steps
        pwmRot   = SERVO_MID + (rotOffsetMax * i) // steps
        mmp.Pwm.Out(chAngle, pwmAngle)
        mmp.Pwm.Out(chRot,   pwmRot)
        time.sleep(delayMs)

    print("　・逆転,減速")
    for i in range(steps, -1, -1):
        pwmAngle = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // steps
        pwmRot   = SERVO_MID + (rotOffsetMax * i) // steps
        mmp.Pwm.Out(chAngle, pwmAngle)
        mmp.Pwm.Out(chRot,   pwmRot)
        time.sleep(delayMs)

    print("　・初期位置")
    mmp.Pwm.Out(chRot,   SERVO_MID)
    mmp.Pwm.Out(chAngle, SERVO_MID)
    print("　[終了]\n")


#============================================================
# メイン（共通）
#============================================================
def run_all(mmp, TARGET):

    print("<< ＭＭＰライブラリ for {} >>\n".format(TARGET))

    print("接続中...", end="")

    if not mmp.ConnectAutoBaud():
        print("ＭＭＰとの接続に失敗しました...")
        return

    print("成功")
    try:
        print("自動検出 {} bps\n".format(mmp.Settings.BaudRate))
    except:
        pass

    print("＝＝＝ ＭＭＰ ＡＰＩテスト［開始］＝＝＝\n")
    RunInfo(mmp)
    RunAnalog(mmp)
    RunDigital(mmp)
    RunMp3Playlist(mmp)
    RunMp3Control(mmp)
    RunPwmByValue(mmp)
    print("＝＝＝ ＭＭＰ ＡＰＩテスト［終了］＝＝＝")
