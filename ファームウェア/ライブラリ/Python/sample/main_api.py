# -*- coding: utf-8 -*-
#============================================================
# ＭＭＰコマンド テスト（ＡＰＩ経由でで実行）
#============================================================
import time
import MMP

#============================================================
# メイン（共通）
#============================================================
def main():

    if not MMP.通信接続(): return

    print("\n＝＝＝ ＭＭＰ ＡＰＩテスト［開始］＝＝＝\n")
    #RunDigital()        # デジタル入出力
    #RunMp3Playlist()    # MP3プレイヤー(基本)
    #RunMp3Control()     # MP3プレイヤー(制御)
    #RunPwm(True)        # PWM出力
    #RunPwm(False)       # I2C→PCA9685 直接制御
    print("＝＝＝ ＭＭＰ ＡＰＩテスト［終了］＝＝＝")

#============================================================
# 出力文字ヘルパ
#============================================================
def tf(b): return "True" if b>0 else "False"


#============================================================
# 1) アナログ入力(HC4067)
#============================================================
def RunAnalog():

    print("１.アナログ入力（ HC4067：JoyPad1,2 ）")

    ok = MMP.接続.Analog.Configure(2, 4)
    print("　・アクセス範囲指定 → [2,4]  : {}".format(tf(ok)))
    if not ok:
        print("　 <<中断>>\n")
        return

    ok = MMP.接続.Analog.Update()
    print("　・アナログ値をバッファに格納 : {}".format(tf(ok)))
    if not ok:
        print("　 <<中断>>\n")
        return

    print("　・バッファを参照")
    for x in range(0, 2):
        print("　　JoyPad[{}]".format(x + 1))
        for y in range(0, 4):
            print("　　　← [{}] = {}".format(y, MMP.接続.Analog.Read(x, y)))

    print("　[終了]\n")


#============================================================
# 2) デジタル入出力
#============================================================
def RunDigital():
    print("２.デジタル入出力（ GPIO ）")
    print("　・入力")
    print("　　←[2] = {}".format("ON" if MMP.接続.Digital.In(2) == 0 else "OFF"))
    print("　　←[6] = {}".format("ON" if MMP.接続.Digital.In(6) == 0 else "OFF"))
    print("　　←[7] = {}".format("ON" if MMP.接続.Digital.In(7) == 0 else "OFF"))

    print("　・出力[3]")
    for _ in range(6):
        print("　　→ HIGH : {}".format(tf(MMP.接続.Digital.Out(3, 1)))); time.sleep(0.5)
        print("　　→ LOW  : {}".format(tf(MMP.接続.Digital.Out(3, 0)))); time.sleep(0.5)
    print("　[終了]\n")


#============================================================
# 3) MP3：フォルダ1のトラック再生,リピート再生
#============================================================
def RunMp3Playlist():

    print("３.ＭＰ３再生（ DFPlayer ）")

    print("　・音量 → 20 : {}".format(tf(MMP.接続.Audio.Volume(1, 20))))
    print("　・ループ → OFF : {}".format(tf(MMP.接続.Audio.Play.SetLoop(1, 0))))

    print("　・再生")
    for track in range(1, 4):

        ok = MMP.接続.Audio.Play.Start(1, 1, track)
        print("　　→ F=1,T={} : {}：状況 = {}".format(track, tf(ok), MMP.接続.Audio.Read.State(1)))
        time.sleep(3.0)

    ok = MMP.接続.Audio.Play.Stop(1)
    print("　・停止 : {} : 状況 = {}".format(tf(ok), MMP.接続.Audio.Read.State(1)))

    ok = MMP.接続.Audio.Play.Start(1, 2, 102)
    print("　・再生 → F=2,T=102 : {}".format(tf(ok)))

    ok = MMP.接続.Audio.Play.SetLoop(1, 1)
    print("　・ループ → ON : {} : 状況 = {}".format(tf(ok), MMP.接続.Audio.Read.State(1)))
    time.sleep(10.0)

    ok = MMP.接続.Audio.Play.Stop(1)
    print("　・停止 : {} : 状況 = {}".format(tf(ok), MMP.接続.Audio.Read.State(1)))
    print("　[終了]\n")


#============================================================
# 4) MP3 制御：状態取得/一時停止/再開/停止/EQ/音量
#============================================================
def RunMp3Control():

    print("４.ＭＰ３制御（ DFPlayer ）")

    print("　・音量 → 20 : {}".format(tf(MMP.接続.Audio.Volume(1, 20))))
    print("　・再生 → F=4,T=1 : {}".format(tf(MMP.接続.Audio.Play.Start(1, 4, 1))))
    print("　・ループ → OFF : {}".format(tf(MMP.接続.Audio.Play.SetLoop(1, 0))))

    print("　・参照")
    print("　　← 状況         = {}".format(MMP.接続.Audio.Read.State(1)))
    print("　　← 音量         = {}".format(MMP.接続.Audio.Read.Volume(1)))
    print("　　← イコライザ   = {}".format(MMP.接続.Audio.Read.Eq(1)))
    print("　　← 総ファイル数 = {}".format(MMP.接続.Audio.Read.FileCounts(1)))
    print("　　← 現在ファイル = {}".format(MMP.接続.Audio.Read.FileNumber(1)))

    ok = MMP.接続.Audio.Play.Pause(1)
    print("　・一時停止 : {} : 状況 = {}".format(tf(ok), MMP.接続.Audio.Read.State(1)))
    time.sleep(2.0)

    ok = MMP.接続.Audio.Play.Resume(1)
    print("　・再開 : {} : 状況 = {}".format(tf(ok), MMP.接続.Audio.Read.State(1)))

    print("　・イコライザー")
    for mode in range(0, 6):
        print("　　→ {} : {}".format(mode, tf(MMP.接続.Audio.SetEq(1, mode))))
        time.sleep(3.0)

    print("　・音量")
    for v in range(0, 31, 5):
        print("　　→ {} : {}".format(v, tf(MMP.接続.Audio.Volume(1, v))))
        time.sleep(1.0)

    ok = MMP.接続.Audio.Play.Stop(1)
    print("　・停止 : {} : 状況 = {}".format(tf(ok), MMP.接続.Audio.Read.State(1)))
    print("　[終了]\n")


#============================================================
# 5) PWM 生値：ch0=180度型、ch15=連続回転型
# 6) I2C：サーボスイープ（PCA9685 レジスタ直書き）
#============================================================
def RunPwm(mode):

    Title = ("５.ＰＷＭ") if mode else ("６.Ｉ２Ｃ")
    print(f"{Title}（ PCA9685：サーボモータ180度型,連続回転型 ）")

    SERVO_MIN    = 150 # PCA9685 12bitの生値（例: 150）
    SERVO_MAX    = 600 # 同上               （例: 600）
    SERVO_MID    = (SERVO_MIN + SERVO_MAX) // 2
    OffsetMax360 = 60
    STEPS        = 80
    STEP         = 8
    STEP_DELAY_S = 0
    CH_180       = 0
    CH_360       = 15
    PCA_ADDR     = 0x40

    def RunI2C(ch, ticks):
        base_reg  = 0x06 + 4 * ch
        MMP.接続.I2c.Write(PCA_ADDR, base_reg + 2, (ticks     ) & 0xFF)
        MMP.接続.I2c.Write(PCA_ADDR, base_reg + 3, (ticks >> 8) & 0x0F)

    print("　・初期位置")
    if mode:
        MMP.接続.Pwm.Out(CH_180, SERVO_MID)
        MMP.接続.Pwm.Out(CH_360, SERVO_MID)
    else:
        RunI2C(CH_180, SERVO_MID)
        RunI2C(CH_360, SERVO_MID)
    time.sleep(0.3)

    print("　・正転,加速")
    for i in range(0, STEPS + 1,STEP):
        pwm180 = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        pwm360 = SERVO_MID + (OffsetMax360 * i)          // STEPS
        if mode:
            MMP.接続.Pwm.Out(CH_180, pwm180)
            MMP.接続.Pwm.Out(CH_360, pwm360)
        else:
            RunI2C(CH_180, pwm180)
            RunI2C(CH_360, pwm360)
        time.sleep(STEP_DELAY_S)

    print("　・逆転,減速")
    for i in range(STEPS, -1, -STEP):
        pwm180 = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        pwm360 = SERVO_MID + (OffsetMax360 * i)          // STEPS
        if mode:
            MMP.接続.Pwm.Out(CH_180, pwm180)
            MMP.接続.Pwm.Out(CH_360, pwm360)
        else:
            RunI2C(CH_180, pwm180)
            RunI2C(CH_360, pwm360)
        time.sleep(STEP_DELAY_S)

    print("　・初期位置")
    if mode:
        MMP.接続.Pwm.Out(CH_180, SERVO_MID)
        MMP.接続.Pwm.Out(CH_360, SERVO_MID)
    else:
        RunI2C(CH_180, SERVO_MID)
        RunI2C(CH_360, SERVO_MID)

    print("　[終了]\n")

#======================================================
if __name__ == "__main__": main()