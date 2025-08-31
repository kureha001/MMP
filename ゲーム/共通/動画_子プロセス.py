# -*- coding: utf-8 -*-
# 動画_子プロセス.py
import os, sys

# 直実行でも -m モジュール実行でも動くように両対応
if __package__ is None or __package__ == "":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    if script_dir not in sys.path:
        sys.path.insert(0, script_dir)
    from 動画コア import VlcTkPlayer, URL_RE, is_direct_video_url
else:
    from 動画コア import VlcTkPlayer, URL_RE, is_direct_video_url

import argparse, tkinter as tk, webbrowser

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--title", required=True)
    ap.add_argument("--path", required=True)
    ap.add_argument("--volume", type=int, default=80)
    ap.add_argument("--countdown", type=int, default=3)
    ap.add_argument("--mute", type=int, default=0)
    ap.add_argument("--maximized", type=int, default=0)
    ap.add_argument("--fullscreen", type=int, default=0)
    ap.add_argument("--autoplay", type=int, default=1)
    args = ap.parse_args()

    # URLの扱い：直リンク以外は既定ブラウザで開いて終了
    if URL_RE.match(args.path) and not is_direct_video_url(args.path):
        webbrowser.open(args.path)
        return

    root = tk.Tk()
    def _back():
        try:
            root.quit()
        except Exception:
            pass

    VlcTkPlayer(
        root,
        video_path=args.path,
        video_title=args.title,
        on_finish=_back,
        default_volume=args.volume,
        countdown_secs=args.countdown,
        initial_mute=bool(args.mute),
        initial_maximized=bool(args.maximized),
        initial_fullscreen=bool(args.fullscreen),
        initial_autoplay=bool(args.autoplay),
    )
    root.mainloop()

if __name__ == "__main__":
    main()
