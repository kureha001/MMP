# -*- coding: utf-8 -*-
#============================================================
# ＭＭＰコマンド テスト（ＷＥＢ－ＡＰＩ経由で実行）
#------------------------------------------------------------
# シリアル通信・TCPブリッジに対応
#============================================================
# -*- coding: utf-8 -*-
# ============================================================
# ＭＭＰコマンド テスト（Web API 版）
# - 元の Python テストの「printログ」を維持
# - API の詳細ログ（URL/HTTP/JSON）は VERBOSE_API で表示/非表示を切替
# - 依存：標準ライブラリのみ（urllib）
# ============================================================

import json
import time
from urllib import request, error

# ====== 設定 ======
HOST = "192.168.2.124"
PORT = 8080

# API 詳細ログ（URL/HTTP/JSON）を表示するか？
VERBOSE_API = False   # ← True にすると詳細ログを出す。False で非表示（従来ログだけ）

#============================================================
# 共通ユーティリティ
#============================================================
def ConnInfo(argLogSW=False):
    for p in ("/conn/isOpen", "/conn/baud", "/conn/port", "/conn/lastError", "/info/version"):
        _api_get(p, argLog=argLogSW)

def _api_get(path, timeout=10.0, argLog=False):
    """GET /path を実行し、(ok, status, json_obj, text) を返す。
       VERBOSE_API が True のときは詳細ログも出力する。"""
    url = f"http://{HOST}:{PORT}{path}"
    body_text = ""
    status = 0
    ok = False
    j = None
    try:
        with request.urlopen(url, timeout=timeout) as resp:
            status = resp.status
            body_text = resp.read().decode("utf-8", errors="replace")
            ok = (200 <= status < 300)
            try:
                j = json.loads(body_text)
            except Exception:
                j = None
    except error.URLError as e:
        body_text = f"(URLError: {e})"
    except Exception as e:
        body_text = f"(ERROR: {e})"

    if VERBOSE_API or argLog:
        print(f"\n=== API ===\nGET {url}\nHTTP {status}")
        if j is not None:
            print(json.dumps(j, ensure_ascii=False, indent=2))
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
    # 実施するテストだけコメントアウトを外してください（複数可）
    # 実施するテストだけコメントを外してください（複数可）
    #RunAnalog()         # アナログ入出力
    #RunDigital()        # デジタル入出力
    #RunMp3Playlist()    # MP3プレイヤー(基本)
    #RunMp3Control()     # MP3プレイヤー(制御)
    RunPwm(True)        # PWM出力
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
            _, _, jj, _ = _api_get(f"/analog/read?ch={x}&dev={y}")
            v = jj.get("value") if jj and ("value" in jj) else "NaN"
            print(f"　　　← [{y}] = {v}")
    print("　[終了]\n")


#============================================================
# 2) デジタル入出力
#============================================================
def RunDigital():
    print("２.デジタル入出力（ GPIO ）")
    print("　・入力")
    for pin in (2, 6, 7):
        _, _, j, _ = _api_get(f"/digital/in?id={pin}")
        val = j.get("value") if j and ("value" in j) else 1
        print(f"　　←[{pin}] = {'ON' if val==0 else 'OFF'}")

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
        ok, _, j1, _ = _api_get(f"/audio/play/start?dev=1&dir=1&file={track}")
        _, _, j2, _ = _api_get("/audio/read/state?dev=1")
        st = j2.get("state") if j2 and ("state" in j2) else "NaN"
        print(f"　　→ F=1,T={track} : {tf(j1 and j1.get('ok'))}：状況 = {st}")
        time.sleep(3.0)

    _, _, j, _ = _api_get("/audio/play/stop?dev=1")
    _, _, jj, _ = _api_get("/audio/read/state?dev=1")
    st = jj.get("state") if jj and ("state" in jj) else "NaN"
    print(f"　・停止 : {tf(j and j.get('ok'))} : 状況 = {st}")

    _, _, j, _ = _api_get("/audio/play/start?dev=1&dir=2&file=102")
    print("　・再生 → F=2,T=102 : {}".format(tf(j and j.get("ok"))))

    _, _, j, _ = _api_get("/audio/play/setLoop?dev=1&mode=1")
    _, _, jj, _ = _api_get("/audio/read/state?dev=1")
    st = jj.get("state") if jj and ("state" in jj) else "NaN"
    print(f"　・ループ → ON : {tf(j and j.get('ok'))} : 状況 = {st}")
    time.sleep(10.0)

    _, _, j, _ = _api_get("/audio/play/stop?dev=1")
    _, _, jj, _ = _api_get("/audio/read/state?dev=1")
    st = jj.get("state") if jj and ("state" in jj) else "NaN"
    print(f"　・停止 : {tf(j and j.get('ok'))} : 状況 = {st}")
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
        _, _, j, _ = _api_get(f"/audio/setEq?dev=1&mode={mode}")
        print("　　→ {} : {}".format(mode, tf(j and j.get("ok"))))
        time.sleep(3.0)

    print("　・音量")
    for v in range(0, 31, 5):
        _, _, j, _ = _api_get(f"/audio/volume?dev=1&value={v}")
        print("　　→ {} : {}".format(v, tf(j and j.get("ok"))))
        time.sleep(1.0)

    _, _, j, _ = _api_get("/audio/play/stop?dev=1")
    _, _, st, _ = _api_get("/audio/read/state?dev=1")
    print("　・停止 : {} : 状況 = {}".format(tf(j and j.get("ok")), st.get("state") if st else "NaN"))
    print("　[終了]\n")


#============================================================
# 5) PWM 生値：ch0=180度型、ch15=連続回転型
# 6) I2C：サーボスイープ（PCA9685 レジスタ直書き）
#============================================================
def RunPwm(mode_pwm=True):
    title = "５.ＰＷＭ" if mode_pwm else "６.Ｉ２Ｃ"
    print("{}（ PCA9685：サーボモータ180度型,連続回転型 ）".format(title))

    SERVO_MIN = 150
    SERVO_MAX = 600
    SERVO_MID = (SERVO_MIN + SERVO_MAX) // 2
    OffsetMax360 = 60
    STEPS = 80
    STEP = 8
    STEP_DELAY_S = 0
    CH_180 = 0
    CH_360 = 15
    PCA_ADDR = 0x40  # 十進 64

    def RunI2C(ch, ticks):
        base_reg = 0x06 + 4 * ch
        _api_get(f"/i2c/write?addr={PCA_ADDR}&reg={base_reg+2}&val={(ticks     ) & 0xFF}")
        _api_get(f"/i2c/write?addr={PCA_ADDR}&reg={base_reg+3}&val={(ticks >> 8) & 0x0F}")

    print("　・初期位置")
    if mode_pwm:
        _api_get(f"/pwm/out?ch={CH_180}&val={SERVO_MID}")
        _api_get(f"/pwm/out?ch={CH_360}&val={SERVO_MID}")
    else:
        RunI2C(CH_180, SERVO_MID)
        RunI2C(CH_360, SERVO_MID)
    time.sleep(0.3)

    print("　・正転,加速")
    for i in range(0, STEPS + 1, STEP):
        pwm180 = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        pwm360 = SERVO_MID + (OffsetMax360 * i) // STEPS
        if mode_pwm:
            _api_get(f"/pwm/out?ch={CH_180}&val={pwm180}")
            _api_get(f"/pwm/out?ch={CH_360}&val={pwm360}")
        else:
            RunI2C(CH_180, pwm180)
            RunI2C(CH_360, pwm360)
        if STEP_DELAY_S:
            time.sleep(STEP_DELAY_S)

    print("　・逆転,減速")
    for i in range(STEPS, -1, -STEP):
        pwm180 = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        pwm360 = SERVO_MID + (OffsetMax360 * i) // STEPS
        if mode_pwm:
            _api_get(f"/pwm/out?ch={CH_180}&val={pwm180}")
            _api_get(f"/pwm/out?ch={CH_360}&val={pwm360}")
        else:
            RunI2C(CH_180, pwm180)
            RunI2C(CH_360, pwm360)
        if STEP_DELAY_S:
            time.sleep(STEP_DELAY_S)

    print("　・初期位置")
    if mode_pwm:
        _api_get(f"/pwm/out?ch={CH_180}&val={SERVO_MID}")
        _api_get(f"/pwm/out?ch={CH_360}&val={SERVO_MID}")
    else:
        RunI2C(CH_180, SERVO_MID)
        RunI2C(CH_360, SERVO_MID)

    print("　[終了]\n")

#======================================================
if __name__ == "__main__":
    main()
