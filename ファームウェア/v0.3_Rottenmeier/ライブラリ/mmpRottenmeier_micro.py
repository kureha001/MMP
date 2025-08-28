#======================================================
# ＭＭＰライブラリ Rottenmeier (MicroPython版)
#------------------------------------------------------
# Ver 0.03.000　2025/08/28 By Takanari.Kureha
#   ・初版作成
#======================================================
from machine import UART, Pin
import time

#------------------------------------------------------
# 例外
#------------------------------------------------------
class ConnectException(Exception):
    print("ConnectException")


class mmp:
    #------------------------------------------------------
    # 初期化：通信パラメータはコンストラクターで決定
    #------------------------------------------------------
    def __init__(self, uart_id=0, tx_pin=0, rx_pin=1, baud=115200, timeout_ms=100):

        print("<<Initializing...>>")
        self.uart = None
        self.接続済 = False
        self.version = ""

        # UART設定（constructorで確定）
        self.uart_id = int(uart_id)
        self.tx_pin = int(tx_pin)
        self.rx_pin = int(rx_pin)
        self.baud = int(baud)
        self.timeout_ms = int(timeout_ms)
        print(f" - UART ID  = {self.uart_id}")
        print(f" - TX       = {self.tx_pin}")
        print(f" - RX       = {self.rx_pin}")
        print(f" - baudrate = {self.baud}bps")
        print(f" - Bits     = 8")
        print(f" - Parity   = None")
        print(f" - Stop     = 1")
        print(f" - timeout  = {self.timeout_ms}ms")

        # アナログパラメータ
        self.参加総人数 = 0
        self.スイッチ数 = 0
        self.丸め = 0
        self.mmpAnaVal = []

        # PWM 状態
        self.PWM機器状況 = [False] * 16

    #------------------------------------------------------
    # 内部：UART 5文字レスポンス受信
    #  - 先行ゴミは自然に捨て、常に「末尾が '!' の5文字」で返す
    #  - タイムアウト時は None を返す
    #------------------------------------------------------
    def _read5(self):

        deadline = time.ticks_add(time.ticks_ms(), self.timeout_ms)
        buf = bytearray()

        while time.ticks_diff(deadline, time.ticks_ms()) > 0:

            # UARTが初期化されていない場合は終了
            if self.uart is None: break

            # 受信データがない場合は待つ
            n = self.uart.any()
            if not n: continue

            # 1バイト受信
            b = self.uart.read(1)
            if not b: continue

            # 常に末尾5バイトだけ保持
            buf += b
            if len(buf) > 5: buf = buf[-5:]
            if len(buf) != 5 or buf[-1] != 0x21: continue # 0x21='!'

            return buf.decode("ascii")

        return None

    #------------------------------------------------------
    # 内部：送受信（5文字固定）
    #------------------------------------------------------
    def _txrx5(self, s):
        if self.uart is None: return None
        try             : self.uart.write(s.encode("ascii"))
        except Exception: return None
        return self._read5()

    #------------------------------------------------------
    # 接続（GPIO UART固定 / 自動スキャンなし）
    #------------------------------------------------------
    def 通信接続(self):
        print("<<Connecting...>>")
        try:
            self.uart = UART(
                self.uart_id                     ,
                baudrate     = self.baud         ,
                bits         = 8                 ,
                parity       = None              ,
                stop         = 1                 ,
                tx           = Pin(self.tx_pin)  ,
                rx           = Pin(self.rx_pin)  ,
                timeout      = self.timeout_ms   ,
                timeout_char = max(1, self.timeout_ms // 10)
                )
        except Exception as e:
            raise ConnectException(f"UART初期化に失敗: {e}")

        # 小休止後、VERで生存確認（失敗なら例外）
        time.sleep(0.1)
        raw = self._txrx5("VER!")
        if raw is None or len(raw) != 5 or raw[-1] != "!":
            self.uart = None
            self.接続済 = False
            raise ConnectException("ファームウェア応答なし[VER!]")

        try:
            sdata = raw[:-1]
            self.version = f"{sdata[0]}.{sdata[1]}.{sdata[2:4]}"
        except Exception: self.version = "?.?.??"

        print("<<Connected>>")
        self.接続済 = True
        return True
    #------------------------------------------------------
    def 通信切断(self):
        try:
            if self.uart: self.uart.deinit()
        except Exception: pass
        self.uart = None
        self.接続済 = False
        print("<<Disconnected>>")
    #------------------------------------------------------
    def バージョン(self):
        resp = self._txrx5("VER!")
        if resp is None or len(resp) != 5 or resp[-1] != "!": return ""
        s = resp[:-1]
        結果 = f"{s[0]}.{s[1]}.{s[2:4]}"
        self.version = 結果
        return 結果

    #======================================================
    # アナログ入力
    #======================================================
    def アナログ初期化(self):
        # 未初期化はエラー
        # 参加総人数 × スイッチ数 の 2次元配列を 0 で確保
        if self.スイッチ数 <= 0 or self.参加総人数 <= 0:
            raise ValueError("アナログ設定が未完了です。")
        self.mmpAnaVal = [[0 for _ in range(self.スイッチ数)] for _ in range(self.参加総人数)]
    #------------------------------------------------------
    def アナログ設定(self, argスイッチ数=4, arg参加人数=1, arg丸め=5):
        print("<<Set Analog Input Parameters>>")
        self.参加総人数 = int(arg参加人数)
        self.スイッチ数 = int(argスイッチ数)
        self.丸め = int(arg丸め)
        print("  - Switches     = %d" % self.スイッチ数)
        print("  - Participants = %d" % self.参加総人数)
        print("  - Rounding     = %d" % self.丸め)
        self.アナログ初期化()
        cmd = f"ANS:{self.参加総人数:02X}:{self.スイッチ数:02X}!"
        print(cmd)
        _ = self._txrx5(cmd)  # 成否は以降のANU/ANRで検出
    #------------------------------------------------------
    def アナログ読取(self):
        _ = self._txrx5("ANU!")
        for pl in range(self.参加総人数):
            for sw in range(self.スイッチ数):
                cmd = f"ANR:{pl:02X}:{sw:02X}!"
                resp = self._txrx5(cmd)
                if resp and len(resp) == 5 and resp[-1] == "!":
                    val = int(resp[:-1], 16)
                    if self.丸め > 0: val = (val // self.丸め) * self.丸め
                    self.mmpAnaVal[pl][sw] = val

    #======================================================
    # PWM（PCA9685）
    #======================================================
    def PWM_機器確認(self):
        状況 = [False] * 16
        for i in range(16):
            resp = self._txrx5(f"PWX:{i:02X}!")
            ok = False
            if resp and len(resp) == 5 and resp[-1] == "!":
                try             : ok = (int(resp[:-1], 16) & 0x1) == 1
                except Exception: ok = False
            状況[i] = ok
        self.PWM機器状況 = 状況
        return 状況
    #------------------------------------------------------
    def PWM_チャンネル使用可否(self, channel):
        if not (0 <= channel <= 255):
            raise ValueError("[エラー] Ch:0〜255 の範囲で指定")
        if not self.PWM機器状況 or len(self.PWM機器状況) != 16:
            self.PWM_機器確認()
        pwm_id = channel // 16  # 0..15
        可否 = bool(self.PWM機器状況[pwm_id])
        return 可否 
    #------------------------------------------------------
    def PWM_VALUE(self, argPort, argPWM):
        cmd = f"PWM:{int(argPort):02X}:{int(argPWM):04X}!"
        resp = self._txrx5(cmd)
        if resp is None: return False
        return resp == "!!!!!"
    #------------------------------------------------------
    def PWM_INIT(self, rs, re, ps, pe):
        cmd = f"PWI:{int(rs):03X}:{int(re):03X}:{int(ps):04X}:{int(pe):04X}!"
        resp = self._txrx5(cmd)
        if resp is None: return False
        return resp == "!!!!!"
    #------------------------------------------------------
    def PWM_ANGLE(self, argPort, argAngle):
        cmd = f"PWA:{int(argPort):02X}:{int(argAngle):03X}!"
        resp = self._txrx5(cmd)
        if resp is None: return False
        return resp == "!!!!!"

    #======================================================
    # デジタル I/O
    #======================================================
    def digital_IO(self, argPort, argValue):
        cmd = f"IO:{int(argPort):02X}:{int(argValue):01X}!"
        resp = self._txrx5(cmd)
        if resp and len(resp) == 5 and resp[-1] == "!":
            try:
                val = int(resp[:-1], 16)
                return val
            except Exception: return 0
        return 0
    #------------------------------------------------------
    def digital_OUT(self, argPort, argValue):
        cmd = f"POW:{int(argPort):02X}:{int(argValue):01X}!"
        resp = self._txrx5(cmd)
        if resp == "!!!!!": return True
        else              : return False
    #------------------------------------------------------
    def digital_IN(self, argPort):
        cmd = f"POR:{int(argPort):02X}!"
        resp = self._txrx5(cmd)
        if resp and len(resp) == 5 and resp[-1] == "!":
            try:
                val = int(resp[:-1], 16)
                return val
            except Exception: return 0
        return 0
    #------------------------------------------------------
    #（互換用・未使用）サーバは '!' 終端なので行単位ではない
    def readline(self):
        print("readline 呼び出し（互換用）")
        s = ""
        while True:
            resp = self._read5()
            if resp is None: break
            if resp.endswith("!"):
                s = resp[:-1]
                break
        return s

    #======================================================
    # MP3（DFPlayer mini）
    #======================================================
    def DFP_Info(self, 引数_devNo):
        cmd = f"DPX:{int(引数_devNo):01X}!"
        resp = self._txrx5(cmd) or ""
        return resp
    #------------------------------------------------------
    def DFP_PlayFolderTrack(self, 引数_devNo, folder, track):
        cmd = f"DIR:{int(引数_devNo):01X}:{int(folder):02X}:{int(track):02X}!"
        resp = self._txrx5(cmd) or ""
        return resp
    #------------------------------------------------------
    def DFP_get_play_state(self, 引数_devNo, 引数_stNo):
        cmd = f"DST:{int(引数_devNo):01X}:{int(引数_stNo):01X}!"
        resp = self._txrx5(cmd) or ""
        return resp
    #------------------------------------------------------
    def DFP_Stop(self, 引数_devNo):
        cmd = f"DSP:{int(引数_devNo):01X}!"
        resp = self._txrx5(cmd) or ""
        return resp
    #------------------------------------------------------
    def DFP_Pause(self, 引数_devNo):
        cmd = f"DPA:{int(引数_devNo):01X}!"
        resp = self._txrx5(cmd) or ""
        return resp
    #------------------------------------------------------
    def DFP_Resume(self, 引数_devNo):
        cmd = f"DPR:{int(引数_devNo):01X}!"
        resp = self._txrx5(cmd) or ""
        return resp
    #------------------------------------------------------
    def DFP_Volume(self, 引数_devNo, vol):
        cmd = f"VOL:{int(引数_devNo):01X}:{int(vol):02X}!"
        resp = self._txrx5(cmd) or ""
        return resp
    #------------------------------------------------------
    def DFP_set_eq(self, 引数_devNo, eq_mode):
        cmd = f"DEF:{int(引数_devNo):01X}:{int(eq_mode):02X}!"
        resp = self._txrx5(cmd) or ""
        return resp

    #======================================================
    # I2C（プロキシ）
    #======================================================
    def i2cWrite(self, slave_addr, addr, val):
        cmd = f"I2W:{int(slave_addr):02X}:{int(addr):02X}:{int(val):02X}!"
        resp = self._txrx5(cmd)
        return resp == "!!!!!"
    #------------------------------------------------------
    def i2cRead(self, slave_addr, addr):
        cmd = f"I2R:{int(slave_addr):02X}:{int(addr):02X}!"
        resp = self._txrx5(cmd)
        if resp and len(resp) == 5 and resp[-1] == "!":
            try             : return int(resp[:-1], 16)
            except Exception: return 0x00
        return 0x00
