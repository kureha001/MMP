# -*- coding: utf-8 -*-
#============================================================
# ＭＭＰコマンド テスト（シリアル コマンドを直接実行）
# CPython用
# パソコン／Android(Pydroid])対応
# このファイル単独で実行できます。
#============================================================
import time
import serial

_UART = None

# usbserial4aのインポート
try:
    import kivy
    from usb4a       import usb
    from usbserial4a import serial4a
    _HAS_USB4A  = True
except Exception:
    usb         = None
    serial4a    = None
    _HAS_USB4A  = False

#=============== プラットフォーム別ポート定義 ===============
# 例）"usb4a://<index>"   <index>:0,1,....
_NAME = "usb4a://0"                 # Pydroid
#------------------------------------------------------------
# 例）"socket://192.168.2.50:3331" … ser2net(raw TCP)
#     "rfc2217://192.168.2.50:3331"… ser2net(RFC2217)
#_NAME = "socket://127.0.0.1:5333"   # Pydroid
#------------------------------------------------------------
# 例）"/dev/ttyACM0" / "COM3" … 直接接続
#_NAME = "COM85"                     # Windows
#_NAME = "/dev/ttyACM0"              # Linux
#_NAME = "dev/cu.usbmodem1101"       # MacOS
#============================================================

#============================================================
# ボーレート
#============================================================
BAUD_CANDIDATES = (
    921600  ,
    115200  ,
    230400  ,
    9600    ,
    4800    ,
    2400    ,
    1200    ,
    300     ,
)

#============================================================
# メイン（共通）
#============================================================
def main():

#    if not ConnectWithBaud(115200):     # 通信速度を指定
    if not ConnectAutoBaud():           # 通信速度を自動検出
        print("接続失敗")
        return False

    print("\n＝＝＝ ＭＭＰ ＡＰＩテスト［開始］＝＝＝\n")
    #RunAnalog()         # アナログ入出力
    #RunDigital()        # デジタル入出力
    #RunMp3Playlist()    # MP3プレイヤー(基本)
    #RunMp3Control()     # MP3プレイヤー(制御)
    #RunPwm(True)        # PWM出力
    #RunPwm(False)       # I2C→PCA9685 直接制御
    _UART.close()
    print("＝＝＝ ＭＭＰ ＡＰＩテスト［終了］＝＝＝")


#============================================================
# シリアル関連
#============================================================
#-------------------
# 接続：条件指定
#-------------------
def ConnectWithBaud(
    argBaud     # ① 通信速度(単位：bps)
) -> bool:      # 戻り値：True=成功／False=失敗

    global _HAS_USB4A
    global _NAME
    global _UART
    try:
        # １．usb4a（Android直結）
        if _NAME.startswith("usb4a://"):
            if not _HAS_USB4A: raise RuntimeError("この環境はusb4a/usbserial4a対象外")

            # 形式: usb4a://<index>   例) usb4a://0
            idx_str = _NAME.split("://", 1)[1]
            index   = int(idx_str) if idx_str else 0

            # デバイス列挙
            devs = usb.get_usb_device_list()
            dev  = devs[index]

            # 権限が無ければ要求（初回はダイアログ）
                # 許可はユーザ操作次第。
                # ここでは待たず、そのまま open を試みる
                # （失敗時は except に落ちて False を返す＝既存仕様）
            if not usb.has_usb_permission(dev):
                usb.request_usb_permission(dev)

            # device_name は UsbDevice の getDeviceName()
            device_name = dev.getDeviceName()

            # ポートを開く（作者サンプルどおり get_serial_port を使用）
            _UART = serial4a.get_serial_port(
                device_name,
                argBaud,               # Baudrate
                8,                     # Data bits
                'N',                   # Parity
                1                      # Stop bits
            )

        # ２．TCPブリッジ
        elif (_NAME.startswith("socket://")) or (_NAME.startswith("rfc2217://")):
            _UART = serial.serial_for_url(_NAME)

        # ３．通常の物理ポート
        else:
            _UART = serial.Serial(
                _NAME,
                argBaud,
                timeout = 0,
            )

        # 受信バッファをクリア
        time.sleep(0.01)
        MmpOpenFlush()

        # バージョン確認（戻り5文字＋'!'前提）
        resp = MmpCMD("VER!")
        if len(resp) != 5 or not resp.endswith('!'): return False

        print(f"　・通信ポート　: {_NAME}")
        print(f"　・通信速度　　: {argBaud}bps")
        print(f"　・バージョン  : {resp}")
        print( "　・PCA9685 [0] : 0x{:04X}".format(MmpCMD_VAL("PWX:0!")))
        print( "　・DFPlayer[1] : 0x{:04X}".format(MmpCMD_VAL("DPX:1!")))
        return True

    except Exception as e:
        print(e)
        return False

#------------------------------
# 自動接続
#------------------------------
def ConnectAutoBaud(
    candidates = BAUD_CANDIDATES    # ①通信速度一覧
) -> bool:                          # 戻り値：True=成功／False=失敗

    # 通信速度一覧の通信速度を用い、総当たりで接続を試みる。
    for b in candidates:
        if ConnectWithBaud(b): return True  # 戻り値 → [成功]
    return False                            # 戻り値 → [失敗]

#------------------------------
# 受信バッファを消去する
#------------------------------
def MmpOpenFlush():

    # シリアルが無効な場合は処理を中断
    global _UART
    if not _UART: return

    # バッファのデータ量を参照
    try             : n = _UART.in_waiting
    except Exception: n = 0

    # バッファに溜まっている分を空読み
    if n:
        try             : _UART.read(n)
        except Exception: pass

#============================================================
# コマンド実行
#============================================================
def MmpCMD(
    cmd: str,               # ① 送信コマンド文字列
    timeout_ms: int = 400,  # ② タイムアウト(ミリ秒)
) -> str:                   # 戻り値：""以外=コマンドの戻り値／""=引数違反
    # シリアルが無効な場合は処理を中断
    global _UART
    if not _UART: return ""

    # 入力バッファを消去→コマンド送信
    MmpOpenFlush()
    time.sleep(0.05)

    try:
        _UART.write(cmd.encode("ascii", "ignore"))
        _UART.flush()
    except Exception:
        return ""

    # 5文字の戻り値を取得（末尾'!'想定）
    resp = ""
    start_ms = int(time.monotonic() * 1000)
    while len(resp) < 5 and (int(time.monotonic() * 1000) - start_ms) < timeout_ms:
        try:
            b = _UART.read(1)           # timeout=0想定：非ブロッキング
        except Exception:
            b = b""

        if b:
            try:
                ch = b.decode("ascii", "ignore")
            except Exception:
                ch = ""
            if ch:
                resp += ch
                if len(resp) >= 5:
                    break
        else:
            time.sleep(0.001)           # 軽いポーリング待ち

    # 形式チェック（5文字かつ末尾'!'）
    if len(resp) == 5 and resp.endswith('!'):
        return resp
    return ""

#------------------------------
# コマンド実行ラッパー：BOOL型
#------------------------------
def MmpCMD_OK(argCMD):
    resp = MmpCMD(argCMD)
    return resp == "!!!!!"

#------------------------------
# コマンド実行ラッパー：BOOL型
#------------------------------
def MmpCMD_VAL(argCMD):
    resp = MmpCMD(argCMD)
    ok, val = _ReturnHex4(resp); 
    return val if ok else -1

#========================
# ヘルパ
#========================
# 戻り値を True/False に変換
def tf(b): return "True" if b>0 else "False"

# HEX値変換ヘルパ(個別桁)
def _hex1(v): return f"{v & 0xF:01X}"
def _hex2(v): return f"{v & 0xFF:02X}"
def _hex3(v): return f"{v & 0x3FF:03X}"
def _hex4(v): return f"{v & 0xFFFF:04X}"

# 戻り値 HEX値変換ヘルパ(4桁)
def _ReturnHex4(s: str):
    if s == "!!!!!!": return False, 0
    try             : return True , int(s[:4], 16)
    except Exception: return False, 0

#============================================================
# 1) アナログ入力(HC4067)
#============================================================
def RunAnalog():

    print("１.アナログ入力（ HC4067：JoyPad1,2 ）")

    ok = MmpCMD_OK("ANS:2:4!")
    print("　・アクセス範囲指定 → [2,4]  : {}".format(tf(ok)))
    if not ok:
        print("　 <<中断>>\n")
        return

    ok = MmpCMD_OK("ANU!")
    print("　・アナログ値をバッファに格納 : {}".format(tf(ok)))
    if not ok:
        print("　 <<中断>>\n")
        return

    print("　・バッファを参照")
    for x in range(0, 2):
        print("　　JoyPad[{}]".format(x + 1))
        for y in range(0, 4):
            print("　　　← [{}] = {}".format(y, MmpCMD_VAL(f"ANR:{x}:{y}!")))

    print("　[終了]\n")


#============================================================
# 2) デジタル入出力
#============================================================
def RunDigital():
    print("２.デジタル入出力（ GPIO ）")
    print("　・入力")
    print("　　←[2] = {}".format("ON" if MmpCMD_VAL("POR:2!") == 0 else "OFF"))
    print("　　←[6] = {}".format("ON" if MmpCMD_VAL("POR:6!") == 0 else "OFF"))
    print("　　←[7] = {}".format("ON" if MmpCMD_VAL("POR:7!") == 0 else "OFF"))

    print("　・出力[3]")
    for _ in range(6):
        print("　　→ HIGH : {}".format(tf(MmpCMD_OK("POW:3:1!")))); time.sleep(0.5)
        print("　　→ LOW  : {}".format(tf(MmpCMD_OK("POW:3:0!")))); time.sleep(0.5)
    print("　[終了]\n")


#============================================================
# 3) MP3：フォルダ1のトラック再生,リピート再生
#============================================================
def RunMp3Playlist():

    print("３.ＭＰ３再生（ DFPlayer ）")

    print("　・音量 → 20 : {}".format   (tf(MmpCMD_OK("VOL:1:20!"))))
    print("　・ループ → OFF : {}".format(tf(MmpCMD_OK("DLP:1:0!"))))

    print("　・再生")
    for track in range(1, 4):

        ok = MmpCMD_OK(f"DIR:1:1:{track}!")
        print("　　→ F=1,T={} : {}：状況 = {}".format(track, tf(ok), MmpCMD_VAL("DST:1:1!")))
        time.sleep(3.0)

    ok = MmpCMD_OK("DSP:1!")
    print("　・停止 : {} : 状況 = {}".format(tf(ok), MmpCMD_VAL("DST:1:1!")))

    ok = MmpCMD_OK("DIR:1:2:102!")
    print("　・再生 → F=2,T=102 : {} : 状況 = {}".format(tf(ok), MmpCMD_VAL("DST:1:1!")))

    ok = MmpCMD_OK("DLP:1:1!")
    print("　・ループ → ON : {} : 状況 = {}".format(tf(ok), MmpCMD_VAL("DST:1:1!")))
    time.sleep(10.0)

    ok = MmpCMD_OK("DSP:1!")
    print("　・停止 : {} : 状況 = {}".format(tf(ok), MmpCMD_VAL("DST:1!")))
    print("　[終了]\n")


#============================================================
# 4) MP3 制御：状態取得/一時停止/再開/停止/EQ/音量
#============================================================
def RunMp3Control():

    print("４.ＭＰ３制御（ DFPlayer ）")

    print("　・音量 → 20 : {}".format     (tf(MmpCMD_OK("VOL:1!20!" ))))
    print("　・再生 → F=4,T=1 : {}".format(tf(MmpCMD_OK("DIR:1:4:1!"))))
    print("　・ループ → OFF : {}".format  (tf(MmpCMD_OK("DLP:1!0!"  ))))

    print("　・参照")
    print("　　← 状況         = {}".format(MmpCMD_VAL("DST:1:1!")))
    print("　　← 音量         = {}".format(MmpCMD_VAL("DST:1:2!")))
    print("　　← イコライザ   = {}".format(MmpCMD_VAL("DST:1:3!")))
    print("　　← 総ファイル数 = {}".format(MmpCMD_VAL("DST:1:4!")))
    print("　　← 現在ファイル = {}".format(MmpCMD_VAL("DST:1:5!")))
    time.sleep(1.0)

    ok = MmpCMD_OK("DPA:1!")
    print("　・一時停止 : {} : 状況 = {}".format(tf(ok), MmpCMD_VAL("DST:1:1!")))
    time.sleep(2.0)

    ok = MmpCMD_OK("DPR:1!")
    print("　・再開 : {} : 状況 = {}".format(tf(ok), MmpCMD_VAL("DST:1:1!")))

    print("　・イコライザー")
    for mode in range(0, 6):
        print("　　→ {} : {}".format(mode, tf(MmpCMD_OK(f"DEF:1:{mode}!"))))
        time.sleep(3.0)

    print("　・音量")
    for v in range(0, 31, 5):
        print("　　→ {} : {}".format(v, tf(MmpCMD_OK(f"VOL:1:{v}!"))))
        time.sleep(1.0)

    ok = MmpCMD_OK("DSP:1!")
    print("　・停止 : {} : 状況 = {}".format(tf(ok), MmpCMD_VAL("DST:1:1!")))

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
    STEP         = 1
    STEP_DELAY_S = 0.01
    CH_180       = 0
    CH_360       = 15
    PCA_ADDR     = 0x40

    def RunI2C(ch, ticks):
        base_reg  = 0x06 + 4 * ch
        MmpCMD_OK(f"I2W:{_hex2(PCA_ADDR)}:{_hex2(base_reg + 2)}:{_hex2((ticks     ) & 0xFF)}!")
        MmpCMD_OK(f"I2W:{_hex2(PCA_ADDR)}:{_hex2(base_reg + 3)}:{_hex2((ticks >> 8) & 0x0F)}!")

    print("　・初期位置")
    if mode:
        MmpCMD_OK(f"PWM:{_hex2(CH_180)}:{_hex4(SERVO_MID)}!")
        MmpCMD_OK(f"PWM:{_hex2(CH_360)}:{_hex4(SERVO_MID)}!")
    else:
        RunI2C(CH_180, SERVO_MID)
        RunI2C(CH_360, SERVO_MID)
    time.sleep(0.3)

    print("　・正転,加速")
    for i in range(0, STEPS + 1,STEP):
        pwm180 = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        pwm360 = SERVO_MID + (OffsetMax360 * i)          // STEPS
        if mode:
            MmpCMD_OK(f"PWM:{_hex2(CH_180)}:{_hex4(pwm180)}!")
            MmpCMD_OK(f"PWM:{_hex2(CH_360)}:{_hex4(pwm360)}!")
        else:
            RunI2C(CH_180, pwm180)
            RunI2C(CH_360, pwm360)
        time.sleep(STEP_DELAY_S)

    print("　・逆転,減速")
    for i in range(STEPS, -1, -STEP):
        pwm180 = SERVO_MIN + (SERVO_MAX - SERVO_MIN) * i // STEPS
        pwm360 = SERVO_MID + (OffsetMax360 * i)          // STEPS
        if mode:
            MmpCMD_OK(f"PWM:{_hex2(CH_180)}:{_hex4(pwm180)}!")
            MmpCMD_OK(f"PWM:{_hex2(CH_360)}:{_hex4(pwm360)}!")
        else:
            RunI2C(CH_180, pwm180)
            RunI2C(CH_360, pwm360)
        time.sleep(STEP_DELAY_S)

    print("　・初期位置")
    if mode:
        MmpCMD_OK(f"PWM:{_hex2(CH_180)}:{_hex4(SERVO_MID)}!")
        MmpCMD_OK(f"PWM:{_hex2(CH_360)}:{_hex4(SERVO_MID)}!")
    else:
        RunI2C(CH_180, SERVO_MID)
        RunI2C(CH_360, SERVO_MID)

    print("　[終了]\n")

#======================================================
if __name__ == "__main__": main()