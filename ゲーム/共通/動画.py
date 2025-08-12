# -*- coding: utf-8 -*-
# video_launcher.py
import os
import re
import sys
import webbrowser
import subprocess
import threading
from typing import Optional, Callable, Union

# URL は既定ブラウザで開く（閉じ検出は不可）
_URL_RE = re.compile(r"^https?://", re.IGNORECASE)

# Windows のコンソール非表示系フラグ
DETACHED_PROCESS   = 0x00000008
CREATE_NO_WINDOW   = 0x08000000
CREATE_NEW_PROCESS = 0x00000010  # 使わないが参考まで


def _wait_and_callback(proc: subprocess.Popen, cb: Callable[[], None]) -> None:
    try:
        proc.wait()
    except Exception:
        pass
    try:
        cb()
    except Exception:
        pass


def play_media(
    title: str,
    path: str,
    *,
    mode: str = "async",                 # "sync" / "async" / "detached"
    on_exit: Optional[Callable[[], None]] = None,  # async時に終了通知が欲しい場合
    hide_console: bool = True,           # Windows: detached時にコンソール非表示
) -> Union[None, subprocess.Popen, int]:
    """
    URLなら既定ブラウザで開く。ローカルファイルなら VLC+Tkinter (video_player_child.py) を起動。

    mode:
      - "sync"     : 再生ウィンドウが閉じるまでブロックして待つ。戻り値はプロセスの returncode (int)。
      - "async"    : すぐ戻る。戻り値は Popen。on_exit を渡すと、終了時に別スレッドでコールバック。
      - "detached" : 完全に投げっぱなし。戻り値は None（プロセスハンドルは返さない）。

    注意:
      - URL（ブラウザ再生）の場合、タブのクローズ検出はできないので、mode は常に即時復帰になります。
    """
    if not isinstance(title, str) or not isinstance(path, str):
        raise TypeError("title, path は str を指定してください")

    # --- URL は既定ブラウザで開く（即復帰） ---
    if _URL_RE.match(path):
        webbrowser.open(path)
        # ブラウザは閉じ検出できないので mode に関係なく None を返す
        return None

    # --- ローカル（相対OK） ---
    abs_path = os.path.abspath(path)
    if not os.path.exists(abs_path):
        raise FileNotFoundError(f"動画ファイルが見つかりません: {abs_path}")

    here  = os.path.dirname(os.path.abspath(__file__))
    child = os.path.join(here, "動画_子プロセス.py")
    if not os.path.exists(child):
        raise FileNotFoundError(f"子プロセス用スクリプトが見つかりません: {child}")

    cmd = [sys.executable, child, "--title", title, "--path", abs_path]

    # --- プロセス起動オプションをモードごとに設定 ---
    popen_kwargs = {}

    if os.name == "nt":
        # Windows: "detached" だけは真正のデタッチ（コンソール非表示）
        if mode == "detached":
            flags = DETACHED_PROCESS | (CREATE_NO_WINDOW if hide_console else 0)
            popen_kwargs["creationflags"] = flags
        # sync / async は creationflags=0（waitやハンドル取得が確実）
    else:
        # POSIX: detached のときは新しいセッションで（Ctrl-C等の親シグナルを切る）
        if mode == "detached":
            popen_kwargs["start_new_session"] = True

    # --- 起動 ---
    proc = subprocess.Popen(cmd, **popen_kwargs)

    # --- モード別の戻り値/コールバック処理 ---
    if mode == "sync":
        rc = proc.wait()
        if callable(on_exit):
            try:
                on_exit()
            except Exception:
                pass
        return rc

    if mode == "async":
        if callable(on_exit):
            th = threading.Thread(target=_wait_and_callback, args=(proc, on_exit), daemon=True)
            th.start()
        return proc

    # mode == "detached"
    return None
