#=====================================================================
# ＭＭＰライブラリ Rottenmeier用 CPython版
#---------------------------------------------------------------------
# Ver 0.03.008　2025/08/31 By Takanari.Kureha
# ・ＰＷＭコマンドを高速化
#=====================================================================
# ★固有のimportあり
import time
import serial

#---------------------------------------------------------------------
# 例外
#---------------------------------------------------------------------
class ConnectException(Exception):
    pass

#=====================================================================
# 本体
#=====================================================================
class mmp:

    #┬
    #○アナログ入力関連
    参加総人数 = None
    スイッチ数 = None
    丸め       = None
    mmpAnaVal  = []
    #│
    #○PWM関連
    PWM機器状況 = [False] * 16
    #│
    #○接続関連
    接続済     = False
    uart       = None
    version    = None
    #┴

    #=====================================================================
    # 初期化：通信パラメータはコンストラクターで決定
    #=====================================================================
    # ★固有の処理あり
    def __init__(
        self,
        arg読込調整     = 64    , # 読込みバッファ(64～256[推奨64])
        arg通信速度     = 115200, # 通信速度(単位：bps)
        arg時間切れ     = 0.2   , # タイムアウト(単位：秒)
        ):
        #┬
        #○1回でまとめて読む最大バイト数
        self.読込調整   = int(arg読込調整)
        #|
        #○UART設定（constructorで確定）
        self.baud       = int(arg通信速度)
        self.timeout_s  = float(arg時間切れ)
        self.timeout_ms = int(round(arg時間切れ * 1000))
        #|
        #○UATRT設定をログに出力する
        print("<<Initializing...>>")
        print(f" - Baudrate = {self.baud}bps")
        print(f" - timeout  = {self.timeout_ms}ms")
        print(f" - Bits     = 8")
        print(f" - Parity   = None")
        print(f" - Stop     = 1")
        print( " - UART     = USB-Serial")
        #┴

    #=====================================================================
    # 内部：共通化のためのヘルパー
    #=====================================================================
    def _now_ms(self):
        # MicroPython 優先（wrap 対応）
        try:
            import time as _t
            if hasattr(_t, "ticks_ms"): return _t.ticks_ms()
        except Exception:
            pass
        # CPython / CircuitPython
        try:
            import time as _t
            return int(_t.monotonic() * 1000)
        except Exception:
            import time as _t
            return int(_t.time() * 1000)
    #---------------------------------------------------------------------
    def _time_left_ms(self, deadline_ms):
        # MicroPython の wrap-aware 差分
        try:
            import time as _t
            if hasattr(_t, "ticks_diff"):
                return _t.ticks_diff(deadline_ms, _t.ticks_ms())
        except Exception: pass
        return deadline_ms - self._now_ms()
    #---------------------------------------------------------------------
    def _sleep_ms(self, ms):
        try:
            import time as _t
            if hasattr(_t, "sleep_ms"):  # MicroPython
                return _t.sleep_ms(int(ms))
        except Exception: pass
        import time as _t
        _t.sleep(ms / 1000.0)
    #---------------------------------------------------------------------
    def _rx_ready(self):
        try:
            # pyserial / CircuitPython busio.UART
            if hasattr(self.uart, "in_waiting"):
                return int(self.uart.in_waiting) or 0
            # MicroPython machine.UART
            if hasattr(self.uart, "any"): return int(self.uart.any()) or 0
        except Exception: return 0
        return 0
    #---------------------------------------------------------------------
    def _read1(self):
        try: return self.uart.read(1)
        except Exception: return None

    #---------------------------------------------------------------------
    # 内部：ＭＭＰから返信を受信
    #---------------------------------------------------------------------
    def _コマンド受信(self):
        # 受信期限（ms）とワークバッファ
        deadline = self._now_ms() + int(self.timeout_ms)
        buf = bytearray()
        limit = getattr(self, "読込調整", 64)  # 1回でまとめて読む上限

        while self._time_left_ms(deadline) > 0:

            # 受信バイト数（プラットフォーム差異は _rx_ready() が吸収）
            n = self._rx_ready()
            if n <= 0:
                self._sleep_ms(1)
                continue

            # 一度に読みすぎない
            if n > limit: n = limit

            # まとめて読めるだけ読む（read(n)が無ければ1バイトフォールバック）
            try:
                chunk = self.uart.read(n)
                # 一部実装は read(n) が None を返すことがある
                if not chunk: chunk = self.uart.read(1) or b""
            except Exception:
                try: chunk = self.uart.read(1) or b""
                except Exception: chunk = b""

            if not chunk:
                # 受信が無ければ少しだけ待つ
                self._sleep_ms(1)
                continue

            # 常に末尾5バイトだけ保持
            buf.extend(chunk)
            if len(buf) > 5: buf = buf[-5:]

            # 5バイト＆末尾'!'で確定
            if len(buf) == 5 and buf[-1] == 0x21:  # '!'
                try: return buf.decode("ascii")
                # 非ASCII混入時も一応返す
                except Exception: return "".join(chr(b) for b in buf)

        # タイムアウト
        return None
    #---------------------------------------------------------------------
    # 内部：ＭＭＰへコマンドを送信
    #---------------------------------------------------------------------
    def _コマンド送信(self, arg文字列):
        try             : self.uart.write(arg文字列.encode("ascii"))
        except Exception: return None
        return self._コマンド受信()

    #=====================================================================
    # 接続処理
    #=====================================================================
    # ★固有の処理あり
    def 通信接続(self):

        self.接続済 = False # 未接続に仮設定

        # UARTを接続する
        print("<<Scanning...>>")
        self.uart               = serial.Serial()
        self.uart.baudrate      = self.baud
        self.uart.timeout       = self.timeout_s
        self.uart.write_timeout = self.timeout_s
        for COM番号 in range(0, 100):
            try:
                # ポートでUARTを接続
                self.uart.port = f"COM{COM番号}"
                self.uart.open()
                print(f"  - COM{COM番号}")

                # バッファをクリア
                time.sleep(0.005) # 単位：秒
                self.バッファ消去()

                # バージョンを取得
                time.sleep(0.2)
                if self.バージョン確認():
                    print(f"  -> Connected({self.uart.port})")
                    print(f"  -> Connected(Ver.{self.version})")
                    self.接続済 = True
                    break

            except Exception: pass #self.通信切断()

        if not self.接続済:
            self.通信切断()
            return False

        return True
    #---------------------------------------------------------------------
    # ★固有の処理あり
    def 通信接続_指定(self, comm_num):

        self.接続済 = False # 未接続に仮設定

        # UARTを接続
        print(f"<<Connecting(USB UART {comm_num})...>>")
        self.uart               = serial.Serial()
        self.uart.baudrate      = self.baud
        self.uart.timeout       = self.timeout_s
        self.uart.write_timeout = self.timeout_s
        self.uart.port          = comm_num
        try: self.uart.open()
        except Exception as e:
            raise ConnectException(f"UARTに接続失敗: {e}")

        # バッファをクリア
        time.sleep(0.005) # 単位：秒
        self.バッファ消去()

        # バージョンを取得
        if self.バージョン確認():
            print(f"  -> Connected(Ver.{self.version})")
            self.接続済 = True
            return True
        else:
            self.通信切断()
            return False
    #---------------------------------------------------------------------
    # ★固有の処理あり
    def バッファ消去(self):
        try:
            self.uart.reset_input_buffer()
            self.uart.reset_output_buffer()
        except Exception: pass
    #---------------------------------------------------------------------
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
    #---------------------------------------------------------------------
    # ★固有の処理あり
    def 通信切断(self):
        try:
            if self.uart and self.uart.is_open: self.uart.close()
        except Exception: pass

        print("<<Disconnected>>")
        self.uart   = None  # UARTを破棄
        self.接続済 = False # 接続状態を未接続

    #=====================================================================
    # アナログ入力
    #=====================================================================
    #---------------------------------------------------------------------
    # バッファ初期化
    #---------------------------------------------------------------------
    def アナログ初期化(self):
        #┬
        #○未初期化はエラー
        if self.スイッチ数 <= 0 or self.参加総人数 <= 0:
            raise ValueError("アナログ設定が未完了です。")
        #│
        #○参加総人数 × スイッチ数 の 2次元配列を 0 で確保
        self.mmpAnaVal = [[0 for _ in range(self.スイッチ数)] for _ in range(self.参加総人数)]
        #┴
    #---------------------------------------------------------------------
    # アクセス範囲設定
    #---------------------------------------------------------------------
    def アナログ設定(self  ,
            argスイッチ数 = 4, # 使用するHC4067の個数(1～4)
            arg参加人数   = 1, # 使用するHC4067のPin数(1～16)
            arg丸め       = 5  # アナログ値の丸め
            ):
        #┬
        #○アナログパラメータをセットする
        self.参加総人数  = int(arg参加人数)
        self.スイッチ数  = int(argスイッチ数)
        self.丸め        = int(arg丸め)
        print("<<Set Analog Input Parameters>>")
        print("  - Switches     = %d" % self.スイッチ数)
        print("  - Participants = %d" % self.参加総人数)
        print("  - Rounding     = %d" % self.丸め)
        #│
        #○測定データをゼロ初期する
        self.アナログ初期化()
        #│
        #○アナログ入力ピンのアクセス範囲を設定する
        _ = self._コマンド送信(f"ANS:{self.参加総人数:02X}:{self.スイッチ数:02X}!")
        #┴
    #---------------------------------------------------------------------
    # データ読取
    #---------------------------------------------------------------------
    def アナログ読取(self):
        #┬
        #○アナログ入力ピンのアクセス範囲を設定する
        _ = self._コマンド送信("ANU!")
        #│
        #◎└┐参加者(HC4067のチャンネル)を走査する
        for チャンネルNo in range(self.参加総人数):
            #│
            #◎└┐スイッチ(HC4067の機器)を走査する
            for ボードNo in range(self.スイッチ数):
                #│
                #○走査中のチャンネルNo・ボードNoのアナログ値を取得する
                読取値 = self._コマンド送信(f"ANR:{チャンネルNo:02X}:{ボードNo:02X}!")
                #│
                #◇┐応答結果を値に変換してバッファに格納する
                結果 = None
                if 読取値 and len(読取値) == 5 and 読取値[-1] == "!":
                #　├→(応答が正常な場合)
                    #○数値化・丸め補正し、バッファに格納する
                    結果 = int(読取値[:-1], 16)
                    if self.丸め > 0: 結果 = (結果 // self.丸め) * self.丸め
                    #┴
                #　└┐
                    #┴
                #│
                #○結果を公開バッファに上書きする
                self.mmpAnaVal[チャンネルNo][ボードNo] = 結果
        #┴　┴　┴

    #=====================================================================
    # ＰＷＭ出力
    #=====================================================================
    #---------------------------------------------------------------------
    # 機器の接続状況を返す
    #  - 返り値: list[bool]（True=接続あり/False=接続なし）
    #---------------------------------------------------------------------
    def PWM_機器確認(self):
        #┬
        #◎└┐機器(PCA9685)を走査する
        状況 = [False] * 16
        for 機器No in range(16):
            #│
            #○機器を指定して接続状況を取得する
            読取値 = self._コマンド送信(f"PWX:{機器No:02X}!")
            #│
            #○下位bit=1なら接続ありにする
            結果 = None
            if 読取値 and len(読取値) == 5 and 読取値[-1] == "!":
                結果 = (int(読取値[:-1], 16) & 0x1) == 1
            #│
            #○結果を内部バッファに格納する
            状況[機器No] = 結果
        #│
        #○内部バッファを公開バッファに上書きする
        self.PWM機器状況 = 状況
        #│
        #▼公開バッファを返す
        return 状況
    #---------------------------------------------------------------------
    # 指定したPWMチャンネルの使用可否を返す
    #  - 返り値: True=使用可能/False=使用不可
    #---------------------------------------------------------------------
    def PWM_チャンネル使用可否(self, argCh通番):
        if not (0 <= argCh通番 <= 255):
            raise ValueError("[エラー] PWMチャンネルは 0〜255 の範囲で指定")
        if not self.PWM機器状況 or len(self.PWM機器状況) != 16:
            self.PWM_機器確認()
        pwm_id = argCh通番 // 16  # 0..15
        可否 = bool(self.PWM機器状況[pwm_id])
        return 可否
    #---------------------------------------------------------------------
    # PWM出力
    #---------------------------------------------------------------------
    def PWM_VALUE(self, argCh通番, argPWM値):
        try:
            self.uart.write(b"PWM:%02X:%04X!" % (argCh通番 & 0xFF, argPWM値 & 0xFFFF))
        except Exception:
            return False
        return self._コマンド受信() == "!!!!!"
    #---------------------------------------------------------------------
    # サーボ特性設定（角度↔PWM）
    #---------------------------------------------------------------------
    def PWM_INIT(self, arg角度下限, arg角度上限, argPWM最小, argPWM最大):
        try:
            self.uart.write(b"PWI:%03X:%03X:%04X:%04X!" % (arg角度下限, arg角度上限, argPWM最小, argPWM最大))
        except Exception:
            return False
        return self._コマンド受信() == "!!!!!"
    #---------------------------------------------------------------------
    # PWM出力(角度)
    #---------------------------------------------------------------------
    def PWM_ANGLE(self, argCh通番, arg角度):
        try:
            self.uart.write(b"PWA:%02X:%03X!" % (argCh通番 & 0xFF, arg角度 & 0x3FF))
        except Exception:
            return False
        return self._コマンド受信() == "!!!!!"

    #=====================================================================
    # デジタル入出力
    #=====================================================================
    #---------------------------------------------------------------------
    # 入力
    #---------------------------------------------------------------------
    def digital_IN(self, argポートNo):
        resp = self._コマンド送信(f"POR:{int(argポートNo):02X}!")
        return int(resp[:-1], 16) if resp and len(resp) == 5 and resp[-1] == "!" else 0
    #---------------------------------------------------------------------
    # 出力
    #---------------------------------------------------------------------
    def digital_OUT(self, argポートNo, arg値):
        resp = self._コマンド送信(f"POW:{int(argポートNo):02X}:{int(arg値):01X}!")
        return resp == "!!!!!"

    #=====================================================================
    # ＭＰ３プレイヤー
    #=====================================================================
    #---------------------------------------------------------------------
    # 機器情報
    #---------------------------------------------------------------------
    def DFP_Info(self, arg機器No):
        return self._コマンド送信(f"DPX:{int(arg機器No):01X}!") or ""
    #---------------------------------------------------------------------
    # 指定フォルダ内トラック再生
    #---------------------------------------------------------------------
    def DFP_PlayFolderTrack(self, arg機器No, argフォルダ, argトラック):
        return self._コマンド送信(f"DIR:{int(arg機器No):01X}:{int(argフォルダ):02X}:{int(argトラック):02X}!") or ""
    #---------------------------------------------------------------------
    # ステータス
    #  1:MP3
    #  2:音量
    #  3:イコライザ
    #  4:ファイル番号(総合計)
    #  5:ファイル番号(フォルダ内)
    #---------------------------------------------------------------------
    def DFP_get_play_state(self, arg機器No, st種別No):
        return self._コマンド送信(f"DST:{int(arg機器No):01X}:{int(st種別No):01X}!") or ""
    #---------------------------------------------------------------------
    # 停止
    #---------------------------------------------------------------------
    def DFP_Stop(self, arg機器No):
        return self._コマンド送信(f"DSP:{int(arg機器No):01X}!") or ""
    #---------------------------------------------------------------------
    # 一時停止
    #---------------------------------------------------------------------
    def DFP_Pause(self, arg機器No):
        return self._コマンド送信(f"DPA:{int(arg機器No):01X}!") or ""
    #---------------------------------------------------------------------
    # 再生再開
    #---------------------------------------------------------------------
    def DFP_Resume(self, arg機器No):
        return self._コマンド送信(f"DPR:{int(arg機器No):01X}!") or ""
    #---------------------------------------------------------------------
    # 音量設定（0〜30）
    #---------------------------------------------------------------------
    def DFP_Volume(self, arg機器No, arg音量):
        return self._コマンド送信(f"VOL:{int(arg機器No):01X}:{int(arg音量):02X}!") or ""
    #---------------------------------------------------------------------
    # イコライザー設定：0〜5
    #  0: Normal, 1: Pop, 2: Rock, 3: Jazz, 4: Classic, 5: Bass
    #---------------------------------------------------------------------
    def DFP_set_eq(self, arg機器No, argモード):
        return self._コマンド送信(f"DEF:{int(arg機器No):01X}:{int(argモード):02X}!") or ""

    #=====================================================================
    # I2C（プロキシ）
    #=====================================================================
    def i2cWrite(self, slave_addr, addr, val):
        resp = self._コマンド送信(f"I2W:{int(slave_addr):02X}:{int(addr):02X}:{int(val):02X}!")
        return resp == "!!!!!"
    #---------------------------------------------------------------------
    def i2cRead(self, slave_addr, addr):
        resp = self._コマンド送信(f"I2R:{int(slave_addr):02X}:{int(addr):02X}!")
        if resp and len(resp) == 5 and resp[-1] == "!":
            try             : return int(resp[:-1], 16)
            except Exception: return 0x00
        return 0x00
