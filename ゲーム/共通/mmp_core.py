# -*- coding: utf-8 -*-
# filename : mmp_core.py
#============================================================
# ＭＭＰコマンド
# バージョン：0.5
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_adapter_base import MmpAdapterBase

# 対応バージョン
VER_MAJOR = 0   # ベータ
VER_MINOR = 5   # コマンド名をWEB-API形式

# ボーレート一覧
BAUD_CANDIDATES = (921600,57600,38400,19200,9600,4800,2400,300)

# 通信データ量
DAT_LENGTH      = 20  # 上記1個あたりの上限バイト数
RES_LENGTH      = 5   # レスポンスのバイト数

# 設定プロパティ
class Settings:
    def __init__(self):
        self.PortName       = ""
        self.BaudRate       = 115200
        self.Timeout        = 400
        self.TimeoutIo      = 200
        self.TimeoutVerify  = 400
        self.TimeoutGeneral = 400
        self.TimeoutAnalog  = 400
        self.TimeoutPWM     = 400
        self.TimeoutMP3     = 400
        self.TimeoutDigital = 400
        self.TimeoutI2C     = 400
        self.AnalogBits     = 10

class MmpClient:
    #━━━━━━━━━━━━━━━
    # 使用モジュール
    #━━━━━━━━━━━━━━━
    from mmp_core_INF import _Info
    from mmp_core_ANA import _Analog
    from mmp_core_DIG import _Digital
    from mmp_core_PWM import _Pwm
    from mmp_core_I2C import _I2C
    from mmp_core_MP3 import _MP3

    #━━━━━━━━━━━━━━━
    # コンストラクタ
    #━━━━━━━━━━━━━━━
    def __init__(self, adapter: MmpAdapterBase):

        # 入場チェック
        if adapter is None: raise ValueError("adapter is required")

        # アダプタを実装する。
        self.adapter    = adapter

        # 設定プロパティを実装する。
        self.Settings   = Settings()

        # 機能モジュールを実装する
        self.INFO    = self._Info   (self, self.Settings.TimeoutGeneral)
        self.ANALOG  = self._Analog (self, self.Settings.TimeoutAnalog )
        self.DIGITAL = self._Digital(self, self.Settings.TimeoutDigital)
        self.PWM     = self._Pwm    (self, self.Settings.TimeoutPWM    )
        self.I2C     = self._I2C    (self, self.Settings.TimeoutI2C    )
        self.MP3     = self._MP3    (self, self.Settings.TimeoutMP3    )

    #━━━━━━━━━━━━━━━
    # プロパティ
    #━━━━━━━━━━━━━━━
    @property
    def IsOpen(self)        : return self.adapter._is_open

    @property
    def ConnectedPort(self) : return self.adapter._connected_port

    @property
    def ConnectedBaud(self) : return self.adapter._connected_baud

    @property
    def LastError(self)     : return self.adapter._lastError

#━━━━━━━━━━━━━━━
# 低レイヤ コマンド
#━━━━━━━━━━━━━━━
    #─────────────
    # 接続：条件指定
    #─────────────
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
            if not self._verify():

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

    #─────────────
    # 接続：自動
    #─────────────
    def ConnectAutoBaud(self,
        candidates = BAUD_CANDIDATES    # ①通信速度一覧
    ) -> bool:                          # 戻り値：True=成功／False=失敗

        # 通信速度一覧の通信速度を用いて、総当たりで接続を試みる。
        for b in candidates:
            if self.ConnectWithBaud(b, 0, 0): return True   # 戻り値 → [成功]

        # 接続情報を更新する。
        self.adapter._lastError = f"以下のボーレートでは、MMPが見つかりませんでした。\n{candidates}"

        return False                                        # 戻り値 → [失敗]

    #─────────────
    # 切断
    #─────────────
    def Close(self):
        # 切断を試み、失敗した場合は接続情報を更新する。
        try    : self.adapter.close()
        finally: self._is_open = False

    #─────────────
    # バージョンチェック
    #─────────────
    def _verify(self) -> bool:              # 戻り値：True=成功／False=失敗
        # バージョンを取得
        # 通信速度切替で不安定になるので、最初にダミーのコマンドを送信
        t    = self.Settings.TimeoutVerify
        resp = self._send_command("!"            , t)
        resp = self._send_command("INFO/VERSION!", t)

        # レスポンスをチェック
        if len(resp)    != RES_LENGTH: return
        if not resp.endswith('!')    : return
        if int(resp[0]) != VER_MAJOR : return
        if int(resp[1]) != VER_MINOR : return

        return True

    #─────────────
    # ＭＭＰシリアル通信
    #─────────────
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

        # 戻値を取得する まはた タイムアウト まで、繰り返す。
        resp_chars = []
        start = self.adapter.now_ms()
        while len(resp_chars) < RES_LENGTH and self.adapter.ticks_diff(self.adapter.now_ms(), start) < int(timeout_ms):

            # 受信バッファの先頭1文字を取得する。
            ch = self.adapter.read_one_char()

            # 取得できれば文字列に追加、出来なければ少し待つ
            if ch: resp_chars.append(ch)
            else : self.adapter.sleep_ms(1)

        # 戻値の書式が正しければ、正常で返す。
        if len(resp_chars) == RES_LENGTH and resp_chars[-1] == '!': return "".join(resp_chars)

        # 接続情報を更新する。
        self.adapter._lastError = "時間内にMMPの返信を受け取れませんでした。"

        # 失敗で返す。
        return ""