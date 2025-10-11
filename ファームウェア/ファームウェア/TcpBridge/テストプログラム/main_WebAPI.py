# -*- coding: utf-8 -*-
# filename : main_WebAPI.py
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
# -*- coding: utf-8 -*-

# ====== 設定 ======
HOST = "192.168.2.124"
PORT = 8080

# API 詳細ログ（URL/HTTP/JSON）を表示するか？
VERBOSE_API = False   # ← True で詳細ログ

#============================================================
# HTTP 互換レイヤ（CPython / MicroPython / CircuitPython）
#============================================================
import sys
IS_MICRO   = (sys.implementation.name == "micropython")
IS_CIRCUIT = (sys.implementation.name == "circuitpython")

# --- MicroPython: urequests ---
if IS_MICRO:
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

# --- CircuitPython: adafruit_requests + socketpool + ssl ---
elif IS_CIRCUIT:
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

# --- CPython: urllib ---
else:
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
    for p in (
        "/conn/isOpen",
        "/conn/baud",
        "/conn/port",
        "/conn/lastError",
        "/info/version"
    ):
        _api_get(p, argLog=argLogSW)

def _api_get(path, timeout=10.0, argLog=False):
    # GET /path を実行し、(ok, status, json_obj, text) を返す。
    # VERBOSE_API or argLog が True のときは詳細ログも出力する。
    url = "http://{}:{}{}".format(HOST, PORT, path)
    body_text = ""
    status = 0
    ok = False
    j = None
    try:
        status, body_text = _http_get(url, timeout=timeout)
        ok = (200 <= status < 300)
        try:
            j = json.loads(body_text)
        except Exception:
            j = None
    except Exception as e:
        body_text = "(ERROR: {})".format(e)

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
    ConnInfo(True)

    print("\n＝＝＝ ＭＭＰ ＡＰＩテスト［開始］＝＝＝\n")
    # 実施するテストだけコメントを外してください（複数可）
    #RunAnalog()         # アナログ入出力
    #RunDigital()        # デジタル入出力
    #RunMp3Playlist()    # MP3プレイヤー(基本)
    #RunMp3Control()     # MP3プレイヤー(制御)
    RunPwm(True)        # PWM出力
    #RunPwm_Angle()      # PWM出力(角度指定)
    #RunPwm_Rotation()   # PWM出力(回転指定)
    #RunPwm(False)       # I2C→PCA9685 直接制御
    print("＝＝＝ ＭＭＰ ＡＰＩテスト［終了］＝＝＝")

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

    # Configure(2,4) 相当（サーバ側は start/count）
    ok, _, j, _ = _api_get("/analog/configure?start=2&count=4&chTtl=2&devTtl=4")
    ok1 = bool(j and j.get("ok"))
    print("　・アクセス範囲指定 → [2,4]  : {}".format(tf(ok1)))
    if not ok1:
        print("　 <<中断>>\n")
        return

    ok, _, j, _ = _api_get("/analog/update")
    ok2 = bool(j and j.get("ok"))
    print("　・アナログ値をバッファに格納 : {}".format(tf(ok2)))
    if not ok2:
        print("　 <<中断>>\n")
        return

    print("　・バッファを参照")
    for x in range(0, 2):
        print("　　JoyPad[{}]".format(x + 1))
        for y in range(0, 4):
            _, _, jj, _ = _api_get("/analog/read?ch={}&dev={}".format(x, y))
            v = jj.get("value") if jj and ("value" in jj) else "NaN"
            print("　　　← [{}] = {}".format(y, v))
    print("　[終了]\n")

#============================================================
# 2) デジタル入出力
#============================================================
def RunDigital():
    print("２.デジタル入出力（ GPIO ）")
    print("　・入力")
    for pin in (2, 6, 7):
        _, _, j, _ = _api_get("/digital/in?id={}".format(pin))
        val = j.get("value") if j and ("value" in j) else 1
        print("　　←[{}] = {}".format(pin, 'ON' if val==0 else 'OFF'))

    print("　・出力[3]")
    for _ in range(6):
        _, _, j1, _ = _api_get("/digital/out?id=3&val=1")
        print("　　→ HIGH : {}".format(tf(j1 and j1.get("ok"))))
        time.sleep(0.5)
        _, _, j0, _ = _api_get("/digital/out?id=3&val=0")
        print("　　→ LOW  : {}".format(tf(j0 and j0.get("ok"))))
        time.sleep(0.5)
    print("　[終了]\n")

#============================================================
# 3) MP3：フォルダ1のトラック再生,リピート再生
#============================================================
def RunMp3Playlist():
    print("３.ＭＰ３再生（ DFPlayer ）")

    _, _, j, _ = _api_get("/audio/volume?dev=1&value=20")
    print("　・音量 → 20 : {}".format(tf(j and j.get("ok"))))

    _, _, j, _ = _api_get("/audio/play/setLoop?dev=1&mode=0")
    print("　・ループ → OFF : {}".format(tf(j and j.get("ok"))))

    print("　・再生")
    for track in range(1, 4):
        ok, _, j1, _ = _api_get("/audio/play/start?dev=1&dir=1&file={}".format(track))
        _, _, j2, _ = _api_get("/audio/read/state?dev=1")
        st = j2.get("state") if j2 and ("state" in j2) else "NaN"
        print("　　→ F=1,T={} : {}：状況 = {}".format(track, tf(j1 and j1.get('ok')), st))
        time.sleep(3.0)

    _, _, j, _ = _api_get("/audio/play/stop?dev=1")
    _, _, jj, _ = _api_get("/audio/read/state?dev=1")
    st = jj.get("state") if jj and ("state" in jj) else "NaN"
    print("　・停止 : {} : 状況 = {}".format(tf(j and j.get('ok')), st))

    _, _, j, _ = _api_get("/audio/play/start?dev=1&dir=2&file=102")
    print("　・再生 → F=2,T=102 : {}".format(tf(j and j.get("ok"))))

    _, _, j, _ = _api_get("/audio/play/setLoop?dev=1&mode=1")
    _, _, jj, _ = _api_get("/audio/read/state?dev=1")
    st = jj.get("state") if jj and ("state" in jj) else "NaN"
    print("　・ループ → ON : {} : 状況 = {}".format(tf(j and j.get('ok')), st))
    time.sleep(10.0)

    _, _, j, _ = _api_get("/audio/play/stop?dev=1")
    _, _, jj, _ = _api_get("/audio/read/state?dev=1")
    st = jj.get("state") if jj and ("state" in jj) else "NaN"
    print("　・停止 : {} : 状況 = {}".format(tf(j and j.get('ok')), st))
    print("　[終了]\n")

#============================================================
# 4) MP3 制御：状態取得/一時停止/再開/停止/EQ/音量
#============================================================
def RunMp3Control():
    print("４.ＭＰ３制御（ DFPlayer ）")

    _, _, j, _ = _api_get("/audio/volume?dev=1&value=20")
    print("　・音量 → 20 : {}".format(tf(j and j.get("ok"))))

    _, _, j, _ = _api_get("/audio/play/start?dev=1&dir=4&file=1")
    print("　・再生 → F=4,T=1 : {}".format(tf(j and j.get("ok"))))

    _, _, j, _ = _api_get("/audio/play/setLoop?dev=1&mode=0")
    print("　・ループ → OFF : {}".format(tf(j and j.get("ok"))))

    print("　・参照")
    _, _, st, _ = _api_get("/audio/read/state?dev=1")
    _, _, vv, _ = _api_get("/audio/read/volume?dev=1")
    _, _, eq, _ = _api_get("/audio/read/eq?dev=1")
    _, _, fc, _ = _api_get("/audio/read/fileCounts?dev=1")
    _, _, fn, _ = _api_get("/audio/read/fileNumber?dev=1")
    print("　　← 状況         = {}".format(st.get("state") if st else "NaN"))
    print("　　← 音量         = {}".format(vv.get("volume") if vv else "NaN"))
    print("　　← イコライザ   = {}".format(eq.get("eq") if eq else "NaN"))
    print("　　← 総ファイル数 = {}".format(fc.get("fileCounts") if fc else "NaN"))
    print("　　← 現在ファイル = {}".format(fn.get("fileNumber") if fn else "NaN"))

    _, _, j, _ = _api_get("/audio/play/pause?dev=1")
    _, _, st, _ = _api_get("/audio/read/state?dev=1")
    print("　・一時停止 : {} : 状況 = {}".format(tf(j and j.get("ok")), st.get("state") if st else "NaN"))
    time.sleep(2.0)

    _, _, j, _ = _api_get("/audio/play/resume?dev=1")
    _, _, st, _ = _api_get("/audio/read/state?dev=1")
    print("　・再開 : {} : 状況 = {}".format(tf(j and j.get("ok")), st.get("state") if st else "NaN"))

    print("　・イコライザー")
    for mode in range(0, 6):
        _, _, j, _ = _api_get("/audio/setEq?dev=1&mode={}".format(mode))
        print("　　→ {} : {}".format(mode, tf(j and j.get("ok"))))
        time.sleep(3.0)

    print("　・音量")
    for v in range(0, 31, 5):
        _, _, j, _ = _api_get("/audio/volume?dev=1&value={}".format(v))
        print("　　→ {} : {}".format(v, tf(j and j.get("ok"))))
        time.sleep(1.0)

    _, _, j, _ = _api_get("/audio/play/stop?dev=1")
    _, _, st, _ = _api_get("/audio/read/state?dev=1")
    print("　・停止 : {} : 状況 = {}".format(tf(j and j.get("ok")), st.get("state") if st else "NaN"))
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

    STEP         = 6
    STEP_DELAY_S = 0.005
    PCA_ADDR     = 0x40

    def RunI2C(ch, ticks):
        base_reg = 0x06 + 4 * ch
        _api_get("/i2c/write?addr={}&reg={}&val={}".format(PCA_ADDR, base_reg + 2, (ticks     ) & 0xFF))
        _api_get("/i2c/write?addr={}&reg={}&val={}".format(PCA_ADDR, base_reg + 3, (ticks >> 8) & 0x0F))

    print("　・初期位置")
    if argMode:
        _api_get("/pwm/out?ch={}&val={}".format(CH_180, PWM_MID))
        _api_get("/pwm/out?ch={}&val={}".format(CH_360, PWM_MID))
    else:
        RunI2C(CH_180, PWM_MID)
        RunI2C(CH_360, PWM_MID)
    time.sleep(PAUSE_S)

    print("　・正転:加速")
    for pwmVal in range(PWM_MID, PWM_MAX, STEP):
        if argMode:
            _api_get("/pwm/out?ch={}&val={}".format(CH_180, pwmVal))
            _api_get("/pwm/out?ch={}&val={}".format(CH_360, pwmVal))
        else:
            RunI2C(CH_180, pwmVal)
            RunI2C(CH_360, pwmVal)
    time.sleep(PAUSE_S)

    print("　・逆転:減速")
    for pwmVal in range(PWM_MAX, PWM_MIN, -STEP):
        if argMode:
            _api_get("/pwm/out?ch={}&val={}".format(CH_180, pwmVal))
            _api_get("/pwm/out?ch={}&val={}".format(CH_360, pwmVal))
        else:
            RunI2C(CH_180, pwmVal)
            RunI2C(CH_360, pwmVal)
        time.sleep(STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・初期位置")
    if argMode:
        _api_get("/pwm/out?ch={}&val={}".format(CH_180, PWM_MID))
        _api_get("/pwm/out?ch={}&val={}".format(CH_360, PWM_MID))
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
        if mode : _api_get("/pwm/angle/out?ch={}&val={}".format(ch, pwmVal))
        else    : _api_get("/pwm/rotation/out?ch={}&val={}".format(ch, pwmVal))
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
    _api_get("/pwm/angle/init?from={}&to={}&rmax={}&pmin={}&pmax={}&pmid={}".format(CH, -1, ANGLE_MAX, PWM_MIN, PWM_MAX, -1))

    print("　・角度：０")
    _api_get("/pwm/angle/out?ch={}&val={}".format(CH, 0))
    time.sleep(PAUSE_S)

    print("　・角度：0度～最大")
    RunPwmSweep(MODE, CH, 0, ANGLE_MAX, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・角度：最大～0度")
    RunPwmSweep(MODE, CH, ANGLE_MAX, 0, STEP, STEP_DELAY_S)
    time.sleep(PAUSE_S)

    print("　・中心位置")
    _api_get("/pwm/angle/center?ch={}".format(CH)
    time.sleep(PAUSE_S)

    print("　・設定削除")
    _api_get("/pwm/angle/delete?from={}&to={}".format(CH, -1))
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
    _api_get("/pwm/rotation/init?from={}&to={}&pmin={}&pmax={}&pmid={}".format(CH, -1, PWM_MIN, PWM_MAX, -1))

    print("　・停止")
    MMP.接続.Pwm.Rotation.Stop(CH)
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
    _api_get("/pwm/rotation/stop?ch={}".format(CH)

    print("　・設定削除")
    _api_get("/pwm/rotation/delete?from={}&to={}".format(CH, -1)

    print("　[終了]\n")

#======================================================
if __name__ == "__main__":
    main()

