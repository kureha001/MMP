# -*- coding: utf-8 -*-
#============================================================
# ＭＭＰコマンド テスト（ＷＥＢ－ＡＰＩ経由で実行）
#------------------------------------------------------------
# シリアル通信・TCPブリッジに対応
# CPython / MicroPython / CircuitPython に対応
#============================================================
# -*- coding: utf-8 -*-
# ============================================================
# ＭＭＰコマンド テスト（Web API 版）
# - 元の Python テストの「printログ」を維持
# - API の詳細ログ（URL/HTTP/JSON）は VERBOSE_API で表示/非表示を切替
# - 依存：標準ライブラリのみ（urllib）
# ============================================================
import json

# ====== 設定 ======
HOST = "192.168.2.124"      # LANの場合
#HOST = "203.141.144.142"    #WANの場合
PORT = 8080

# API 詳細ログ（URL/HTTP/JSON）を表示するか？
VERBOSE_API = False   # ← True で詳細ログ

#============================================================
# HTTP 互換レイヤ（CPython / MicroPython / CircuitPython）
#============================================================
import sys
IS_MICRO   = (sys.implementation.name == "micropython")
IS_CIRCUIT = (sys.implementation.name == "circuitpython")

if IS_MICRO:
    # --- MicroPython: urequests ---
    import urequests as _requests
    import utime as time
    def _http_get(url, timeout=10.0):
        try:
            try:
                resp = _requests.get(url, timeout=timeout)
            except TypeError:
                resp = _requests.get(url)  # 一部ポートはtimeout未対応
            try:
                status = getattr(resp, "status", getattr(resp, "status_code", 0))
                text = resp.text if hasattr(resp, "text") else resp.content.decode("utf-8", "replace")
                return status, text
            finally:
                try: resp.close()
                except: pass
        except Exception as e:
            raise RuntimeError("MicroPython GET failed: {}".format(e))

elif IS_CIRCUIT:
    # --- CircuitPython: adafruit_requests + socketpool + ssl ---
    import time
    import ssl
    import wifi          # 事前に wifi.radio.connect(...) 済み前提
    import socketpool
    import adafruit_requests as _ada_req

    _CP_SESSION = None
    def _get_cp_session():
        global _CP_SESSION
        if _CP_SESSION is None:
            pool = socketpool.SocketPool(wifi.radio)
            try:
                ctx = ssl.create_default_context()
            except Exception:
                ctx = None
            _CP_SESSION = _ada_req.Session(pool, ctx)
            # どの版でも扱えるよう念のためデフォルトタイムアウトも設定可
            try:
                _CP_SESSION.timeout = 10  # サポートされる版のみ反映
            except Exception:
                pass
        return _CP_SESSION

    def _http_get(url, timeout=10.0):
        s = _get_cp_session()
        try:
            try:
                resp = s.get(url, timeout=timeout)  # 新しめの版
            except TypeError:
                resp = s.get(url)                   # 古い版
            status = getattr(resp, "status_code", getattr(resp, "status", 0))
            text = resp.text
            return status, text
        finally:
            try: resp.close()
            except: pass

else:
    # --- CPython: urllib ---
    import time
    from urllib import request as _urlreq, error as _urlerr

    def _http_get(url, timeout=10.0):
        try:
            with _urlreq.urlopen(url, timeout=timeout) as resp:
                status = getattr(resp, "status", 0)
                text = resp.read().decode("utf-8", errors="replace")
                return status, text
        except _urlerr.URLError as e:
            raise RuntimeError("URLError: {}".format(e))


#============================================================
# 共通ユーティリティ
#============================================================
def ConnInfo(argLogSW=False):
    for p in ("/INFO/VERSION"):
        _api_get(p, argLog=argLogSW)

def _api_get(path, timeout=10.0, argLog=False):
    url = f"http://{HOST}:{PORT}{path}"
    body_text = ""
    status = 0
    ok = False
    j = None
    status, body_text = _http_get(url, timeout=timeout)
    ok = (200 <= status < 300)
    j = json.loads(body_text)

    if VERBOSE_API or argLog:
        print("\n=== API ===\nGET {}\nHTTP {}".format(url, status))
        if j is not None:
            try:
                print(json.dumps(j, ensure_ascii=False, indent=2))
            except Exception:
                print(body_text or "(空)")
        else:
            print(body_text or "(空)")
        print("=== /API ===\n")

    return ok, status, j, body_text

#============================================================
# メイン（共通）
#============================================================
def main():

    while True:
        print("")
        print("---------- ＭＭＰ ＡＰＩテスト -----------")
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
def tf(b):  # True/False を文字で
    return "True" if b else "False"

#============================================================
# 1) アナログ入力(HC4067)
#============================================================
def RunAnalog():

    print("１.アナログ入力（ HC4067：JoyPad1,2 ）")

    命令 = "/ANALOG/"

    ok, _, j, _ = _api_get(f"{命令}SETUP:2:4")
    ok1 = bool(j and j.get("result"))
    print(f"　・アクセス範囲指定 [2,4]  : {ok1}")
    if not ok1:
        print("　 <<中断>>\n")
        return

    ok, _, j, _ = _api_get(f"{命令}INPUT")
    ok2 = bool(j and j.get("result"))
    print(f"　・アナログ値をバッファに格納 : {ok2}")
    if not ok2:
        print("　 <<中断>>\n")
        return

    print("　・バッファを参照")
    for x in range(0, 2):
        print(f"　　JoyPad[{x + 1}]")
        for y in range(0, 4):
            _, _, jj, _ = _api_get(f"{命令}READ:{x}:{y}")
            print(f"　　　[{y}] = {jj.get("value")}")
    print("　[終了]\n")

#============================================================
# 2) デジタル入出力
#============================================================
def RunDigital():

    print("２.デジタル入出力（ GPIO ）")

    命令 = "/DIGITAL/"

    print("　・入力")
    for pin in (2, 6, 7):
        _, _, j, _ = _api_get(f"{命令}INPUT:{pin}")
        print(f"　　[{pin}] {'ON' if j.get("value")==0 else 'OFF'}")

    print("　・出力[3]")
    for _ in range(6):
        _, _, j1, _ = _api_get(f"{命令}OUTPUT:3:1")
        print(f"　　・[HIGH] {tf(j1.get("result"))}"); time.sleep(0.5)
        _, _, j0, _ = _api_get(f"{命令}OUTPUT:3:0")
        print(f"　　・[LOW ] {tf(j0.get("result"))}"); time.sleep(0.5)
    print("　[終了]\n")

#============================================================
# 3) MP3：フォルダ1のトラック再生,リピート再生
#============================================================
def RunMp3Playlist():

    print("３.ＭＰ３再生（ DFPlayer ）")

    命令0 = "/MP3/SET/"
    命令1 = "/MP3/TRACK/"

    _, _, j, _ = _api_get(f"{命令0}VOLUME:1:20")
    print(f"　・音量 → 20 : {j.get("result")}")

    _, _, j, _ = _api_get(f"{命令1}LOOP:1:0")
    print(f"　・ループ → OFF : {j.get("value")}")

    print("　・再生")
    for track in range(1, 4):
        ok, _, j, _ = _api_get(f"{命令1}PLAY:1:1:{track}")
        print(f"　　→ F=1,T={track} : {j.get("value")}")
        time.sleep(3.0)

    _, _, j, _ = _api_get(f"{命令1}STOP:1")
    print(f"　・停止 : {j.get("value")}")

    _, _, j, _ = _api_get(f"{命令1}PLAY:1:2:102")
    print(f"　・再生 → F=2,T=102 : {j.get("value")}")

    _, _, j, _ = _api_get(f"{命令1}LOOP:1:1")
    print(f"　・ループ → ON : {j.get("value")}")
    time.sleep(10.0)

    _, _, j, _ = _api_get(f"{命令1}STOP:1")
    print(f"　・停止 : {j.get("value")}")

    print("　[終了]\n")

#============================================================
# 4) MP3 制御：状態取得/一時停止/再開/停止/EQ/音量
#============================================================
def RunMp3Control():

    print("４.ＭＰ３制御（ DFPlayer ）")

    命令0 = "/MP3/SET/"
    命令1 = "/MP3/TRACK/"
    命令2 = "/MP3/INFO/"

    _, _, j, _ = _api_get(f"{命令0}VOLUME:1:20")
    print(f"　・音量 → 20 : {tf(j.get("ok"))}")

    _, _, j, _ = _api_get(f"{命令1}PLAY:1:4:1")
    print(f"　・再生 → F=4,T=1 : {j.get("value")}")

    _, _, j, _ = _api_get(f"{命令1}LOOP:1:0")
    print(f"　・ループ → OFF : {j.get("value")}")

    print("　・参照")
    _, _, st, _ = _api_get(f"{命令2}TRACK:1"  )
    _, _, vv, _ = _api_get(f"{命令2}VOLUME:1" )
    _, _, eq, _ = _api_get(f"{命令2}EQ:1"     )
    _, _, fn, _ = _api_get(f"{命令2}FILEID:1" )
    _, _, fc, _ = _api_get(f"{命令2}FILES:1"  )
    print(f"　　・トラック状況 = {st.get("value")}")
    print(f"　　・音量         = {vv.get("value")}")
    print(f"　　・イコライザ   = {eq.get("value")}")
    print(f"　　・現在ファイル = {fn.get("value")}")
    print(f"　　・総ファイル数 = {fc.get("value")}")

    _, _, j, _ = _api_get(f"{命令1}PAUSE:1")
    print(f"　・一時停止 : {j.get("value")}")
    time.sleep(2.0)

    _, _, j, _ = _api_get(f"{命令1}START:1")
    print(f"　・再開 : {j.get("value")}")

    print("　・イコライザー")
    for mode in range(0, 6):
        _, _, j, _ = _api_get(f"{命令0}EQ:1:{mode}")
        print(f"　　[{mode}] {tf(j.get("result"))}")
        time.sleep(3.0)

    print("　・音量")
    for v in range(0, 31, 5):
        _, _, j, _ = _api_get(f"{命令0}VOLUME:1:{v}")
        print(f"　　[{v}] {tf(j.get("ok"))}")
        time.sleep(1.0)

    _, _, j, _ = _api_get(f"{命令1}STOP:1")
    print(f"　・停止 : {j.get("value")}")
    print("　[終了]\n")

#============================================================
# 5) PWM 生値：ch0=180度型、ch15=連続回転型
# 6) I2C：サーボスイープ（PCA9685 レジスタ直書き）
#============================================================
CH_180       = 0
CH_360       = 15
PWM_MIN = 150
PWM_MAX = 600
PWM_MID = (PWM_MIN + PWM_MAX) // 2
PAUSE_S = 0.5

def RunPwm(mode_pwm=True):
    title = "５.ＰＷＭ" if mode_pwm else "６.Ｉ２Ｃ"
    print("{}（ PCA9685：サーボモータ180度型,連続回転型 ）".format(title))
 
    命令PWM = "/PWM/OUTPUT"
    命令I2C = "/I2C/WRITE"

    STEP         = 3
    STEP_DELAY_S = 0.01
    PCA_ADDR     = 0x40

    def RunI2C(ch, ticks):
        base_reg = 0x06 + 4 * ch
        _api_get(f"{命令I2C}:{PCA_ADDR}:{base_reg + 2}:{(ticks     ) & 0xFF}")
        _api_get(f"{命令I2C}:{PCA_ADDR}:{base_reg + 3}:{(ticks >> 8) & 0x0F}")

    print("　・初期位置")
    if mode_pwm:
        _api_get(f"{命令PWM}:{CH_180}:{PWM_MID}")
        _api_get(f"{命令PWM}:{CH_360}:{PWM_MID}")
    else:
        RunI2C(CH_180, PWM_MID)
        RunI2C(CH_360, PWM_MID)
    time.sleep(PAUSE_S)

    print("　・正転:加速")
    for pwmVal in range(PWM_MID, PWM_MAX, STEP):
        if mode_pwm:
            _api_get(f"{命令PWM}:{CH_180}:{pwmVal}")
            _api_get(f"{命令PWM}:{CH_360}:{pwmVal}")
        else:
            RunI2C(CH_180, pwmVal)
            RunI2C(CH_360, pwmVal)
        if STEP_DELAY_S:
            time.sleep(STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・逆転:減速")
    for pwmVal in range(PWM_MAX, PWM_MIN, -STEP):
        if mode_pwm:
            _api_get(f"{命令PWM}:{CH_180}:{pwmVal}")
            _api_get(f"{命令PWM}:{CH_360}:{pwmVal}")
        else:
            RunI2C(CH_180, pwmVal)
            RunI2C(CH_360, pwmVal)
        if STEP_DELAY_S:
            time.sleep(STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・初期位置")
    if mode_pwm:
        _api_get(f"{命令PWM}:{CH_180}:{PWM_MID}")
        _api_get(f"{命令PWM}:{CH_360}:{PWM_MID}")
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

    print("　・角度：０")
    print(命令.OUTPUT(CH, 0))
    time.sleep(PAUSE_S)

    print("　・角度：0度～最大")
    RunPwmSweep(命令, CH, 0, ANGLE_MAX, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・角度：最大～0度")
    RunPwmSweep(命令, CH, ANGLE_MAX, 0, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・中心位置")
    print(命令.CENTER(CH))
    time.sleep(PAUSE_S)

    print("　・設定削除")
    res = 命令.DELETE(
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
    res = 命令.DELETE(
        CH  ,   # サーボを接続するGPIO番号
        -1  ,   # 単一
    )

    print("　[終了]\n")

#======================================================
if __name__ == "__main__":
    main()

