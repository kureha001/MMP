# -*- coding: utf-8 -*-
# mmp_adapter_Tcp_Micro.py
#============================================================
# Pydroid用：usb4aブリッジアダプタ
#------------------------------------------------------------
# [設置方法]
#   スマホとMMPをUSBケーブルで繋ぐ
#------------------------------------------------------------
# [依存関係](pipでインストールが必要)
#   pyserual
#   kivy
#   usb4a
#   usbserial4a
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
import time
from mmp_adapter_base import MmpAdapterBase

# ランタイム依存（Kivy/usb4a/usbserial4a）
_HAS_USB4A = False

try:
    import kivy
    from usb4a       import usb
    from usbserial4a import serial4a
    _HAS_USB4A  = True
except Exception:
    _HAS_USB4A  = False

#=================
# アダプタ クラス
#=================
class MmpAdapter(MmpAdapterBase):

    # 接続情報を初期化する。
    _uart           = None
    _is_open        = False
    _connected_port = None
    _connected_baud = None
    _lastError      = None

    def __init__(self,
        device_index        : int   = 0     ,
        request_permission  : bool  = True  ,
        read_timeout_s      : float = 0.2   ,
        **kwargs
    ) -> None:

        super().__init__()

        self._device_index = int(device_index)
        self._request_perm = bool(request_permission)
        self._read_timeout = float(read_timeout_s)


    # ----------------------------------------------------------------
    # 接続制御
    # ----------------------------------------------------------------
    def open_baud(self, baud: int) -> bool:

        global _HAS_USB4A
        self._lastError = None

        if not _HAS_USB4A:
            self._lastError = "usb4a/usbserial4a がこの環境で使用不可"
            return False

        try:
            # デバイス列挙
            devs = usb.get_usb_device_list()
            if not devs:
                self._lastError = "USBデバイスが見つかりません"
                return False

            # index 範囲チェック
            if self._device_index < 0 or self._device_index >= len(devs):
                self._lastError = f"device_index={self._device_index} が範囲外（0..{len(devs)-1}）"
                return False

            # デバイス取得
            dev = devs[self._device_index]

            # 権限が無ければ要求（初回はダイアログ）
                # 許可はユーザ操作次第。
                # ここでは待たず、そのまま open を試みる
                # （失敗時は except に落ちて False を返す＝既存仕様）
            if not usb.has_usb_permission(dev):
                usb.request_usb_permission(dev)

            # device_name は UsbDevice の getDeviceName()
            device_name = dev.getDeviceName()

            # ポートを開く
            self._uart = serial4a.get_serial_port(
                device_name ,
                baud        ,   # Baudrate
                8           ,   # Data bits
                'N'         ,   # Parity
                1               # Stop bits
            )
            # ポート状態更新
            self._is_open = True
            self._connected_port = f"usb4a://{self._device_index}"
            self._connected_baud = int(baud)

            # オープン直後の入力を捨てて整流
            self.clear_input()

            return True

        except Exception as e:
            self._lastError = f"usb4a open 失敗: {e}"
            self._uart      = None
            self._is_open   = False
            return False

    #------------------------------
    # 受信バッファを消去する
    #------------------------------
    def clear_input(self) -> None:

        if not self._uart: return

        # バッファのデータ量を参照
        try             : n = self._uart.in_waiting
        except Exception: n = 0

        # バッファに溜まっている分を空読み
        if n:
            try             : self._uart.read(n)
            except Exception: pass

    #------------------------------
    # ASCII文字を送信
    #------------------------------
    def write_ascii(self, s: str) -> None:
        if not self._uart: return
        self._uart.write(s.encode("ascii", "ignore"))

    #------------------------------
    # 受信バッファから１文字取得
    #------------------------------
    def read_one_char(self) -> str | None:

        # シリアルが無効な場合は処理を中断
        if not self._uart: return None

        # 受信バッファから１文字取得する
        try:
            # 受信バッファから空以外の場合：
            if self._uart.in_waiting > 0:
                # 受信バッファの先頭１文字を取得する
                b = self._uart.read(1)
                if b:
                    # 取得文字をASCIIから復元して返す
                                                    # 戻り値 → [成功]
                    try             : return b.decode("ascii", "ignore")
                    except Exception: return None   # 戻り値 → [失敗]
        except Exception            : return None   # 戻り値 → [失敗]

        return None                                 # 戻り値 → [失敗]

    #------------------------------
    # 送信フラッシュ
    # USBシリアルに明示 flush があれば呼ぶ。
    # 無ければ no-op
    #------------------------------
    def flush(self) -> None:
        self._uart.flush()

    #------------------------------
    # 通信を切断
    #------------------------------
    def close(self) -> None:
        try:
            if self._uart:
                try: self._uart.close()
                except Exception: pass
        finally:
            self._uart = None
            self._is_open = False


    #========================================================
    # アダプタ共通コマンド：ヘルパ
    #========================================================
    #------------------------------
    # 一時停止
    #------------------------------
    def sleep_ms(self, ms: int) -> None:
        time.sleep(max(0.0, ms) / 1000.0)

    #------------------------------
    # 現在時刻
    #------------------------------
    def now_ms(self) -> int:
        return int(time.monotonic() * 1000)
