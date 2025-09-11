# -*- coding: utf-8 -*-
import time
import MMP

#============================================================
# メイン（共通）
#============================================================
def main():

    if not MMP.通信接続(): return

    print("\n＝＝＝ ＭＭＰ ＡＰＩテスト［開始］＝＝＝\n")
    RunDigital()         # デジタル入出力
    RunMp3Playlist()     # MP3プレイヤー(基本)
    RunMp3Control()      # MP3プレイヤー(制御)
    RunPwmByValue()      # PWM出力
    RunI2cServoSweep()   # I2C→PCA9685 直接制御
    print("＝＝＝ ＭＭＰ ＡＰＩテスト［終了］＝＝＝")

#============================================================
# 出力文字ヘルパ
#============================================================
def tf(b): return "True" if b else "False"


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
#============================================================
def RunPwmByValue():

    print("５.ＰＷＭ（ PCA9685：サーボモータ180度型,連続回転型 ）")

    SERVO_MIN = 150
    SERVO_MAX = 600
    SERVO_MID = (SERVO_MIN + SERVO_MAX) // 2
    chAngle = 0
    chRot   = 15

    print("　・初期位置")
    MMP.接続.Pwm.Out(chAngle, SERVO_MID)
    MMP.接続.Pwm.Out(chRot,   SERVO_MID)
    time.sleep(0.3)

    rotOffsetMax = 60

    print("　・正転,加速")
    for i in range(0, STEPS + 1):
        pwmAngle = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        pwmRot   = SERVO_MID + (rotOffsetMax * i) // STEPS
        MMP.接続.Pwm.Out(chAngle, pwmAngle)
        MMP.接続.Pwm.Out(chRot,   pwmRot)
        time.sleep(STEP_DELAY_S)

    print("　・逆転,減速")
    for i in range(STEPS, -1, -1):
        pwmAngle = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        pwmRot   = SERVO_MID + (rotOffsetMax * i) // STEPS
        MMP.接続.Pwm.Out(chAngle, pwmAngle)
        MMP.接続.Pwm.Out(chRot,   pwmRot)
        time.sleep(STEP_DELAY_S)

    print("　・初期位置")
    MMP.接続.Pwm.Out(chRot,   SERVO_MID)
    MMP.接続.Pwm.Out(chAngle, SERVO_MID)
    print("　[終了]\n")


#============================================================
# 6) I2C：サーボスイープ（PCA9685 レジスタ直書き）
#------------------------------------------------------------
#  LEDn の ON=0 / OFF=val でデューティ設定
#  LED0_ON_L=0x06 からチャネル毎に 4 レジスタ
#------------------------------------------------------------
#  base = 0x06 + 4*ch
#   [base+0]=ON_L,
#   [base+1]=ON_H,
#   [base+2]=OFF_L,
#   [base+3]=OFF_H
#============================================================
PCA9685_ADDR  = 0x40
CHANNEL_SERVO = 0       # テストする PCA9685 のチャネル
SERVO_MIN     = 150     # PCA9685 12bitの生値（例: 150）
SERVO_MAX     = 600     # 同上（例: 600）
SERVO_MID     = (SERVO_MIN + SERVO_MAX) // 2
STEPS         = 80
STEP_DELAY_S  = 0.02
#------------------------------------------------------------
def _pca9685_set_pwm(接続, ch: int, value_0_4095: int) -> bool:

    base = 0x06 + 4 * ch
    on_l, on_h = 0x00, 0x00
    off = max(0, min(4095, int(value_0_4095)))
    off_l = (off & 0xFF)
    off_h = ((off >> 8) & 0x0F)  # 上位4bitのみ

    ok  = MMP.接続.I2c.Write(PCA9685_ADDR, base + 0, on_l)   # ON_L
    ok &= MMP.接続.I2c.Write(PCA9685_ADDR, base + 1, on_h)   # ON_H
    ok &= MMP.接続.I2c.Write(PCA9685_ADDR, base + 2, off_l)  # OFF_L
    ok &= MMP.接続.I2c.Write(PCA9685_ADDR, base + 3, off_h)  # OFF_H
    return ok
#------------------------------------------------------------
def _pca9685_get_pwm(接続, ch: int) -> int:
    base = 0x06 + 4 * ch
    off_l = MMP.接続.I2c.Read(PCA9685_ADDR, base + 2)
    off_h = MMP.接続.I2c.Read(PCA9685_ADDR, base + 3) & 0x0F
    if off_l < 0 or off_h < 0:
        return -1
    return (off_h << 8) | off_l
#------------------------------------------------------------
def RunI2cServoSweep():

    print("６.I2C（ PCA9685：サーボスイープ ）")

    print("　・初期位置")
    if not _pca9685_set_pwm(MMP.接続, CHANNEL_SERVO, SERVO_MID):
        print("　[Err:中断]\n")
        return
    time.sleep(0.3)

    print("　・スイープ(往路)")
    for i in range(STEPS + 1):
        v = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        if not _pca9685_set_pwm(MMP.接続, CHANNEL_SERVO, v):
            print("　[Err:中断]\n")
            return
        time.sleep(STEP_DELAY_S)

    print("　・スイープ(復路)")
    for i in range(STEPS, -1, -1):
        v = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        if not _pca9685_set_pwm(MMP.接続, CHANNEL_SERVO, v):
            print("　[Err:中断]\n")
            return
        time.sleep(STEP_DELAY_S)

    # 読み出し確認（最後に設定した値）
    last = _pca9685_get_pwm(MMP.接続, CHANNEL_SERVO)
    print(f"　・現在の OFF 値 = {last}")

    print("　・初期位置")
    _pca9685_set_pwm(MMP.接続, CHANNEL_SERVO, SERVO_MID)
    if not _pca9685_set_pwm(MMP.接続, CHANNEL_SERVO, SERVO_MID):
        print("　[Err:中断]\n")
        return

    print("　[終了]\n")


#======================================================
if __name__ == "__main__": main()