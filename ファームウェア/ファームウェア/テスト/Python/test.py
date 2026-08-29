# -*- coding: utf-8 -*-
#============================================================
# ＭＭＰコマンド テスト メイン (test.py)
#============================================================
import time
from test_comm import COMM_CONFIG, session_connect, session_disconnect, api_get

#============================================================
# メインループ
#============================================================
def main():
    while True:
        current_mode = COMM_CONFIG["CURRENT_MODE"]
        print("")
        print("---------- ＭＭＰ ＡＰＩテスト -----------")
        print(f"現在の通信モード: [ {current_mode} ] (切替: M)")
        print("------------------------------------------")
        print("Ｉ／Ｏ：[1]アナログ入力 [2]デジタル入出力"   )
        print("ＭＰ３：[3]基本  [4]制御"                    )
        print("ＰＷＭ：[5]基本  [6]角度  [7]回転"           )
        print("Ｉ２Ｃ：[8]PCA9685を直接制御"                )
        print("------------------------------------------")
        print("[M] 通信モード変更 (WAPI / TCPR / WSOC / COMP / BLE)")
        print("[Q] 終了")
        print("------------------------------------------")

        入力 = input("> コマンドを入力：").upper()
        
        if   入力 == "Q": break
        elif 入力 == "M":
            if current_mode == "WAPI":
                COMM_CONFIG["CURRENT_MODE"] = "TCPR"
                print(f"\n-> 通信モードを [TCP(RAW)] (Port: {COMM_CONFIG['PORT_TCPR']}) に変更しました。")
            elif current_mode == "TCPR":
                COMM_CONFIG["CURRENT_MODE"] = "WSOC"
                print(f"\n-> 通信モードを [WebSocket] (Port: {COMM_CONFIG['PORT_WSOC']}) に変更しました。")
            elif current_mode == "WSOC":
                COMM_CONFIG["CURRENT_MODE"] = "COMP"
                print(f"\n-> 通信モードを [COMポート] ({COMM_CONFIG['COMP_PORT']}) に変更しました。")
            elif current_mode == "COMP":
                COMM_CONFIG["CURRENT_MODE"] = "BLE"
                print(f"\n-> 通信モードを [BLE] ({COMM_CONFIG['BLE_NAME']}) に変更しました。")
            else:
                COMM_CONFIG["CURRENT_MODE"] = "WAPI"
                print(f"\n-> 通信モードを [WebAPI] (Port: {COMM_CONFIG['PORT_WAPI']}) に変更しました。")
            continue

        session_connect()
        try:
            if   入力 == "1": print("\n"); RunAnalog()
            elif 入力 == "2": print("\n"); RunDigital()
            elif 入力 == "3": print("\n"); RunMp3Playlist()
            elif 入力 == "4": print("\n"); RunMp3Control()
            elif 入力 == "5": print("\n"); RunPwm(True)
            elif 入力 == "6": print("\n"); RunPwm_Angle()
            elif 入力 == "7": print("\n"); RunPwm_Rotate()
            elif 入力 == "8": print("\n"); RunPwm(False)
            else          : print("\nエラー：入力誤り")
        finally:
            session_disconnect()

#============================================================
# 出力文字ヘルパ
#============================================================
def tf(b):
    return "True" if b else "False"

#============================================================
# 各テスト関数
#============================================================
def RunAnalog():
    print("１.アナログ入力（ HC4067：JoyPad1,2 ）")
    命令 = "/ANALOG/"
    ok, _, j, _ = api_get(f"{命令}SETUP:2:4")
    ok1 = bool(j and (j.get("result") or j.get("ok")))
    print(f" ・アクセス範囲指定 [2,4]  : {ok1}")
    if not ok1:
        print("  <<中断>>\n")
        return

    ok, _, j, _ = api_get(f"{命令}INPUT")
    ok2 = bool(j and (j.get("result") or j.get("ok")))
    print(f" ・アナログ値をバッファに格納 : {ok2}")
    if not ok2:
        print("  <<中断>>\n")
        return

    print(" ・バッファを参照")
    for x in range(0, 2):
        print(f"  JoyPad[{x + 1}]")
        for y in range(0, 4):
            _, _, jj, _ = api_get(f"{命令}READ:{x}:{y}")
            val = jj.get("value") if jj else "NaN"
            print(f"   [{y}] = {val}")
    print(" [終了]\n")

def RunDigital():
    print("２.デジタル入出力（ GPIO ）")
    命令 = "/DIGITAL/"
    print(" ・入力")
    for pin in (18, 14, 13):
        _, _, j, _ = api_get(f"{命令}INPUT:{pin}")
        val = j.get("value") if j else 1
        print(f"  [{pin}] {'ON' if val==0 else 'OFF'}")

    print(" ・出力[17]")
    for _ in range(3):
        _, _, j1, _ = api_get(f"{命令}OUTPUT:17:1")
        res1 = j1.get("result") if j1 else True
        print(f"  ・[HIGH] {tf(res1)}"); time.sleep(0.5)
        _, _, j0, _ = api_get(f"{命令}OUTPUT:17:0")
        res0 = j0.get("result") if j0 else True
        print(f"  ・[LOW ] {tf(res0)}"); time.sleep(0.5)
    print(" [終了]\n")

def RunMp3Playlist():
    print("３.ＭＰ３再生（ DFPlayer ）")
    命令0 = "/MP3/SET/"
    命令1 = "/MP3/TRACK/"
    _, _, j, _ = api_get(f"{命令0}VOLUME:1:20")
    print(f" ・音量 → 20 : {j.get('result') if j else 'True'}")
    _, _, j, _ = api_get(f"{命令1}LOOP:1:0")
    print(f" ・ループ → OFF : {j.get('value') if j else '0'}")
    print(" ・再生")
    for track in range(1, 4):
        _, _, j, _ = api_get(f"{命令1}PLAY:1:1:{track}")
        print(f"  → F=1,T={track} : {j.get('value') if j else 'OK'}")
        time.sleep(3.0)
    _, _, j, _ = api_get(f"{命令1}STOP:1")
    print(f" ・停止 : {j.get('value') if j else 'OK'}")
    _, _, j, _ = api_get(f"{命令1}PLAY:1:2:102")
    print(f" ・再生 → F=2,T=102 : {j.get('value') if j else 'OK'}")
    _, _, j, _ = api_get(f"{命令1}LOOP:1:1")
    print(f" ・ループ → ON : {j.get('value') if j else '1'}")
    time.sleep(10.0)
    _, _, j, _ = api_get(f"{命令1}STOP:1")
    print(f" ・停止 : {j.get('value') if j else 'OK'}")
    print(" [終了]\n")

def RunMp3Control():
    print("４.ＭＰ３制御（ DFPlayer ）")
    命令0 = "/MP3/SET/"
    命令1 = "/MP3/TRACK/"
    命令2 = "/MP3/INFO/"
    _, _, j, _ = api_get(f"{命令0}VOLUME:1:20")
    print(f" ・音量 → 20 : {tf(j.get('ok') if j else True)}")
    _, _, j, _ = api_get(f"{命令1}PLAY:1:4:1")
    print(f" ・再生 → F=4,T=1 : {j.get('value') if j else 'OK'}")
    _, _, j, _ = api_get(f"{命令1}LOOP:1:0")
    print(f" ・ループ → OFF : {j.get('value') if j else '0'}")
    print(" ・参照")
    _, _, st, _ = api_get(f"{命令2}TRACK:1"  )
    _, _, vv, _ = api_get(f"{命令2}VOLUME:1" )
    _, _, eq, _ = api_get(f"{命令2}EQ:1"     )
    _, _, fn, _ = api_get(f"{命令2}FILEID:1" )
    _, _, fc, _ = api_get(f"{命令2}FILES:1"  )
    print(f"  ・トラック状況 = {st.get('value') if st else 'NaN'}")
    print(f"  ・音量         = {vv.get('value') if vv else 'NaN'}")
    print(f"  ・イコライザ   = {eq.get('value') if eq else 'NaN'}")
    print(f"  ・現在ファイル = {fn.get('value') if fn else 'NaN'}")
    print(f"  ・総ファイル数 = {fc.get('value') if fc else 'NaN'}")
    _, _, j, _ = api_get(f"{命令1}PAUSE:1")
    print(f" ・一時停止 : {j.get('value') if j else 'OK'}")
    time.sleep(2.0)
    _, _, j, _ = api_get(f"{命令1}START:1")
    print(f" ・再開 : {j.get('value') if j else 'OK'}")
    print(" ・イコライザー")
    for mode in range(0, 6):
        _, _, j, _ = api_get(f"{命令0}EQ:1:{mode}")
        print(f"  [{mode}] {tf(j.get('result') if j else True)}")
        time.sleep(3.0)
    print(" ・音量")
    for v in range(0, 31, 5):
        _, _, j, _ = api_get(f"{命令0}VOLUME:1:{v}")
        print(f"  [{v}] {tf(j.get('ok') if j else True)}")
        time.sleep(1.0)
    _, _, j, _ = api_get(f"{命令1}STOP:1")
    print(f" ・停止 : {j.get('value') if j else 'OK'}")
    print(" [終了]\n")

CH_180      = 0
CH_360      = 15
PWM_180     = (90,600)
PWM_360     = ((397,430),(358,330))
PWM_MID_180 = (PWM_180[0]    + PWM_180[1])    // 2
PWM_MID_360 = (PWM_360[0][0] + PWM_360[1][0]) // 2
PAUSE_S     = 2

def RunPwm(argMode=True):
    title = "５.ＰＷＭ" if argMode else "６.Ｉ２Ｃ"
    print("{}（ PCA9685：サーボモータ180度型,連続回転型 ）".format(title))
    命令PWM = "/PWM/OUTPUT"
    命令I2C = "/I2C/WRITE"
    STEP         = 3
    STEP_DELAY_S = 0.005
    PCA_ADDR     = 0x40

    def RunI2C(ch, ticks):
        base_reg = 0x06 + 4 * ch
        api_get(f"{命令I2C}:{PCA_ADDR}:{base_reg + 2}:{(ticks    ) & 0xFF}")
        api_get(f"{命令I2C}:{PCA_ADDR}:{base_reg + 3}:{(ticks >> 8) & 0x0F}")

    print(" ・初期位置")
    if argMode:
        api_get(f"{命令PWM}:{CH_180}:{PWM_MID_180}")
        api_get(f"{命令PWM}:{CH_360}:{PWM_MID_360}")
    else:
        RunI2C(CH_180, PWM_MID_180)
        RunI2C(CH_360, PWM_MID_360)
    time.sleep(PAUSE_S)

    print(" ・角度:増加")
    for pwmVal in range(PWM_180[0], PWM_180[1], STEP):
        if argMode: api_get(f"{命令PWM}:{CH_180}:{pwmVal}")
        else      : RunI2C(CH_180, pwmVal)
        time.sleep(STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print(" ・角度:減少")
    for pwmVal in range(PWM_180[1], PWM_180[0], -STEP):
        if argMode: api_get(f"{命令PWM}:{CH_180}:{pwmVal}")
        else      : RunI2C(CH_180, pwmVal)
        time.sleep(STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print(" ・正転")
    for pwmVal in range(PWM_MID_360, PWM_360[0][1], STEP):
        if argMode: api_get(f"{命令PWM}:{CH_360}:{pwmVal}")
        else      : RunI2C(CH_360, pwmVal)
        time.sleep(STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print(" ・逆転")
    for pwmVal in range(PWM_360[1][0], PWM_360[1][1], -STEP):
        if argMode: api_get(f"{命令PWM}:{CH_360}:{pwmVal}")
        else      : RunI2C(CH_360, pwmVal)
        time.sleep(STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print(" ・初期位置")
    if argMode:
        api_get(f"{命令PWM}:{CH_180}:{PWM_MID_180}")
        api_get(f"{命令PWM}:{CH_360}:{PWM_MID_360}")
    else:
        RunI2C(CH_180, PWM_MID_180)
        RunI2C(CH_360, PWM_MID_360)
    print(" [終了]\n")

def RunPwmSweep(命令, ch, start, end, step, delay):
    if start < end  : rng = range(start, end + 1,  step)
    else            : rng = range(start, end - 1, -step)
    for pwmVal in rng:
        api_get(f"{命令}OUTPUT:{ch}:{pwmVal}")
        time.sleep(delay)

def RunPwm_Angle():
    print("６.ＰＷＭ（ 角度指定：180度型サーボ ）")
    命令 = "/PWM/ANGLE/"
    ANGLE_MAX       = 180
    STEP            = 3
    STEP_DELAY_S    = 0.01
    print(" ・設定(登録)")
    api_get(f"{命令}SETUP:{CH_180}:-1:{ANGLE_MAX}:{PWM_180[0]}:{PWM_180[1]}")
    print(" ・角度：0度")
    api_get(f"{命令}OUTPUT:{CH_180}:0")
    time.sleep(PAUSE_S)
    print(" ・角度：0度～最大")
    RunPwmSweep(命令, CH_180, 0, ANGLE_MAX, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)
    print(" ・角度：最大～0度")
    RunPwmSweep(命令, CH_180, ANGLE_MAX, 0, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)
    print(" ・角度：中心")
    api_get(f"{命令}OUTPUT:{CH_180}:{ANGLE_MAX // 2}")
    time.sleep(PAUSE_S)
    print(" ・設定削除")
    api_get(f"{命令}RESET:{CH_180}:-1")
    print(" [終了]\n")

def RunPwm_Rotate():
    print("７.ＰＷＭ（ 連続回転型サーボ ）")
    命令 = "/PWM/ROTATE/"
    STEP            = 1
    STEP_DELAY_S    = 0.05
    print(" ・初期化")
    api_get(f"{命令}SETUP:{CH_360}:-1:{PWM_360[0][0]}:{PWM_360[0][1]}:{PWM_360[1][0]}:{PWM_360[1][1]}")
    print(" ・停止")
    api_get(f"{命令}OUTPUT:{CH_360}:{0}")
    time.sleep(PAUSE_S)
    print(" ・正転：加速")
    RunPwmSweep(命令, CH_360, 0, 100, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)
    print(" ・正転：減速")
    RunPwmSweep(命令, CH_360, 100, 0, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)
    print(" ・逆転：加速")
    RunPwmSweep(命令, CH_360, 0, -100, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)
    print(" ・逆転：減速")
    RunPwmSweep(命令, CH_360, -100, 0, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)
    print(" ・停止")
    api_get(f"{命令}OUTPUT:{CH_360}:{0}")
    print(" ・設定削除")
    api_get(f"{命令}RESET:{CH_360}:-1")
    print(" [終了]\n")

if __name__ == "__main__":
    main()