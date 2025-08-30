#=====================================================================
# ＭＭＰライブラリ Rottenmeier (CircuitPython版)
#---------------------------------------------------------------------
# Ver 0.03.001　2025/08/30 By Takanari.Kureha
# ・ソースコード統一化
#=====================================================================
# ★固有のimportあり
import time
import board
import busio

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
    uart       = None
    接続済     = False
    version    = ""
    #┴

    #=====================================================================
    # 初期化：通信パラメータはコンストラクターで決定
    #=====================================================================
    # ★固有の処理あり
    def __init__(
        self,
        arg通信速度     = 115200, # 通信速度(単位：bps)
        arg時間切れ     = 100   , # タイムアウト(単位：ミリ秒)
        arg割当ピンTX   = None  , # TXピン; None=自動取得
        arg割当ピンRX   = None  , # RXピン; None=自動取得
        ):
        #┬
        #○UART設定（constructorで確定）
        self.baud       = int(arg通信速度)
        self.timeout    = int(arg時間切れ)
        self.tx_pin     = arg割当ピンTX
        self.rx_pin     = arg割当ピンRX
        #|
        #○UATRT設定をログに出力する
        print("<<Initializing...>>")
        print(f" - UART     = GPIO-Serial")
        print(f" - TX       = {self.tx_pin}")
        print(f" - RX       = {self.rx_pin}")
        print(f" - Baudrate = {self.baud}bps")
        print(f" - Bits     = 8")
        print(f" - Parity   = None")
        print(f" - Stop     = 1")
        print(f" - timeout  = {self.timeout}ms")
        #┴

    #---------------------------------------------------------------------
    # 内部：board ピン解決（TX/RX デフォルトあり）
    #---------------------------------------------------------------------
    # ★固有の関数
    def UARTピン取得(self, pin, is_tx):
        # 1) そのまま board の属性っぽいオブジェクトなら返す
        try:
            # 多くのボードで repr に "board." が含まれる
            if hasattr(pin, "name") or "board." in repr(pin): return pin
        except Exception: pass

        # 2) None → board.TX / board.RX
        if pin is None: return getattr(board, "TX" if is_tx else "RX")

        # 3) 文字列なら board.<name> を探す
        if isinstance(pin, str):

            name = pin

            # そのまま
            if hasattr(board, name): return getattr(board, name)

            # 大文字化して再トライ
            if hasattr(board, name.upper()): return getattr(board, name.upper())

            # 先頭に GP を付けて再トライ（RP2040 など）
            if name.isdigit():
                for prefix in ("GP", "D", "IO"):
                    cand = f"{prefix}{name}"
                    if hasattr(board, cand): return getattr(board, cand)

            raise ConnectException(f"指定ピンを解決できません: '{pin}'")

        # 4) 数値なら GPx/Dx/IOx を順に探す
        if isinstance(pin, int):
            for prefix in ("GP", "D", "IO"):
                cand = f"{prefix}{pin}"
                if hasattr(board, cand): return getattr(board, cand)
            raise ConnectException(f"数値ピンを解決できません: {pin}")

        # 最後の手段：そのまま返す（ボード依存型）
        return pin

    #---------------------------------------------------------------------
    # 内部：現在時刻(秒)と期限(秒)
    #---------------------------------------------------------------------
    # ★固有の関数
    def _now(self): # 固有の関数
        return time.monotonic()

    def _deadline(self, ms):
        return self._now() + (ms / 1000.0)

    #---------------------------------------------------------------------
    # 内部：ＭＭＰから返信を受信
    #---------------------------------------------------------------------
    # ★固有の処理あり
    def _コマンド受信(self):
        end_t = self._deadline(self.timeout)
        buf = bytearray()

        while self._now() < end_t:
            n = self.uart.in_waiting
            if n and n > 0:
                b = self.uart.read(1)
                if not b: continue
                buf += b
                if len(buf) > 5: buf = buf[-5:]
                if len(buf) == 5 and buf[-1] == 0x21:  # '!'
                    try             : return buf.decode("ascii")
                    except Exception: return "".join(chr(x) for x in buf)
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
        print(f"<<Connecting(GPIO UART)...>>")
        tx      = self.UARTピン取得(self.tx_pin, True)
        rx      = self.UARTピン取得(self.rx_pin, False)
        timeout = self.timeout / 1000.0
        try:
            self.uart = busio.UART(
                baudrate             = self.baud, # ボーレート
                timeout              = timeout  , # タイムアウト
                bits                 = 8        , # データビット
                parity               = None     , # パリティ
                stop                 = 1        , # ストップビット
                tx                   = tx       , # TXピン
                rx                   = rx       , # RXピン
                receiver_buffer_size = 64       , # 必要に応じて調整
            )
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
            # reset_input_bufferを実装している場合
            if hasattr(self.uart, "reset_input_buffer"):
                try:
                    self.uart.reset_input_buffer()
                    return
                except Exception: pass

            deadline = time.monotonic() + (30 / 1000.0)
            while time.monotonic() < deadline:

                # サイズを取得 ※in_waitingを実装している場合
                n = 0
                if hasattr(self.uart, "in_waiting"):
                    try: n = int(self.uart.in_waiting) or 0
                    except Exception: n = 0

                # → サイズ取得に対応している場合
                if n > 0:
                    # → サイズ指定読出しを実装している場合
                    try: self.uart.read(n)
                    # → サイズ指定読出しを実装していない場合
                    except Exception:
                        try: self.uart.read()
                        except Exception: break

                # → サイズ取得に対応していない場合
                else:
                    # 1バイト読み捨てして終了判定
                    try: b = self.uart.read(1)
                    except Exception: b = None
                    if not b: break
                time.sleep(0.001)
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
            if self.uart: self.uart.deinit()
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
        resp = self._コマンド送信(f"PWM:{int(argCh通番):02X}:{int(argPWM値):04X}!")
        return resp == "!!!!!"
    #---------------------------------------------------------------------
    # サーボ特性設定（角度↔PWM）
    #---------------------------------------------------------------------
    def PWM_INIT(self, arg角度下限, arg角度上限, argPWM最小, argPWM最大):
        resp = self._コマンド送信(f"PWI:{int(arg角度下限):03X}:{int(arg角度上限):03X}:{int(argPWM最小):04X}:{int(argPWM最大):04X}!")
        return resp == "!!!!!"
    #---------------------------------------------------------------------
    # PWM出力(角度)
    #---------------------------------------------------------------------
    def PWM_ANGLE(self, argCh通番, arg角度):
        resp = self._コマンド送信(f"PWA:{int(argCh通番):02X}:{int(arg角度):03X}!")
        return resp == "!!!!!"

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
