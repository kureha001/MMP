# -*- coding: utf-8 -*-
# 動画ファイラ.py
"""
GUI版エントリー。メニューUIを呼び出すだけの薄いラッパー。
"""
from 動画メニュー実装 import open_video_menu

if __name__ == "__main__":
    # サンプル
    VIDEOS = {
        "mech_saurus": {
            "title": "MECH SAURUS ストーリー",
            "desc" : "恐竜を救出する物語",
            "path" : "./games/02_MechSaurus.mpg",
        },
        "mech_hunter": {
            "title": "MECH SAURUS 遊び方",
            "desc" : "遊び方の紹介",
            "path" : "./games/02_MechSaurus実録.mp4",
        },
    }
    open_video_menu(VIDEOS, window_title="MECH 動画メニュー")
