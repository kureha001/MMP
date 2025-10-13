# -*- coding: utf-8 -*-
# filename : mmp_core_MP3.py
#============================================================
# ＭＭＰコマンド：ＭＰ３プレイヤー
# バージョン：0.4
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
from mmp_core_COM import _resolve, _hex1, _hex2, _try_parse_hex4_bang

class _AudioModule:
    #----------------------
    # コンストラクタ
    #----------------------
    def __init__(self, p:'MmpClient'):
        self._p = p
        self.Play = self._PlayModule(p)
        self.Read = self._ReadModule(p)

    #----------------------
    # １. 単独コマンド
    #----------------------
    # 1-1. デバイス情報
    #----------------------
    def Info(self, devId1to4:int, timeoutMs:int=0) -> int:
        return self._p.Info.Dev.Audio(devId1to4, timeoutMs)

    #----------------------
    # 1-2. 音量設定
    #----------------------
    def Volume(self, devId1to4:int, vol0to30:int, timeoutMs:int=0) -> bool:
        t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
        resp = self._p._send_command(f"VOL:{_hex2(devId1to4)}:{_hex2(vol0to30)}!", t) 
        return resp == "!!!!!"

    #----------------------
    # 1-3.イコライザ設定
    #----------------------
    def SetEq(self, devId1to4:int, mode0to5:int, timeoutMs:int=0) -> bool:
        t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
        resp = self._p._send_command(f"DEF:{_hex2(devId1to4)}:{_hex2(mode0to5)}!", t) 
        return resp == "!!!!!"

    #----------------------
    # ２. 再生サブモジュール
    #----------------------
    class _PlayModule:
        #----------------------
        # コンストラクタ
        #----------------------
        def __init__(self, p:'MmpClient'): self._p = p

        #----------------------
        # 2-1. 再生（Start）
        #----------------------
        def Start(self, devId1to4:int, dir1to255:int, file1to255:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
            resp = self._p._send_command(f"DIR:{_hex2(devId1to4)}:{_hex2(dir1to255)}:{_hex2(file1to255)}!", t)
            return resp == "!!!!!"

        #----------------------
        # 2-2. ループ再生指定
        #----------------------
        def SetLoop(self, devId1to4:int, enable:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
            resp = self._p._send_command(f"DLP:{_hex2(devId1to4)}:{_hex1(1 if enable else 0)}!", t)
            return resp == "!!!!!"

        #----------------------
        # 2-3. 停止
        #----------------------
        def Stop(self, devId1to4:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
            resp = self._p._send_command(f"DSP:{_hex2(devId1to4)}!", t)
            return resp == "!!!!!"

        #----------------------
        # 2-4. 一時停止
        #----------------------
        def Pause(self, devId1to4:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
            resp = self._p._send_command(f"DPA:{_hex2(devId1to4)}!", t)
            return resp == "!!!!!"

        #----------------------
        # 2-5. 再開
        #----------------------
        def Resume(self, devId1to4:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
            resp = self._p._send_command(f"DPR:{_hex2(devId1to4)}!", t)
            return resp == "!!!!!"

    #----------------------
    # ３. 参照サブモジュール
    #----------------------
    class _ReadModule:
        #----------------------
        # コンストラクタ
        #----------------------
        def __init__(self, p:'MmpClient'): self._p = p

        #----------------------
        # 3-1. 再生状況
        #----------------------
        def State(self, devId1to4:int, timeoutMs:int=0) -> int:
            return self._dst_query(devId1to4, 1, timeoutMs)

        #----------------------
        # 3-2. 音量
        #----------------------
        def Volume(self, devId1to4:int, timeoutMs:int=0) -> int:
            return self._dst_query(devId1to4, 2, timeoutMs)

        #----------------------
        # 3-3. イコライザのモード
        #----------------------
        def Eq(self, devId1to4:int, timeoutMs:int=0) -> int:
            return self._dst_query(devId1to4, 3, timeoutMs)

        #----------------------
        # 3-4. 総ファイル総数
        #----------------------
        def FileCounts(self, devId1to4:int, timeoutMs:int=0) -> int:
            return self._dst_query(devId1to4, 4, timeoutMs)

        #----------------------
        # 3-5. 現在のファイル番号
        #----------------------
        def FileNumber(self, devId1to4:int, timeoutMs:int=0) -> int:
            return self._dst_query(devId1to4, 5, timeoutMs)

        #----------------------
        # 内部ヘルパ
        #----------------------
        def _dst_query(self,
            devId1to4:int   ,   # ① デバイスID
            kind:int        ,   # ② 種類ID(１～５)
            timeoutMs:int   ,   # ③ タイムアウト(単位：ミリ秒)
        ) -> int:               # 戻り値：ＭＭＰコマンドの戻り値
            t     = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
            resp  = self._p._send_command(f"DST:{_hex2(devId1to4)}:{_hex2(kind)}!", t)
            ok, v = _try_parse_hex4_bang(resp); 
            return (v) if ok else (-1)