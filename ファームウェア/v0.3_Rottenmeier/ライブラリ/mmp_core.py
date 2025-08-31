# 共有コア（環境非依存）

class Core:

    # 共有フィールド
    参加総人数 = None
    スイッチ数 = None
    丸め       = None
    mmpAnaVal  = []
    PWM機器状況 = [False] * 16
    接続済     = False
    version    = None

    def __init__(self, adapter, 読込調整=64, baud=115200, timeout_ms=100):
        self.adapter     = adapter
        self.読込調整     = int(読込調整)
        self.baud        = int(baud)
        self.timeout_ms  = int(timeout_ms)
        self.timeout_s   = float(timeout_ms / 1000.0)

        print("<<Initializing...>>")
        print(f" - Baudrate = {self.baud}bps")
        print(f" - timeout  = {self.timeout_ms}ms")
        print(f" - Bits     = 8")
        print(f" - Parity   = None")
        print(f" - Stop     = 1")

    # ===== 時間ヘルパ =====
    def _now_ms(self):
        try:
            import time as _t
            if hasattr(_t, "ticks_ms"):
                return _t.ticks_ms()
        except Exception:
            pass
        try:
            import time as _t
            return int(_t.monotonic() * 1000)
        except Exception:
            import time as _t
            return int(_t.time() * 1000)

    def _time_left_ms(self, deadline_ms):
        try:
            import time as _t
            if hasattr(_t, "ticks_diff"):
                return _t.ticks_diff(deadline_ms, _t.ticks_ms())
        except Exception:
            pass
        return deadline_ms - self._now_ms()

    def _sleep_ms(self, ms):
        try:
            import time as _t
            if hasattr(_t, "sleep_ms"):
                return _t.sleep_ms(int(ms))
        except Exception:
            pass
        import time as _t
        _t.sleep(ms / 1000.0)

    # ===== 受信ユーティリティ =====
    def _rx_ready(self):
        try:
            return int(self.adapter.rx_ready()) or 0
        except Exception:
            return 0

    def _read_n(self, n):
        try:
            return self.adapter.read(n)
        except Exception:
            return b""

    # ===== MMP 受信/送信 =====
    def _コマンド受信(self):
        deadline = self._now_ms() + int(self.timeout_ms)
        buf = bytearray()
        limit = int(self.読込調整) if self.読込調整 > 0 else 64

        while self._time_left_ms(deadline) > 0:
            n = self._rx_ready()
            if n <= 0:
                self._sleep_ms(1)
                continue
            if n > limit:
                n = limit
            try:
                chunk = self._read_n(n)
                if not chunk:
                    chunk = self._read_n(1) or b""
            except Exception:
                chunk = self._read_n(1) or b""

            if not chunk:
                self._sleep_ms(1)
                continue

            buf.extend(chunk)
            if len(buf) > 5:
                buf = buf[-5:]

            if len(buf) == 5 and buf[-1] == 0x21:  # '!'
                try:
                    return buf.decode("ascii")
                except Exception:
                    return "".join(chr(b) for b in buf)

        return None

    def _コマンド送信(self, s):
        try:
            self.adapter.write(s.encode("ascii"))
        except Exception:
            return None
        return self._コマンド受信()

    # ===== バージョン =====
    def バージョン確認(self):
        resp = self._コマンド送信("VER!")
        if resp and len(resp) == 5 and resp[-1] == "!":
            s = resp[:-1]
            self.version = f"{s[0]}.{s[1]}.{s[2:4]}"
            return True
        else:
            print("Failed to get version")
            self.version = "?.?.??"
            return False

    # ===== アナログ入力 =====
    def アナログ初期化(self):
        if self.スイッチ数 is None or self.参加総人数 is None:
            raise ValueError("アナログ設定が未完了です。")
        if self.スイッチ数 <= 0 or self.参加総人数 <= 0:
            raise ValueError("アナログ設定が未完了です。")
        self.mmpAnaVal = [[0 for _ in range(self.スイッチ数)] for _ in range(self.参加総人数)]

    def アナログ設定(self, argスイッチ数=4, arg参加人数=1, arg丸め=5):
        self.参加総人数 = int(arg参加人数)
        self.スイッチ数 = int(argスイッチ数)
        self.丸め = int(arg丸め)
        print("<<Set Analog Input Parameters>>")
        print(f"  - Switches     = {self.スイッチ数}")
        print(f"  - Participants = {self.参加総人数}")
        print(f"  - Rounding     = {self.丸め}")
        self.アナログ初期化()
        _ = self._コマンド送信(f"ANS:{self.参加総人数:02X}:{self.スイッチ数:02X}!")

    def アナログ読取(self):
        _ = self._コマンド送信("ANU!")
        for ch in range(self.参加総人数):
            for bd in range(self.スイッチ数):
                r = self._コマンド送信(f"ANR:{ch:02X}:{bd:02X}!")
                結果 = None
                if r and len(r) == 5 and r[-1] == "!":
                    結果 = int(r[:-1], 16)
                    if self.丸め and self.丸め > 0:
                        結果 = (結果 // self.丸め) * self.丸め
                self.mmpAnaVal[ch][bd] = 結果

    # ===== PWM =====
    def PWM_機器確認(self):
        状況 = [False] * 16
        for 機器No in range(16):
            r = self._コマンド送信(f"PWX:{機器No:02X}!")
            結果 = None
            if r and len(r) == 5 and r[-1] == "!":
                結果 = (int(r[:-1], 16) & 0x1) == 1
            状況[機器No] = 結果
        self.PWM機器状況 = 状況
        return 状況

    def PWM_チャンネル使用可否(self, argCh通番):
        if not (0 <= argCh通番 <= 255):
            raise ValueError("[エラー] PWMチャンネルは 0〜255 の範囲で指定")
        if not self.PWM機器状況 or len(self.PWM機器状況) != 16:
            self.PWM_機器確認()
        pwm_id = argCh通番 // 16
        return bool(self.PWM機器状況[pwm_id])

    def PWM_VALUE(self, argCh通番, argPWM値):
        try:
            self.adapter.write(b"PWM:%02X:%04X!" % (argCh通番 & 0xFF, argPWM値 & 0xFFFF))
        except Exception:
            return False
        return self._コマンド受信() == "!!!!!"

    def PWM_INIT(self, arg角度下限, arg角度上限, argPWM最小, argPWM最大):
        try:
            self.adapter.write(b"PWI:%03X:%03X:%04X:%04X!" % (arg角度下限, arg角度上限, argPWM最小, argPWM最大))
        except Exception:
            return False
        return self._コマンド受信() == "!!!!!"

    def PWM_ANGLE(self, argCh通番, arg角度):
        try:
            self.adapter.write(b"PWA:%02X:%03X!" % (argCh通番 & 0xFF, arg角度 & 0x3FF))
        except Exception:
            return False
        return self._コマンド受信() == "!!!!!"

    # ===== デジタル I/O =====
    def digital_IN(self, argポートNo):
        r = self._コマンド送信(f"POR:{int(argポートNo):02X}!")
        return int(r[:-1], 16) if r and len(r) == 5 and r[-1] == "!" else 0

    def digital_OUT(self, argポートNo, arg値):
        r = self._コマンド送信(f"POW:{int(argポートNo):02X}:{int(arg値):01X}!")
        return r == "!!!!!"

    # ===== MP3(DFPlayer) =====
    def DFP_Info(self, arg機器No):
        return self._コマンド送信(f"DPX:{int(arg機器No):01X}!") or ""

    def DFP_PlayFolderTrack(self, arg機器No, argフォルダ, argトラック):
        return self._コマンド送信(f"DIR:{int(arg機器No):01X}:{int(argフォルダ):02X}:{int(argトラック):02X}!") or ""

    def DFP_get_play_state(self, arg機器No, st種別No):
        return self._コマンド送信(f"DST:{int(arg機器No):01X}:{int(st種別No):01X}!") or ""

    def DFP_Stop(self, arg機器No):
        return self._コマンド送信(f"DSP:{int(arg機器No):01X}!") or ""

    def DFP_Pause(self, arg機器No):
        return self._コマンド送信(f"DPA:{int(arg機器No):01X}!") or ""

    def DFP_Resume(self, arg機器No):
        return self._コマンド送信(f"DPR:{int(arg機器No):01X}!") or ""

    def DFP_Volume(self, arg機器No, arg音量):
        return self._コマンド送信(f"VOL:{int(arg機器No):01X}:{int(arg音量):02X}!") or ""

    def DFP_set_eq(self, arg機器No, argモード):
        return self._コマンド送信(f"DEF:{int(arg機器No):01X}:{int(argモード):02X}!") or ""

    # ===== I2C Proxy =====
    def i2cWrite(self, slave_addr, addr, val):
        r = self._コマンド送信(f"I2W:{int(slave_addr):02X}:{int(addr):02X}:{int(val):02X}!")
        return r == "!!!!!"

    def i2cRead(self, slave_addr, addr):
        r = self._コマンド送信(f"I2R:{int(slave_addr):02X}:{int(addr):02X}!")
        if r and len(r) == 5 and r[-1] == "!":
            try:
                return int(r[:-1], 16)
            except Exception:
                return 0x00
        return 0x00
