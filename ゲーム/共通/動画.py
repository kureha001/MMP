# -*- coding: utf-8 -*-
# 動画.py
"""
関数版API。GUI版と同じ引数で再生できます。
- 同期: 現在プロセス内で Tk/VLC を開いてブロック
- 非同期: 別プロセス（動画_子プロセス.py）を起動してすぐ戻る
"""
import os
import sys
import subprocess
import tkinter as tk
import webbrowser
from 動画コア import VlcTkPlayer, URL_RE, is_direct_video_url


def _run_player_here(title: str, path: str, **opts):
    """現在プロセス内でプレイヤーを起動してブロック"""
    if URL_RE.match(path) and not is_direct_video_url(path):
        webbrowser.open(path)
        return

    root = tk.Tk()
    def _back():
        try:
            root.quit()
        except Exception:
            pass

    VlcTkPlayer(
        root,
        video_path=path,
        video_title=title or "Video",
        on_finish=_back,
        default_volume=opts.get("default_volume", 80),
        countdown_secs=opts.get("countdown_secs", 3),
        initial_mute=opts.get("initial_mute", False),
        initial_maximized=opts.get("initial_maximized", False),
        initial_fullscreen=opts.get("initial_fullscreen", False),
        initial_autoplay=opts.get("initial_autoplay", True),
    )
    root.mainloop()


def play_media(
    title: str,
    path: str,
    *,
    default_volume: int = 80,
    countdown_secs: int = 3,
    initial_mute: bool = False,
    initial_maximized: bool = False,
    initial_fullscreen: bool = False,
    initial_autoplay: bool = True,
    background: bool = False,
):
    """
    単一メディアを再生する関数API。

    - background=False（既定）: 現在プロセスで起動し、ウィンドウを閉じるまでブロック。戻り値 None。
    - background=True: 別プロセスで '動画_子プロセス.py' を起動し、すぐ戻る。戻り値は subprocess.Popen。
    """
    if not background:
        _run_player_here(
            title, path,
            default_volume=int(default_volume),
            countdown_secs=int(countdown_secs),
            initial_mute=bool(initial_mute),
            initial_maximized=bool(initial_maximized),
            initial_fullscreen=bool(initial_fullscreen),
            initial_autoplay=bool(initial_autoplay),
        )
        return None

    # 非同期: 子プロセス起動
    script_dir = os.path.dirname(os.path.abspath(__file__))
    child_path = os.path.join(script_dir, "動画_子プロセス.py")
    cmd = [
        sys.executable, child_path,
        "--title", title,
        "--path", path,
        "--volume", str(int(default_volume)),
        "--countdown", str(int(countdown_secs)),
        "--mute", "1" if initial_mute else "0",
        "--maximized", "1" if initial_maximized else "0",
        "--fullscreen", "1" if initial_fullscreen else "0",
        "--autoplay", "1" if initial_autoplay else "0",
    ]
    return subprocess.Popen(cmd)


if __name__ == "__main__":
    # 簡易CLI: python 動画.py --title タイトル --path ./movie.mp4 --mute 0/1 ...
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument("--title", required=True)
    p.add_argument("--path", required=True)
    p.add_argument("--volume", type=int, default=80)
    p.add_argument("--countdown", type=int, default=3)
    p.add_argument("--mute", type=int, default=0)
    p.add_argument("--maximized", type=int, default=0)
    p.add_argument("--fullscreen", type=int, default=0)
    p.add_argument("--autoplay", type=int, default=1)
    p.add_argument("--bg", type=int, default=0)
    a = p.parse_args()
    play_media(
        a.title, a.path,
        default_volume=a.volume,
        countdown_secs=a.countdown,
        initial_mute=bool(a.mute),
        initial_maximized=bool(a.maximized),
        initial_fullscreen=bool(a.fullscreen),
        initial_autoplay=bool(a.autoplay),
        background=bool(a.bg),
    )
