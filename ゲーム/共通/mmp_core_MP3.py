# -*- coding: utf-8 -*-
# filename : mmp_core_MP3.py
#============================================================
# ＭＭＰコマンド：ＭＰ３プレイヤー
# バージョン：0.5
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_com import _DECtoHEX1, _DECtoHEX2, _HEX4toDEC

class _MP3:
#━━━━━━━━━━━━━━━
# コンストラクタ
#━━━━━━━━━━━━━━━
    def __init__(self, p:'MmpClient', argTimeOut):
        self._p      = p
        self.TimeOut = argTimeOut
        self.INFO    = self._Info (p, self.TimeOut)
        self.SET     = self._Set  (p, self.TimeOut)
        self.TRACK   = self._Track(p, self.TimeOut)

#━━━━━━━━━━━━━━━
# コマンド
#━━━━━━━━━━━━━━━
    #─────────────
    # サブ：デバイス設定
    #─────────────
    class _Set:
        #─────────────
        # コンストラクタ
        #─────────────
        def __init__(self, p, argTimeOut):
            self._p = p
            self.TimeOut = argTimeOut

        #─────────────
        # 音量
        #─────────────
        def VOLUME(self,
            dev     :int,   # ① デバイスID
            volume  :int,   # ② 音量
        ) -> bool:
            cmd = "MP3/SET/VOLUME"
            cmd = f"{cmd}:{_DECtoHEX2(dev)}:{_DECtoHEX2(volume)}!"
            res = self._p._send_command(cmd, self.TimeOut) 
            return res == "!!!!!"

        #─────────────
        # イコライザ
        #─────────────
        def EQ(self,
            dev     :int,   # ① デバイスID
            mode    :int,   # ② モード
        ) -> bool:
            cmd = "MP3/SET/EQ"
            cmd = f"{cmd}:{_DECtoHEX2(dev)}:{_DECtoHEX2(mode)}!"
            res = self._p._send_command(cmd, self.TimeOut) 
            return res == "!!!!!"

    #─────────────
    # サブ：トラック
    #─────────────
    class _Track:
        #─────────────
        # コンストラクタ
        #─────────────
        def __init__(self, p, argTimeOut):
            self._p = p
            self.TimeOut = argTimeOut

        #─────────────
        # トラック再生
        #─────────────
        def PLAY(self,
            dev     :int,   # ① デバイスID
            dir     :int,   # ② フォルダID
            file    :int,   # ③ ファイルID(フォルダ内)
        ) -> int:
            cmd = "MP3/TRACK/PLAY"
            cmd = f"{cmd}:{_DECtoHEX2(dev)}:{_DECtoHEX2(dir)}:{_DECtoHEX2(file)}!"
            res = self._p._send_command(cmd, self.TimeOut) 
            ok, v = _HEX4toDEC(res); 
            return (v) if ok else (-1)

        #─────────────
        # ループ再生有無
        #─────────────
        def LOOP(self,
            dev     :int,   # ① デバイスID
            enable  :int,   # ② ループ再生有無
        ) -> int:
            cmd = "MP3/TRACK/LOOP"
            cmd = f"{cmd}:{_DECtoHEX2(dev)}:{_DECtoHEX1(1 if enable else 0)}!"
            res = self._p._send_command(cmd, self.TimeOut) 
            ok, v = _HEX4toDEC(res); 
            return (v) if ok else (-1)

        #─────────────
        # 停止
        #─────────────
        def STOP(self,
            dev :int,   # ① デバイスID
        ) -> int:
            cmd = "MP3/TRACK/STOP"
            cmd = f"{cmd}:{_DECtoHEX2(dev)}!"
            res = self._p._send_command(cmd, self.TimeOut) 
            ok, v = _HEX4toDEC(res); 
            return (v) if ok else (-1)

        #─────────────
        # 一時停止
        #─────────────
        def PAUSE(self,
            dev :int,   # ① デバイスID
        ) -> int:
            cmd = "MP3/TRACK/PAUSE"
            cmd = f"{cmd}:{_DECtoHEX2(dev)}!"
            res = self._p._send_command(cmd, self.TimeOut) 
            ok, v = _HEX4toDEC(res); 
            return (v) if ok else (-1)

        #─────────────
        # 開始
        #─────────────
        def START(self,
            dev :int,   # ① デバイスID
        ) -> int:
            cmd = "MP3/TRACK/START"
            cmd = f"{cmd}:{_DECtoHEX2(dev)}!"
            res = self._p._send_command(cmd, self.TimeOut) 
            ok, v = _HEX4toDEC(res); 
            return (v) if ok else (-1)

    #─────────────
    # サブ：インフォメーション
    #─────────────
    class _Info:
        #─────────────
        # コンストラクタ
        #─────────────
        def __init__(self, p, argTimeOut):
            self._p = p
            self.TimeOut = argTimeOut

        #─────────────
        # デバイス接続状況
        #─────────────
        def CONNECT(self, deb:int) -> int:
            cmd = "MP3/INFO/CONNECT"
            cmd = f"{cmd}:{_DECtoHEX2(deb)}!"
            res = self._p._send_command(cmd, self.TimeOut) 
            ok, v = _HEX4toDEC(res); 
            return v if ok else -1

        #─────────────
        # 再生状況
        # 音量
        # イコライザ
        # 現在のファイル番号
        # 総ファイル総数
        #─────────────
        def TRACK (self, dev:int) -> int: return self._Info(dev, "TRACK" )
        def VOLUME(self, dev:int) -> int: return self._Info(dev, "VOLUME")
        def EQ    (self, dev:int) -> int: return self._Info(dev, "EQ"    )
        def FILEID(self, dev:int) -> int: return self._Info(dev, "FILEID")
        def FILES (self, dev:int) -> int: return self._Info(dev, "FILES" )

    #━━━━━━━━━━━━━━━
    # 内部ヘルパ
    #━━━━━━━━━━━━━━━
        #─────────────
        # ステータス計コマンド共通
        #─────────────
        def _Info(self,
            argDev :int    ,   # ① デバイスID
            argCmd :str    ,   # ② 種類ID(１～５)
        ) -> int:           # 戻り値：ＭＭＰコマンドの戻り値
            cmd = f"MP3/INFO/{argCmd}:{_DECtoHEX2(argDev)}!"
            res = self._p._send_command(cmd, self.TimeOut)
            ok, v = _HEX4toDEC(res); 
            return (v) if ok else (-1)
