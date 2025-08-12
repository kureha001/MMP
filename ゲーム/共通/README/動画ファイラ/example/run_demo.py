# -*- coding: utf-8 -*-
# example/run_demo.py
from video_gui_player import open_video_menu

VIDEOS = {
    "mech_saurus": {
        "title": "MECH SAURUS ストーリー",
        "desc" : "恐竜を救出する物語",
        "path" : "./games/02_MechSaurus.mpg",  # 相対パスOK（存在するパスに置き換えてください）
    },
    "mech_hunter": {
        "title": "MECH SAURUS 遊び方",
        "desc" : "遊び方の紹介",
        "path" : "./games/02_MechSaurus実録.mp4",
    },
    # "site": {"title": "公式サイト", "desc": "", "path": "https://example.com"}
}

if __name__ == "__main__":
    open_video_menu(
        VIDEOS,
        window_title="MECH 動画メニュー",
        default_volume=70,
        countdown_secs=3,
        initial_mute=False,
        initial_maximized=False,
        initial_fullscreen=False,
        initial_autoplay=True,
    )
