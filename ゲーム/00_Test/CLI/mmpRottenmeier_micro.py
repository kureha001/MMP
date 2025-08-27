#======================================================
# ＭＭＰライブラリ Rottenmeier (MicroPython クライアント版)
#------------------------------------------------------
# Ver 0.03.000　2025/08/27 By Takanari.Kureha
#======================================================
from machine import UART, Pin
import time

#--------------------------------
# 例外
#--------------------------------
class ConnectException(Exception):
    pass


class mmp:
    #--------------------------------
    # 初期化：通信パラメータはコンストラクターで決定
    #--------------------------------
    def __init__(self, uart_id=1, tx_pin=0, rx_pin=1, baud=115200, timeout_ms=100):
        print("\n<<接続準備>>")
        self.uart = None
        self.接続済 = False
        self.version = ""

        # UART設定（constructorで確定）
        self.uart_id = int(uart_id)
        self.tx_pin = int(tx_pin)
        self.rx_pin = int(rx_pin)
        self.baud = int(baud)
        self.timeout_ms = int(timeout_ms)

        print("UART既定値を設定しました:")
        print(f" ・UART ID={self.uart_id}, TX={self.tx_pin}, RX={self.rx_pin}")
        print(f" ・{self.baud}bps, 8N1, timeout={self.timeout_ms}ms")

        # アナログパラメータ
        self.参加総人数 = None
        self.スイッチ数 = None
        self.丸め = None
        self.mmpAnaVal = []

        # PWM 状態
        self.PWM機器状況 = [False] * 16

    #--------------------------------
    # 内部：UART 5文字レスポンス受信
    #  - 先行ゴミは自然に捨て、常に「末尾が '!' の5文字」で返す
    #  - タイムアウト時は None を返す
    #--------------------------------
    def _read5(self):
        deadline = time.ticks_add(time.ticks_ms(), self.timeout_ms)
        buf = bytearray()
        while time.ticks_diff(deadline, time.ticks_ms()) > 0:
            if self.uart is None:
                break
            n = self.uart.any()
            if n:
                b = self.uart.read(1)
                if not b:
                    continue
                buf += b
                # 常に末尾5バイトだけ保持
                if len(buf) > 5:
                    buf = buf[-5:]
                if len(buf) == 5 and buf[-1] == 0x21:  # '!'
                    try:
                        return buf.decode("ascii")
                    except Exception:
                        return "".join(chr(x) for x in buf)
        return None

    #--------------------------------
    # 内部：送受信（5文字固定）
    #--------------------------------
    def _txrx5(self, s):
        if self.uart is None:
            return None
        try:
            self.uart.write(s.encode("ascii"))
        except Exception:
            return None
        return self._read5()

    #--------------------------------
    # 接続（GPIO UART固定 / 自動スキャンなし）
    #  ※ 通信パラメータは __init__ で設定済み
    #--------------------------------
    def 通信接続(self):
        print("UART設定:", f"ID={self.uart_id}, TX={self.tx_pin}, RX={self.rx_pin}, {self.baud}bps, 8N1, timeout={self.timeout_ms}ms")
        try:
            self.uart = UART(
                self.uart_id,
                baudrate=self.baud,
                bits=8,
                parity=None,
                stop=1,
                tx=Pin(self.tx_pin),
                rx=Pin(self.rx_pin),
                timeout=self.timeout_ms,
                timeout_char=max(1, self.timeout_ms // 10)
            )
        except Exception as e:
            raise ConnectException(f"UART初期化に失敗: {e}")

        # 小休止後、VERで生存確認（失敗なら例外）
        time.sleep(0.1)
        print("ファームウェア応答確認: 'VER!' を送信します")
        raw = self._txrx5("VER!")
        if raw is None or len(raw) != 5 or raw[-1] != "!":
            self.uart = None
            self.接続済 = False
            raise ConnectException("ファームウェア応答なし（VER!）")
        print(f"受信データ: '{raw}'")

        # "xyzz!" → "x.y.zz"
        try:
            sdata = raw[:-1]
            self.version = f"{sdata[0]}.{sdata[1]}.{sdata[2:4]}"
        except Exception:
            self.version = "?.?.??"

        print(f"接続済み(Ver.{self.version})")
        self.接続済 = True
        return True

    #--------------------------------
    def 通信切断(self):
        print("\n<<切断>>")
        try:
            if self.uart:
                self.uart.deinit()
        except Exception:
            pass
        self.uart = None
        self.接続済 = False
        print("切断完了")

    #--------------------------------
    def バージョン(self):
        print("バージョン照会: 'VER!' を送信します")
        resp = self._txrx5("VER!")
        if resp is None or len(resp) != 5 or resp[-1] != "!":
            print("バージョン取得失敗")
            return ""
        print(f"受信データ: '{resp}'")
        s = resp[:-1]
        結果 = f"{s[0]}.{s[1]}.{s[2:4]}"
        self.version = 結果
        return 結果

    #======================================================
    # アナログ入力
    #======================================================
    def アナログ初期化(self):
        print("アナログ測定バッファ初期化")
        tmpスイッチ = []
        for _ in range(self.スイッチ数):
            tmpスイッチ.append(0)
        self.mmpAnaVal = []
        for _ in range(self.参加総人数):
            self.mmpAnaVal.append(tmpスイッチ.copy())

    def アナログ設定(self, argスイッチ数=4, arg参加人数=1, arg丸め=5):
        print("<<アナログ入力準備>>")
        self.参加総人数 = int(arg参加人数)
        self.スイッチ数 = int(argスイッチ数)
        self.丸め = int(arg丸め)

        print(" ・HC4067 使用個数     : %d" % self.スイッチ数)
        print(" ・HC4067 使用ポート数 : %d" % self.参加総人数)
        print(" ・アナログ値の丸め処理: %d" % self.丸め)

        self.アナログ初期化()

        cmd = f"ANS:{self.参加総人数:02X}:{self.スイッチ数:02X}!"
        print(cmd)
        _ = self._txrx5(cmd)  # 成否は以降のANU/ANRで検出

    def アナログ読取(self):
        print("アナログ値更新: 'ANU!' → 'ANR' 一括走査")
        _ = self._txrx5("ANU!")
        for pl in range(self.参加総人数):
            for sw in range(self.スイッチ数):
                cmd = f"ANR:{pl:02X}:{sw:02X}!"
                resp = self._txrx5(cmd)
                if resp and len(resp) == 5 and resp[-1] == "!":
                    try:
                        val = int(resp[:-1], 16)
                        if self.丸め > 0:
                            val = (val // self.丸め) * self.丸め
                        self.mmpAnaVal[pl][sw] = val
                    except Exception:
                        print(f"[警告] ANR応答の数値化に失敗: {resp}")

    #======================================================
    # PWM（PCA9685）
    #======================================================
    def PWM_機器確認(self):
        print("PWM_機器確認: PWX を 0..15 へ問い合わせ")
        状況 = [False] * 16
        for i in range(16):
            resp = self._txrx5(f"PWX:{i:02X}!")
            ok = False
            if resp and len(resp) == 5 and resp[-1] == "!":
                try:
                    ok = (int(resp[:-1], 16) & 0x1) == 1
                except Exception:
                    ok = False
            状況[i] = ok
        self.PWM機器状況 = 状況
        print("PWM機器状況:", 状況)
        return 状況

    def PWM_チャンネル使用可否(self, channel):
        if not (0 <= channel <= 255):
            raise ValueError("[エラー] PWMチャンネルは 0〜255 の範囲で指定")
        if not self.PWM機器状況 or len(self.PWM機器状況) != 16:
            self.PWM_機器確認()
        pwm_id = channel // 16  # 0..15
        可 = bool(self.PWM機器状況[pwm_id])
        print(f"PWM_チャンネル使用可否 ch={channel} → {可}")
        return 可

    def PWM_VALUE(self, argPort, argPWM):
        cmd = f"PWM:{int(argPort):02X}:{int(argPWM):04X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd)
        if resp is None:
            print("[警告] PWM_VALUE 応答なし")

    def PWM_INIT(self, rs, re, ps, pe):
        # RS/RE/PS/PE の4引数方式（将来の per-ch 拡張を見据え）
        cmd = f"PWI:{int(rs):03X}:{int(re):03X}:{int(ps):04X}:{int(pe):04X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd)
        if resp is None:
            print("[警告] PWM_INIT 応答なし")

    def PWM_ANGLE(self, argPort, argAngle):
        cmd = f"PWA:{int(argPort):02X}:{int(argAngle):03X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd)
        if resp is None:
            print("[警告] PWM_ANGLE 応答なし")

    #======================================================
    # デジタル I/O
    #======================================================
    def digital_IO(self, argPort, argValue):
        cmd = f"IO:{int(argPort):02X}:{int(argValue):01X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd)
        if resp and len(resp) == 5 and resp[-1] == "!":
            try:
                val = int(resp[:-1], 16)
                print("受信:", resp, "→", val)
                return val
            except Exception:
                print("[警告] digital_IO 応答の数値化に失敗:", resp)
                return 0
        print("[警告] digital_IO 応答異常:", resp)
        return 0

    def digital_OUT(self, argPort, argValue):
        cmd = f"POW:{int(argPort):02X}:{int(argValue):01X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd)
        if resp == "!!!!!":
            print("受信: !!!!! (OK)")
            return True
        else:
            print("[警告] digital_OUT 応答異常:", resp)
            return False

    def digital_IN(self, argPort):
        cmd = f"POR:{int(argPort):02X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd)
        if resp and len(resp) == 5 and resp[-1] == "!":
            try:
                val = int(resp[:-1], 16)
                print("受信:", resp, "→", val)
                return val
            except Exception:
                print("[警告] digital_IN 応答の数値化に失敗:", resp)
                return 0
        print("[警告] digital_IN 応答異常:", resp)
        return 0

    #（互換用・未使用）サーバは '!' 終端なので行単位ではない
    def readline(self):
        print("readline 呼び出し（互換用）")
        s = ""
        while True:
            resp = self._read5()
            if resp is None:
                break
            if resp.endswith("!"):
                s = resp[:-1]
                break
        return s

    #======================================================
    # MP3（DFPlayer mini）
    #======================================================
    def DFP_Info(self, 引数_devNo):
        cmd = f"DPX:{int(引数_devNo):01X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd) or ""
        print("受信:", resp)
        return resp

    def DFP_PlayFolderTrack(self, 引数_devNo, folder, track):
        cmd = f"DIR:{int(引数_devNo):01X}:{int(folder):02X}:{int(track):02X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd) or ""
        print("受信:", resp)
        return resp

    def DFP_get_play_state(self, 引数_devNo, 引数_stNo):
        cmd = f"DST:{int(引数_devNo):01X}:{int(引数_stNo):01X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd) or ""
        print("受信:", resp)
        return resp

    def DFP_Stop(self, 引数_devNo):
        cmd = f"DSP:{int(引数_devNo):01X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd) or ""
        print("受信:", resp)
        return resp

    def DFP_Pause(self, 引数_devNo):
        cmd = f"DPA:{int(引数_devNo):01X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd) or ""
        print("受信:", resp)
        return resp

    def DFP_Resume(self, 引数_devNo):
        cmd = f"DPR:{int(引数_devNo):01X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd) or ""
        print("受信:", resp)
        return resp

    def DFP_Volume(self, 引数_devNo, vol):
        cmd = f"VOL:{int(引数_devNo):01X}:{int(vol):02X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd) or ""
        print("受信:", resp)
        return resp

    def DFP_set_eq(self, 引数_devNo, eq_mode):
        cmd = f"DEF:{int(引数_devNo):01X}:{int(eq_mode):02X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd) or ""
        print("受信:", resp)
        return resp

    #======================================================
    # I2C（プロキシ）
    #======================================================
    def i2cWrite(self, slave_addr, addr, val):
        cmd = f"I2W:{int(slave_addr):02X}:{int(addr):02X}:{int(val):02X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd)
        print("受信:", resp)
        return resp == "!!!!!"

    def i2cRead(self, slave_addr, addr):
        cmd = f"I2R:{int(slave_addr):02X}:{int(addr):02X}!"
        print("送信:", cmd)
        resp = self._txrx5(cmd)
        print("受信:", resp)
        if resp and len(resp) == 5 and resp[-1] == "!":
            try:
                return int(resp[:-1], 16)
            except Exception:
                print("[警告] i2cRead 応答の数値化に失敗:", resp)
                return 0x00
        print("[警告] i2cRead 応答異常:", resp)
        return 0x00
