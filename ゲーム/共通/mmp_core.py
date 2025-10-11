# -*- coding: utf-8 -*-
# filename : mmp_core.py
#============================================================
# ＭＭＰコマンド
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_adapter_base import MmpAdapterBase
BAUD_CANDIDATES = (921600,57600,38400,19200,9600,4800,2400,300)

class Settings:
    def __init__(self):
        self.PortName       = ""
        self.BaudRate       = 115200
        self.Timeout        = 400
        self.TimeoutIo      = 200
        self.TimeoutVerify  = 400
        self.TimeoutGeneral = 400
        self.TimeoutAnalog  = 400
        self.TimeoutPwm     = 400
        self.TimeoutAudio   = 400
        self.TimeoutDigital = 400
        self.TimeoutI2c     = 400
        self.AnalogBits     = 10

from mmp_core_INF import _InfoModule
from mmp_core_ANA import _AnalogModule
from mmp_core_DIG import _DigitalModule
from mmp_core_PWM import _PwmModule
from mmp_core_I2C import _I2cModule
from mmp_core_MP3 import _AudioModule

from mmp_core_COM import _resolve, _try_parse_hex4_bang, _hex1, _hex2, _hex3, _hex4

class MmpClient:

    # --- Backward-compatibility aliases for split modules ---
    # 旧来コードの client._InfoModule / MmpClient._InfoModule 参照に対応
    from mmp_core_INF import _InfoModule    as _InfoModule
    from mmp_core_ANA import _AnalogModule  as _AnalogModule
    from mmp_core_DIG import _DigitalModule as _DigitalModule
    from mmp_core_PWM import _PwmModule     as _PwmModule
    from mmp_core_I2C import _I2cModule     as _I2cModule
    from mmp_core_MP3 import _AudioModule   as _AudioModule

    #========================
    # コンストラクタ
    #========================
    def __init__(self, adapter: MmpAdapterBase):

        if adapter is None: raise ValueError("adapter is required")

        # アダプタを実装する。
        self.adapter         = adapter

        # 設定モジュールを実装する。
        self.Settings        = Settings()

        # 機能モジュールを実装する
        self.Command = self._Command(self)
        self.Info    = self._InfoModule(self)
        self.Analog  = self._AnalogModule(self)
        self.Pwm     = self._PwmModule(self)
        self.Audio   = self._AudioModule(self)
        self.Digital = self._DigitalModule(self)
        self.I2c     = self._I2cModule(self)

    #========================
    # プロパティ
    #========================
    @property
    def IsOpen(self)        : return self.adapter._is_open

    @property
    def ConnectedPort(self) : return self.adapter._connected_port

    @property
    def ConnectedBaud(self) : return self.adapter._connected_baud

    @property
    def LastError(self)     : return self.adapter._lastError

    #========================
    # 低レイヤ コマンド
    #========================
    #-------------------
    # 接続：条件指定
    #-------------------
    def ConnectWithBaud(self    ,
        baud: int               ,   # ① 通信速度
        timeoutIo:       int = 0,   # ② 一般用タイムアウト(単位；ミリ秒)
        verifyTimeoutMs: int = 0,   # ③ ベリファイ用タイムアウト(単位；ミリ秒)
    ) -> bool:                      # 戻り値：True=成功／False=失敗

        # 接続情報を初期化する。
        self.adapter._is_open        = False
        self.adapter._connected_port = None
        self.adapter._connected_baud = None
        self.adapter._lastError      = None

        use_io = _resolve(timeoutIo, self.Settings.TimeoutIo)
        use_ver= _resolve(verifyTimeoutMs, self.Settings.TimeoutVerify)

        try:
            # 指定の通信速度でポートを開く。
            if not self.adapter.open_baud(int(baud)):
                # 接続情報を更新する。
                self.adapter._lastError = f"{baud}bpsで、ポートを開けませんでした。"
                return False            # 戻り値 → [失敗]

            # 入力バッファを消去する。
            self.adapter.sleep_ms(10)
            self.adapter.clear_input()
            self.adapter.flush()

            # バージョンを取得して、ベリファイを実行する。
            if not self._verify(use_ver):

                # 通信を切断する。
                self.adapter.close()

                # 接続情報を更新する。
                self.adapter._lastError = f"{baud}bpsで、MMPが見つかりませんでした。"

                return False            # 戻り値 → [失敗]

            # 接続情報を更新する。
            self.Settings.PortName  = str(self.adapter._connected_port)
            self.Settings.BaudRate  = int(self.adapter._connected_baud)
            self.adapter._lastError = None

            return True                 # 戻り値 → [成功]

        except Exception as ex:

            # 通信を切断する。
            try     : self.adapter.close()
            except  : pass

            # 接続情報を更新する。
            self.adapter._is_open   = False
            self.adapter._lastError = str(ex)

            return False                # 戻り値 → [失敗]

    #-------------------
    # 接続：自動
    #-------------------
    def ConnectAutoBaud(self,
        candidates = BAUD_CANDIDATES    # ①通信速度一覧
    ) -> bool:                          # 戻り値：True=成功／False=失敗

        # 通信速度一覧の通信速度を用いて、総当たりで接続を試みる。
        for b in candidates:
            if self.ConnectWithBaud(b, 0, 0): return True   # 戻り値 → [成功]

        # 接続情報を更新する。
        self.adapter._lastError = f"以下のボーレートでは、MMPが見つかりませんでした。\n{candidates}"

        return False                                        # 戻り値 → [失敗]

    #-------------------
    # 切断
    #-------------------
    def Close(self):
        # 切断を試み、失敗した場合は接続情報を更新する。
        try    : self.adapter.close()
        finally: self._is_open = False

    #-------------------
    # バージョンチェック
    #-------------------
    def _verify(self,
        timeout_ms: int     # ① タイムアウト(単位：ミリ秒)
    ) -> bool:              # 戻り値：True=成功／False=失敗
        # ＭＭＰからバージョンを取得し、実行ステータスを返す。
        # 通信速度切替で不安定になるので、最初にダミーのコマンドを送信してから
        # バージョンコマンドを送信する。
        resp = self._send_command("!"   , _resolve(timeout_ms, self.Settings.TimeoutVerify))
        resp = self._send_command("VER!", _resolve(timeout_ms, self.Settings.TimeoutVerify))
        return len(resp) == 5 and resp.endswith('!')

    #-------------------
    # ＭＭＰシリアル通信
    #-------------------
    def _send_command(self,
        cmd: str        ,   # ① 送信コマンド文字列
        timeout_ms: int ,   # ② タイムアウト(単位：ミリ秒)
    ) -> str:               # 戻り値：""以外=コマンドの戻り値／""=引数違反

        if not cmd or not isinstance(cmd, str):
            # 接続情報を更新する。
            self.adapter._lastError = "コマンド文字列がありません。"
            return ""       # 戻り値 → [失敗]

        # 末尾の"!"が無ければ補正する。
        if not cmd.endswith('!'): cmd += '!'

        # 入力バッファを消去→コマンドを送信→送信バッファを消去
        self.adapter.clear_input()
        self.adapter.flush()
        self.adapter.write_ascii(cmd)
        self.adapter.flush()

        # 5文字の戻り値を取得する まはた タイムアウト まで、繰り返す。
        resp_chars = []
        start = self.adapter.now_ms()
        while len(resp_chars) < 5 and self.adapter.ticks_diff(self.adapter.now_ms(), start) < int(timeout_ms):

            # 受信バッファの先頭1文字を取得する。
            ch = self.adapter.read_one_char()

            # 取得できれば文字列に追加、出来なければ少し待つ
            if ch: resp_chars.append(ch)
            else : self.adapter.sleep_ms(1)

        # 戻り値の書式が正しければ、正常で返す。
        if len(resp_chars) == 5 and resp_chars[-1] == '!': return "".join(resp_chars)
                            # 戻り値 → [正常]

        # 接続情報を更新する。
        self.adapter._lastError = "時間内にMMPの返信を受け取れませんでした。"

        return ""           # 戻り値 → [失敗]

    #========================
    #（公開用）階層化API
    #========================
    #----------------------
    # << コマンドモジュール >>
    #----------------------
    class _Command:
        #----------------------
        # コンストラクタ
        #----------------------
        def __init__(self, p:'MmpClient'): self._p = p
 
        def typeOk(self, argCMD:str, timeoutMs:int=0) -> int:
            t       = _resolve(timeoutMs, self._p.Settings.Timeout)
            resp    = self._p._send_command(argCMD, t)
            return resp == "!!!!!"

        def typeVal(self, argCMD:str, timeoutMs:int=0) -> int:
            t       = _resolve(timeoutMs, self._p.Settings.Timeout)
            resp    = self._p._send_command(argCMD, t)
            ok, v   = _try_parse_hex4_bang(resp); 
            return v if ok else -1