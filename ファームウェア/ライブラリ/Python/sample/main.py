# -*- coding: utf-8 -*-
# filename : main.py
#============================================================
# ＭＭＰコマンド テスト（通常ライブラリで実行）
# バージョン：0.5
#------------------------------------------------------------
# シリアル通信・TCPブリッジに対応
#============================================================
import time
import MMP

#============================================================
# テスト構成
# `USE_*` の ON/OFF で切替る
# `USE_*` 意外は 各自の環境に合わせる
#============================================================
#(0) UART番号（ USB=0, UART1=1, UART2=2, ...）
USE_UART_NO         = 0

#(1) 直結：シリアル接続（プラットフォーム自動）
USE_SERIAL_AUTO     = False

#(2) TCPブリッジ：ser2net
USE_TCP             = True
TCP_HOST_PC         = "192.168.2.124"   # LANの場合
#TCP_HOST_PC         = "203.141.144.142" #WANの場合
TCP_PORT_PC         = 5331

#(3) TCPブリッジ：usb4a(Pydroid)
USE_USB4A_DIRECT    = False
USB4A_INDEX         = 0

#(4) TCP共通：read_one_char の待ち秒（?timeout=） 
TCP_TIMEOUT_S       = 0.2   # ネットワーク品質に応じて調整

#============================================================
# 互換シム（MMP.通信接続 / MMP.接続 を既存通り使えるように）
#============================================================
def 引数取得():

    # 1) シリアル直結（自動）
    if USE_SERIAL_AUTO      : return "auto"

    # 2) TCPブリッジ
    if USE_TCP:
        return f"tcp://{TCP_HOST_PC}:{TCP_PORT_PC}?timeout={TCP_TIMEOUT_S}"

    # 3) usb4a (pydroid専用)
    if USE_USB4A_DIRECT   : return f"usb4a://{USB4A_INDEX}"

    # 何も選ばれていなければ既定はシリアル自動
    return "auto"


#============================================================
# メイン（共通）
#============================================================
def main():

    if not MMP.通信接続(引数取得()): return

    print("\n＝＝＝ ＭＭＰ ＡＰＩテスト［開始］＝＝＝\n")
    # 実施するテストだけコメントを外してください（複数可）
    #RunAnalog()         # アナログ入力
    #RunDigital()        # デジタル入出力
    #RunMp3Playlist()    # MP3プレイヤー(基本)
    #RunMp3Control()     # MP3プレイヤー(制御)
    RunPwm(True)        # PWM出力
    RunPwm_Angle()      # PWM出力(角度指定)
    RunPwm_Rotation()   # PWM出力(回転指定)
    #RunPwm(False)       # I2C→PCA9685 直接制御
    print("＝＝＝ ＭＭＰ ＡＰＩテスト［終了］＝＝＝")


#============================================================
# 出力文字ヘルパ
#============================================================
def tf(b): 
    return "True" if b>0 else "False"


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

    print("【基本】")
    print(f"　・音量   → 20  : {MMP.接続.MP3.Set.Volume(1, 20)}")
    print(f"　・ループ → OFF : {MMP.接続.MP3.Track.Loop(1, 0 )}")

    print("　・再生")
    for track in range(1, 4):
        print(f"　　　 → F=1,T={track} : {MMP.接続.MP3.Track.Play(1, 1, track)}")
        time.sleep(3.0)

    print(f"　・停止　　　　 : {MMP.接続.MP3.Track.Stop(1)}")

    print(f"【ループ】")
    print(f"　・再生 → F=2,T=102 : {MMP.接続.MP3.Track.Play(1, 2, 102)}")
    print(f"　・ループ → ON　　  : {MMP.接続.MP3.Track.Loop(1, 1)}")
    time.sleep(10.0)
    print(f"　・停止　　　　　　 : {MMP.接続.MP3.Track.Stop(1)}")
    print("　[終了]\n")


#============================================================
# 4) MP3 制御：状態取得/一時停止/再開/停止/EQ/音量
#============================================================
def RunMp3Control():
    print("４.ＭＰ３制御（ DFPlayer ）")

    print("【基本】")
    print(f"　・音量 → 20     : {MMP.接続.MP3.Set.Volume(1, 20)}")
    print(f"　・再生 → F=4,T=1: {MMP.接続.MP3.Track.Play(1, 4, 1)}")
    print(f"　・ループ → OFF  : {MMP.接続.MP3.Track.Loop(1, 0)}")

    print("【インフォメーション】")
    print(f"　　・状況　　　　: {MMP.接続.MP3.Info.Track   (1)}")
    print(f"　　・音量　　　　: {MMP.接続.MP3.Info.Volume  (1)}")
    print(f"　　・イコライザ　: {MMP.接続.MP3.Info.EQ      (1)}")
    print(f"　　・現在ファイル: {MMP.接続.MP3.Info.FileID  (1)}")
    print(f"　　・総ファイル数: {MMP.接続.MP3.Info.Files   (1)}")

    print("【基本】")
    print(f"　　・一時停止　　: {MMP.接続.MP3.Track.Pause(1)}")

    time.sleep(2.0)
    print(f"　　・再開　　　　: {MMP.接続.MP3.Track.Start(1)}")

    print("【イコライザー】")
    for mode in range(0, 6):
        print(f"　　・{mode} : {MMP.接続.MP3.Set.EQ(1, mode)}")
        time.sleep(3.0)

    print("　・音量")
    for v in range(0, 31, 5):
        print(f"　　・{v} : {MMP.接続.MP3.Set.Volume(1, v)}")
        time.sleep(1.0)

    print(f"　・停止 : {MMP.接続.MP3.Track.Stop(1)}")
    print("　[終了]\n")


#============================================================
# 5) PWM 生値：ch0=180度型、ch15=連続回転型
# 8) I2C：サーボスイープ（PCA9685 レジスタ直書き）
#============================================================
CH_180  = 0
CH_360  = 15
PWM_MIN = 150
PWM_MAX = 600
PWM_MID = (PWM_MIN + PWM_MAX) // 2
PAUSE_S = 0.5

def RunPwm(argMode):
    Title = ("５.ＰＷＭ") if argMode else ("６.Ｉ２Ｃ")
    print(f"{Title}（ PCA9685：サーボモータ180度型,連続回転型 ）")

    STEP         = 3
    STEP_DELAY_S = 0.005
    PCA_ADDR     = 0x40

    def RunI2C(ch, ticks):
        base_reg  = 0x06 + 4 * ch
        MMP.接続.I2C.Write(PCA_ADDR, base_reg + 2, (ticks     ) & 0xFF)
        MMP.接続.I2C.Write(PCA_ADDR, base_reg + 3, (ticks >> 8) & 0x0F)

    print("　・初期位置")
    if argMode:
        MMP.接続.PWM.Out(CH_180, PWM_MID)
        MMP.接続.PWM.Out(CH_360, PWM_MID)
    else:
        RunI2C(CH_180, PWM_MID)
        RunI2C(CH_360, PWM_MID)
    time.sleep(PAUSE_S)

    print("　・正転:加速")
    for pwmVal in range(PWM_MID, PWM_MAX, STEP):
        if argMode:
            MMP.接続.PWM.Out(CH_180, pwmVal)
            MMP.接続.PWM.Out(CH_360, pwmVal)
        else:
            RunI2C(CH_180, pwmVal)
            RunI2C(CH_360, pwmVal)
        time.sleep(STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・逆転:減速")
    for pwmVal in range(PWM_MAX, PWM_MIN, -STEP):
        if argMode:
            MMP.接続.PWM.Out(CH_180, pwmVal)
            MMP.接続.PWM.Out(CH_360, pwmVal)
        else:
            RunI2C(CH_180, pwmVal)
            RunI2C(CH_360, pwmVal)
        time.sleep(STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・初期位置")
    if argMode:
        MMP.接続.PWM.Out(CH_180, PWM_MID)
        MMP.接続.PWM.Out(CH_360, PWM_MID)
    else:
        RunI2C(CH_180, PWM_MID)
        RunI2C(CH_360, PWM_MID)

    print("　[終了]\n")

#============================================================
# 6) PWM 角度制御
# 7) PWM 回転制御
#============================================================
def RunPwmSweep(mode, ch, start, end, step, delay):
    if start < end  : rng = range(start, end + 1,  step)
    else            : rng = range(start, end - 1, -step)
    for pwmVal in rng:
        if mode : MMP.接続.PWM.Angle.Out(ch, pwmVal)
        else    : MMP.接続.PWM.Rotation.Out(ch, pwmVal)
        time.sleep(delay)

#============================================================
# 6) PWM 角度制御
#============================================================
def RunPwm_Angle():
    print("６.ＰＷＭ（ 角度指定：180度型サーボ ）")

    MODE            = True      # 180度型サーボモータ
    CH              = CH_180    # サーボを接続するGPIO番号 
    ANGLE_MAX       = 180       # 180度型
    STEP            = 3         # 変化量(度)
    STEP_DELAY_S    = 0.01      # 変化間隔(秒)

    print("　・設定(登録)")
    res = MMP.接続.PWM.Angle.Init(
        CH_180      ,   # サーボを接続するGPIO番号
        -1          ,   # 単一
        ANGLE_MAX   ,   # 角度最大値
        PWM_MIN     ,   # 0度でのPWM値
        PWM_MAX     ,   # 角度最大でのPWM値
                        # 省略：中間を自動設定
    )

    print("　・角度：０")
    MMP.接続.PWM.Angle.Out(CH, 0)
    time.sleep(PAUSE_S)

    print("　・角度：0度～最大")
    RunPwmSweep(MODE, CH, 0, ANGLE_MAX, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・角度：最大～0度")
    RunPwmSweep(MODE, CH, ANGLE_MAX, 0, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・中心位置")
    MMP.接続.PWM.Angle.Center(CH)
    time.sleep(PAUSE_S)

    print("　・設定削除")
    res = MMP.接続.PWM.Angle.Delete(
        CH  ,   # サーボを接続するGPIO番号
        -1  ,   # 単一
    )

    print("　[終了]\n")

#============================================================
# 7) PWM 回転制御
#============================================================
def RunPwm_Rotation():
    print("７.ＰＷＭ（ 連続回転型サーボ ）")

    MODE            = False     # 連続回転型サーボモータ
    CH              = CH_360    # サーボを接続するGPIO番号 
    STEP            = 1         # 変化量(度)
    STEP_DELAY_S    = 0.05      # 変化間隔(秒)

    print("　・初期化")
    MMP.接続.PWM.Rotation.Init(
        CH_360  ,   # サーボを接続するGPIO番号
        -1      ,   # 単一のGPIO番号
        PWM_MIN ,   # ＰＷＭ値(右周り最大)
        PWM_MAX ,   # ＰＷＭ値(右周り最大)
                    # 省略：中間を自動設定
    )

    print("　・停止")
    MMP.接続.PWM.Rotation.Stop(CH)
    time.sleep(PAUSE_S)

    print("　・正転：0～100%")
    RunPwmSweep(MODE, CH, 0, 100, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・正転：100～0%")
    RunPwmSweep(MODE, CH, 100, 0, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・逆転：0～100%")
    RunPwmSweep(MODE, CH, 0, -100, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・逆転：100～0%")
    RunPwmSweep(MODE, CH, -100, 0, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・停止")
    MMP.接続.PWM.Rotation.Stop(CH)

    print("　・設定削除")
    res = MMP.接続.PWM.Rotation.Delete(
        CH  ,   # サーボを接続するGPIO番号
        -1  ,   # 単一
    )

    print("　[終了]\n")

#======================================================
if __name__ == "__main__": 
    main()
