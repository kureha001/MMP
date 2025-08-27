#======================================================
# ＭＭＰライブラリ Rottenmeier (MicroPython クライアント版)
#------------------------------------------------------
# Ver 0.03.002　2025/08/27 By Takanari.Kureha
#   - RP2040/RP2350等の MicroPython 上で動作
#   - UART1 を既定。GPIOはコンストラクタで変更可能
#   - 接続時のみ例外送出。通常コマンドの内容バリデーションは行わない
#======================================================
import time
from machine import UART, Pin

#=============
# 例外クラス
#=============
class ConnectException(Exception):
    pass

class TimeoutException(Exception):
    pass

class ProtocolException(Exception):
    pass

#=============
# 本体クラス
#=============
class mmp:
    #------------------------------------------------------------------
    # コンストラクタ（日本語引数／既定値は合意どおり）
    #------------------------------------------------------------------
    def __init__(self,
                 uart番号=1,
                 送信ピン=0,
                 受信ピン=1,
                 通信速度=115200,
                 データビット=8,
                 パリティ=None,
                 ストップビット=1,
                 タイムアウトms=300,
                 リトライ回数=2):

        self._uart_id = uart番号
        self._tx = 送信ピン
        self._rx = 受信ピン
        self._baud = 通信速度
        self._bits = データビット
        self._parity = パリティ
        self._stop = ストップビット
        self._timeout = int(タイムアウトms)
        self._retries = int(リトライ回数)

        # UART確立
        self._uart = UART(self._uart_id,
                          baudrate=self._baud,
                          bits=self._bits,
                          parity=self._parity,
                          stop=self._stop,
                          tx=Pin(self._tx),
                          rx=Pin(self._rx))

        # 状態変数
        self.接続済 = False
        self.version = ""

        # アナログ関連パラメータ
        self.参加総人数 = None
        self.スイッチ数 = None
        self.丸め = None
        self.mmpAnaVal = []

        # PWM 状態
        self.PWM機器状況 = [False] * 16         # 機器ID 0..15
        self.PWMチャンネル状況 = [False] * 256  # 通しch 0..255

    #======================================================
    # 内部ユーティリティ
    #======================================================
    def _ensure_connected(self):
        if not self.接続済 or (self._uart is None):
            raise ConnectException("Not connected")

    def _write_ascii(self, s: str):
        # 送信（ASCII前提）
        self._uart.write(s.encode("ascii"))

    def _read5_once(self, timeout_ms: int) -> bytes:
        """5バイトを締切内で読み切る。未満ならNone相当を返す。"""
        buf = bytearray()
        t0 = time.ticks_ms()
        while time.ticks_diff(time.ticks_ms(), t0) <= timeout_ms:
            if self._uart.any():
                chunk = self._uart.read(5 - len(buf))
                if chunk:
                    buf.extend(chunk)
                    if len(buf) >= 5:
                        return bytes(buf[:5])
            # 少しだけ譲る
            time.sleep_ms(0)
        return None  # timeout (部分受信)

    def _read5(self) -> bytes:
        """5バイト読み（最大リトライ self._retries）。末尾'!'でなければProtocolException。"""
        attempt = 0
        while True:
            attempt += 1
            data = self._read5_once(self._timeout)
            if data is None:
                if attempt > self._retries:
                    raise TimeoutException("Timeout waiting 5 bytes")
                # リトライ（バッファ破棄はしない方針）
                continue
            if len(data) != 5:
                # 規約上あり得ないが、保険
                if attempt > self._retries:
                    raise TimeoutException("Partial frame")
                continue
            if data[-1] != ord('!'):
                # 5バイトだが終端不一致 → プロトコル例外
                raise ProtocolException("Invalid frame terminator")
            return data

    def _read5_text(self) -> str:
        return self._read5().decode("ascii", "strict")

    def _fmt02(self, n: int) -> str:
        return f"{n & 0xFF:02X}"

    def _fmt03(self, n: int) -> str:
        return f"{n & 0xFFF:03X}"

    def _fmt04(self, n: int) -> str:
        return f"{n & 0xFFFF:04X}"

    #======================================================
    # 接続系
    #======================================================
    def 通信接続(self):
        """接続確立（VER!で疎通確認）。成功時はバージョン文字列を返す。"""
        # 直ちに疎通確認（待機なし）
        s = self.バージョン()
        if not s:
            raise ConnectException("Handshake failed")
        self.接続済 = True
        return s  # バージョン文字列

    def 通信切断(self):
        """切断して直ちに ConnectException を送出（アプリを停止させる）。"""
        try:
            if self._uart and hasattr(self._uart, "deinit"):
                self._uart.deinit()
        finally:
            self._uart = None
            self.接続済 = False
        raise ConnectException("Disconnected")

    def バージョン(self):
        """'VER!' → 'xyzz!' を受け取り 'x.y.zz' を返す（PC版互換）。"""
        self._write_ascii("VER!")
        data = self._read5_text()  # 例: "0304!"
        s = data[:-1]  # "0304"
        # PC版互換（先頭2桁を x.y と解釈、後ろ2桁を zz）
        if len(s) >= 4:
            結果 = f"{s[0]}.{s[1]}.{s[2:4]}"
        else:
            結果 = ""
        self.version = 結果
        return 結果

    #======================================================
    # アナログ入力
    #======================================================
    def アナログ初期化(self):
        tmp = [0 for _ in range(self.スイッチ数)]
        self.mmpAnaVal = [tmp.copy() for _ in range(self.参加総人数)]

    def アナログ設定(self, argスイッチ数=4, arg参加人数=1, arg丸め=5):
        self.参加総人数 = arg参加人数
        self.スイッチ数 = argスイッチ数
        self.丸め = int(arg丸め)
        self.アナログ初期化()

        cmd = f"ANS:{self._fmt02(self.参加総人数)}:{self._fmt02(self.スイッチ数)}!"
        self._write_ascii(cmd)
        _ = self._read5()  # 応答は内容未使用（5文字枠のみ保証）

    def アナログ読取(self):
        # 値バッファ更新（ANU）
        self._write_ascii("ANU!")
        _ = self._read5()  # "!!!!!" 期待（内容検査はしない）

        # 個別参照（ANR）
        for pl in range(self.参加総人数):
            for sw in range(self.スイッチ数):
                cmd = f"ANR:{self._fmt02(pl)}:{self._fmt02(sw)}!"
                self._write_ascii(cmd)
                data = self._read5_text()
                # 内容検査はせず、16進解釈（失敗時は現行C選択＝例外伝播）
                val = int(data[:-1], 16)
                val = (val // self.丸め) * self.丸め
                self.mmpAnaVal[pl][sw] = val

    #======================================================
    # PWM（PCA9685）
    #======================================================
    def PWM_機器確認(self):
        """PWX:<id>! を 0..15 に発行し、機器ごとの接続可否を内部保持して返す。"""
        状況 = [False] * 16
        for dev in range(16):
            cmd = f"PWX:{self._fmt02(dev)}!"
            self._write_ascii(cmd)
            data = self._read5_text()      # "0001!" or "0000!" など
            try:
                v = int(data[:-1], 16)
                状況[dev] = (v & 1) == 1
            except Exception:
                # 互換：パース失敗時は False（PC版の「失敗→False化」に近い）
                状況[dev] = False
        self.PWM機器状況 = 状況
        return 状況

    def PWM_チャンネル確認(self):
        """全256chの使用可否テーブルを構築（機器状況から展開）。"""
        if not any(self.PWM機器状況):
            self.PWM_機器確認()
        ch_ok = [False] * 256
        for dev in range(16):
            if self.PWM機器状況[dev]:
                base = dev * 16
                for ch in range(16):
                    ch_ok[base + ch] = True
        self.PWMチャンネル状況 = ch_ok
        return ch_ok

    def PWM_チャンネル使用可否(self, channel: int):
        if not (0 <= channel <= 255):
            raise ValueError("PWMチャンネルは0〜255で指定")
        dev = channel // 16
        if not self.PWM機器状況 or len(self.PWM機器状況) != 16:
            self.PWM_機器確認()
        return bool(self.PWM機器状況[dev])

    def PWM_VALUE(self, argPort, argPWM):
        cmd = f"PWM:{self._fmt02(argPort)}:{self._fmt04(argPWM)}!"
        self._write_ascii(cmd)
        _ = self._read5()  # 成功/失敗は上位が内容で判断（ここでは枠のみ保証）

    def PWM_INIT(self, 開始角度, 終了角度, 開始PWM, 終了PWM):
        cmd = f"PWI:{self._fmt03(開始角度)}:{self._fmt03(終了角度)}:{self._fmt04(開始PWM)}:{self._fmt04(終了PWM)}!"
        self._write_ascii(cmd)
        _ = self._read5()

    def PWM_ANGLE(self, argPort, argAngle):
        cmd = f"PWA:{self._fmt02(argPort)}:{self._fmt03(argAngle)}!"
        self._write_ascii(cmd)
        _ = self._read5()

    #======================================================
    # デジタル入出力
    #======================================================
    def digital_IO(self, argPort, argValue):
        cmd = f"IO:{self._fmt02(argPort)}:{argValue & 0x1:01X}!"
        self._write_ascii(cmd)
        data = self._read5_text()
        return int(data[:-1], 16)  # C選択：失敗時は例外伝播

    def digital_OUT(self, argPort, argValue):
        cmd = f"POW:{self._fmt02(argPort)}:{argValue & 0x1:01X}!"
        self._write_ascii(cmd)
        data = self._read5_text()
        # 現行仕様：アプリ側で使い分けるため内容はそのまま返却せず、True/Falseは維持しない
        # 互換性のため "!!!!!" / "----!" / その他 をそのまま上位で扱ってもらうなら、文字列を返す。
        # ただしPC版は True/False を返していたため、互換優先で以下とする。
        if data == "!!!!!":
            return True
        elif data == "----!":
            return False
        else:
            return False

    def digital_IN(self, argPort):
        cmd = f"POR:{self._fmt02(argPort)}!"
        self._write_ascii(cmd)
        data = self._read5_text()
        return int(data[:-1], 16)  # C選択

    #======================================================
    # 任意行読み（非ブロッキング、終端'!'）
    #======================================================
    def readline(self):
        s = bytearray()
        while self._uart.any():
            b = self._uart.read(1)
            if not b:
                break
            if b == b'!':
                # '!'は除外し、UTF-8として返す
                try:
                    return s.decode('utf-8')
                except Exception:
                    return ""
            s.extend(b)
        try:
            return s.decode('utf-8') if s else ""
        except Exception:
            return ""

    #======================================================
    # ＭＰ３プレイヤー（DFPlayer）
    #======================================================
    def DFP_Info(self, 引数_devNo):
        cmd = f"DPX:{self._fmt02(引数_devNo)}!"
        self._write_ascii(cmd)
        return self._read5_text()

    def DFP_PlayFolderTrack(self, 引数_devNo, folder, track):
        cmd = f"DIR:{self._fmt02(引数_devNo)}:{self._fmt02(folder)}:{self._fmt02(track)}!"
        self._write_ascii(cmd)
        return self._read5_text()

    def DFP_get_play_state(self, 引数_devNo, 引数_stNo):
        cmd = f"DST:{self._fmt02(引数_devNo)}:{self._fmt02(引数_stNo)}!"
        self._write_ascii(cmd)
        return self._read5_text()

    def DFP_Stop(self, 引数_devNo):
        cmd = f"DSP:{self._fmt02(引数_devNo)}!"
        self._write_ascii(cmd)
        return self._read5_text()

    def DFP_Pause(self, 引数_devNo):
        cmd = f"DPA:{self._fmt02(引数_devNo)}!"
        self._write_ascii(cmd)
        return self._read5_text()

    def DFP_Resume(self, 引数_devNo):
        cmd = f"DPR:{self._fmt02(引数_devNo)}!"
        self._write_ascii(cmd)
        return self._read5_text()

    def DFP_Volume(self, 引数_devNo, vol):
        cmd = f"VOL:{self._fmt02(引数_devNo)}:{self._fmt02(vol)}!"
        self._write_ascii(cmd)
        return self._read5_text()

    def DFP_set_eq(self, 引数_devNo, eq_mode):
        cmd = f"DEF:{self._fmt02(引数_devNo)}:{self._fmt02(eq_mode)}!"
        self._write_ascii(cmd)
        return self._read5_text()

    #======================================================
    # I2C（リモート）
    #======================================================
    def i2cWrite(self, slave_addr, addr, val):
        cmd = f"I2W:{self._fmt02(slave_addr)}:{self._fmt02(addr)}:{self._fmt02(val)}!"
        self._write_ascii(cmd)
        _ = self._read5()  # "!!!!!" 期待（内容検査はしない）
        # PC版は True/False を返していたが、'----!' 未実装のため常に True 相当とするか、
        # 応答文字列を見て判断する実装に変える場合はここで分岐。
        return True

    def i2cRead(self, slave_addr, addr):
        cmd = f"I2R:{self._fmt02(slave_addr)}:{self._fmt02(addr)}!"
        self._write_ascii(cmd)
        data = self._read5_text()
        return int(data[:-1], 16)  # C選択：失敗時は例外伝播

    # *_word 系は合意どおり廃止
