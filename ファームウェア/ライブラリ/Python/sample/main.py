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
# 接続情報
#============================================================
#(0) UART番号（ USB=0, UART1=1, UART2=2, ...）

#(1) 直結：シリアル接続（プラットフォーム自動）

#(2) TCPブリッジ：ser2net
NET_TIMEOUT_S   = 0.2   # ネットワーク品質に応じて調整
NET_HOST_PC     = "192.168.2.254"   # LANの場合
#NET_HOST_PC     = "203.141.144.142" #WANの場合
WEB_PORT_PC     = 8080
TCP_PORT_PC     = 8081

#(3) TCPブリッジ：usb4a(Pydroid)
USB4A_INDEX     = 0


#============================================================
# 互換シム（MMP.通信接続 / MMP.接続 を既存通り使えるように）
#============================================================
def 接続(argMode):

    文字 = None
    if argMode == "SERIAL" : 文字 = "auto"
    if argMode == "TCP"    : 文字 = f"tcp://{NET_HOST_PC}:{TCP_PORT_PC}?timeout={NET_TIMEOUT_S}"
    if argMode == "WEB"    : 文字 = f"http://{NET_HOST_PC}:{WEB_PORT_PC}?timeout={NET_TIMEOUT_S}"
    if argMode == "ANDROID": 文字 = f"usb4a://{USB4A_INDEX}"

    if 文字 is None          : return None
    if not MMP.通信接続(文字): return None
    return argMode

#============================================================
# メイン（共通）
#============================================================
def main():

    状態 = None

    while True:
        print("")
        print("---------- ＭＭＰ ＡＰＩテスト -----------")
        if 状態 is None:
            print(f"【通信方法】[S]erial [T]cp [W]eb [A]ndroid")

        文字 = f"READY [{状態}]" if 状態 else "-- NG --"
        print(f"【接続状況】{文字}")

        if 状態 is not None:
            print("------------------------------------------"  )
            print("Ｉ／Ｏ：[1]アナログ入力 [2]デジタル入出力"   )
            print("ＭＰ３：[3]基本  [4]制御"                    )
            print("ＰＷＭ：[5]基本  [6]角度  [7]回転"           )
            print("Ｉ２Ｃ：[8]PCA9685を直接制御"                )

        print("------------------------------------------")
        print("[Q] 終了")
        print("------------------------------------------")

        入力 = input("> コマンドを入力：")
        入力 = 入力.upper()
        if   入力 == "Q": break
        elif 入力 == "S": print("\n"); 状態 = 接続("SERIAL")
        elif 入力 == "T": print("\n"); 状態 = 接続("TCP")
        elif 入力 == "W": print("\n"); 状態 = 接続("WEB")
        elif 入力 == "A": print("\n"); 状態 = 接続("ANDROID")
        elif 入力 == "1": print("\n"); RunAnalog()
        elif 入力 == "2": print("\n"); RunDigital()
        elif 入力 == "3": print("\n"); RunMp3Playlist()
        elif 入力 == "4": print("\n"); RunMp3Control()
        elif 入力 == "5": print("\n"); RunPwm(True)
        elif 入力 == "6": print("\n"); RunPwm_Angle()
        elif 入力 == "7": print("\n"); RunPwm_Rotate()
        elif 入力 == "8": print("\n"); RunPwm(False)
        else            : print("\nエラー：入力誤り")


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

    命令 = MMP.接続.ANALOG

    ok = 命令.SETUP(2, 4)
    print("　・アクセス範囲指定 → [2,4]  : {}".format(tf(ok)))
    if not ok:
        print("　 <<中断>>\n")
        return

    ok = 命令.INPUT()
    print("　・アナログ値をバッファに格納 : {}".format(tf(ok)))
    if not ok:
        print("　 <<中断>>\n")
        return

    print("　・バッファを参照")
    for x in range(0, 2):
        print(f"　　JoyPad[{x + 1}]")
        for y in range(0, 4):
            print(f"　　　[{y}] = {命令.READ(x, y)}")

    print("　・丸め処理")
    print(f"　　　中間：{命令.ROUND (0, 0, 5, 10)}")
    print(f"　　　切上：{命令.ROUNDU(0, 0, 5, 10)}")
    print(f"　　　切下：{命令.ROUNDD(0, 0, 5, 10)}")

    print("　[終了]\n")


#============================================================
# 2) デジタル入出力
#============================================================
def RunDigital():

    print("２.デジタル入出力（ GPIO ）")

    命令 = MMP.接続.DIGITAL

    print("　・入力")
    for pin in (2, 6, 7):
        print(f"　　[{pin}] {'ON' if 命令.INPUT(pin)==0 else 'OFF'}")

    print("　・出力[3]")
    for _ in range(3):
        print(f"　　・[HIGH] {tf(命令.OUTPUT(3, 1))}"); time.sleep(0.5)
        print(f"　　・[LOW ] {tf(命令.OUTPUT(3, 0))}"); time.sleep(0.5)
    print("　[終了]\n")


#============================================================
# 3) MP3：フォルダ1のトラック再生,リピート再生
#============================================================
def RunMp3Playlist():

    print("３.ＭＰ３再生（ DFPlayer ）")

    命令0 = MMP.接続.MP3.SET
    命令1 = MMP.接続.MP3.TRACK

    print("【基本】")
    print(f"　・音量   [20 ]: {命令0.VOLUME(1, 20)}")
    print(f"　・ループ [OFF]: {命令1.LOOP  (1, 0 )}")

    print("　・ループ再生")
    for track in range(1, 4):
        print(f"　　　[F=1][T={track}]: {命令1.PLAY(1, 1, track)}")
        time.sleep(3.0)
    print(f"　・停止　　　　　　　: {命令1.STOP(1)}")

    print(f"【ループ】")
    print(f"　・再生   [F=2][T=102] : {命令1.PLAY(1, 2, 102)}")
    print(f"　・ループ [ON]　　　　 : {命令1.LOOP(1, 1)}"     )
    time.sleep(10.0)
    print(f"　・停止　　　　　　　　: {命令1.STOP(1)}"        )

    print("　[終了]\n")


#============================================================
# 4) MP3 制御：状態取得/一時停止/再開/停止/EQ/音量
#============================================================
def RunMp3Control():

    print("４.ＭＰ３制御（ DFPlayer ）")

    命令0 = MMP.接続.MP3.SET
    命令1 = MMP.接続.MP3.TRACK
    命令2 = MMP.接続.MP3.INFO

    print("【基本】")
    print(f"　・音量 → 20     : {命令0.VOLUME(1, 20)}"   )
    print(f"　・再生 → F=4,T=1: {命令1.PLAY(1, 4, 1)}" )
    print(f"　・ループ → OFF  : {命令1.LOOP(1, 0)}"    )

    print("【インフォメーション】")
    print(f"　　・状況　　　　: {命令2.TRACK (1)}")
    print(f"　　・音量　　　　: {命令2.VOLUME(1)}")
    print(f"　　・イコライザ　: {命令2.EQ    (1)}")
    print(f"　　・総ファイル数: {命令2.FILES (1)}")
    print(f"　　・現在ファイル: {命令2.FILEID(1)}")

    print("【基本】")
    print(f"　　・一時停止　　: {命令1.PAUSE(1)}")

    time.sleep(2.0)
    print(f"　　・再開　　　　: {命令1.START(1)}")

    print("【イコライザー】")
    for mode in range(0, 6):
        print(f"　　・{mode} : {命令0.EQ(1, mode)}")
        time.sleep(3.0)

    print("　・音量")
    for v in range(0, 31, 5):
        print(f"　　・{v} : {命令0.VOLUME(1, v)}")
        time.sleep(1.0)

    print(f"　・停止 : {命令1.STOP(1)}")
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

    命令PWM = MMP.接続.PWM.OUTPUT
    命令I2C = MMP.接続.I2C.WRITE

    STEP         = 3
    STEP_DELAY_S = 0.005
    PCA_ADDR     = 0x40

    def RunI2C(ch, ticks):
        base_reg  = 0x06 + 4 * ch
        命令I2C(PCA_ADDR, base_reg + 2, (ticks     ) & 0xFF)
        命令I2C(PCA_ADDR, base_reg + 3, (ticks >> 8) & 0x0F)

    print("　・初期位置")
    if argMode:
        命令PWM(CH_180, PWM_MID)
        命令PWM(CH_360, PWM_MID)
    else:
        RunI2C(CH_180, PWM_MID)
        RunI2C(CH_360, PWM_MID)
    time.sleep(PAUSE_S)

    print("　・正転:加速")
    for pwmVal in range(PWM_MID, PWM_MAX, STEP):
        if argMode:
            命令PWM(CH_180, pwmVal)
            命令PWM(CH_360, pwmVal)
        else:
            RunI2C(CH_180, pwmVal)
            RunI2C(CH_360, pwmVal)
        time.sleep(STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・逆転:減速")
    for pwmVal in range(PWM_MAX, PWM_MIN, -STEP):
        if argMode:
            命令PWM(CH_180, pwmVal)
            命令PWM(CH_360, pwmVal)
        else:
            RunI2C(CH_180, pwmVal)
            RunI2C(CH_360, pwmVal)
        time.sleep(STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・初期位置")
    if argMode:
        命令PWM(CH_180, PWM_MID)
        命令PWM(CH_360, PWM_MID)
    else:
        RunI2C(CH_180, PWM_MID)
        RunI2C(CH_360, PWM_MID)

    print("　[終了]\n")

#============================================================
# 6) PWM 角度制御
# 7) PWM 回転制御
#============================================================
def RunPwmSweep(命令, ch, start, end, step, delay):
    if start < end  : rng = range(start, end + 1,  step)
    else            : rng = range(start, end - 1, -step)
    for pwmVal in rng:
        命令.OUTPUT (ch, pwmVal)
        time.sleep(delay)

#============================================================
# 6) PWM 角度制御
#============================================================
def RunPwm_Angle():

    print("６.ＰＷＭ（ 角度指定：180度型サーボ ）")

    命令 = MMP.接続.PWM.ANGLE

    CH              = CH_180    # サーボを接続するGPIO番号 
    ANGLE_MAX       = 180       # 180度型
    STEP            = 3         # 変化量(度)
    STEP_DELAY_S    = 0.01      # 変化間隔(秒)

    print("　・設定(登録)")
    res = 命令.SETUP(
        CH_180      ,   # サーボを接続するGPIO番号
        -1          ,   # 単一
        ANGLE_MAX   ,   # 角度最大値
        PWM_MIN     ,   # 0度でのPWM値
        PWM_MAX     ,   # 角度最大でのPWM値
                        # 省略：中間を自動設定
    )

    print("　・角度：０ : PWM=", end="")
    print(命令.OUTPUT(CH, 0))
    time.sleep(PAUSE_S)

    print("　・角度：0度～最大")
    RunPwmSweep(命令, CH, 0, ANGLE_MAX, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・角度：最大～0度")
    RunPwmSweep(命令, CH, ANGLE_MAX, 0, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・中心位置 : PWM=", end="")
    print(命令.CENTER(CH))
    time.sleep(PAUSE_S)

    print("　・設定削除")
    res = 命令.RESET(
        CH  ,   # サーボを接続するGPIO番号
        -1  ,   # 単一
    )

    print("　[終了]\n")

#============================================================
# 7) PWM 回転制御
#============================================================
def RunPwm_Rotate():

    print("７.ＰＷＭ（ 連続回転型サーボ ）")

    命令 = MMP.接続.PWM.ROTATE

    CH              = CH_360    # サーボを接続するGPIO番号 
    STEP            = 1         # 変化量(度)
    STEP_DELAY_S    = 0.1       # 変化間隔(秒)
    RATE            = 30        # 出力率

    print("　・初期化")
    命令.SETUP(
        CH_360  ,   # サーボを接続するGPIO番号
        -1      ,   # 単一のGPIO番号
        PWM_MIN ,   # ＰＷＭ値(右周り最大)
        PWM_MAX ,   # ＰＷＭ値(右周り最大)
                    # 省略：中間を自動設定
    )

    print("　・停止")
    命令.STOP(CH)
    time.sleep(PAUSE_S)

    print("　・正転：加速")
    RunPwmSweep(命令, CH, 0, RATE, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・正転：減速")
    RunPwmSweep(命令, CH, RATE, 0, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・逆転：加速")
    RunPwmSweep(命令, CH, 0, -RATE, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・逆転：減速")
    RunPwmSweep(命令, CH, -RATE, 0, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・停止")
    命令.STOP(CH)

    print("　・設定削除")
    res = 命令.RESET(
        CH  ,   # サーボを接続するGPIO番号
        -1  ,   # 単一
    )

    print("　[終了]\n")

#======================================================
if __name__ == "__main__": 
    main()
