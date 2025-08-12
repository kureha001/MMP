# -*- coding: utf-8 -*-
# video_player_child.py
import os
import sys
import argparse
import tkinter as tk
from tkinter import ttk, messagebox

# ====== 環境準備（libvlc.dll と plugins はこのファイルと同じフォルダ想定） ======
HERE = os.path.dirname(os.path.abspath(__file__))
if sys.platform.startswith("win"):
    try:
        os.add_dll_directory(HERE)
    except Exception:
        pass

import vlc  # pip install python-vlc

# ====== 設定 ======
COUNTDOWN_SECS = 3       # 終了時のカウントダウン秒
DEFAULT_VOLUME  = 80      # 起動時音量（0-100）
FORCE_UNMUTE_ON_START = True  # 起動時は必ずアンミュートにする

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--title", required=True)
    ap.add_argument("--path",  required=True)
    args = ap.parse_args()

    title    = args.title
    abs_path = args.path

    if not os.path.exists(abs_path):
        messagebox.showerror("エラー", f"ファイルが見つかりません:\n{abs_path}")
        return

    # ====== Tk ウィンドウ ======
    root = tk.Tk()
    root.title(title or "Video")
    root.geometry("960x540")

    video_frame = ttk.Frame(root)
    video_frame.pack(fill="both", expand=True)

    ctrl = ttk.Frame(root, padding=6)
    ctrl.pack(fill="x")

    # ====== VLC 準備 ======
    plugins_dir = os.path.join(HERE, "plugins")
    instance = vlc.Instance([f"--plugin-path={plugins_dir}"] if os.path.isdir(plugins_dir) else [])
    player = instance.media_player_new()

    root.update_idletasks()
    handle = video_frame.winfo_id()
    if sys.platform.startswith("win"):
        player.set_hwnd(handle)
    elif sys.platform == "darwin":
        player.set_nsobject(handle)
    else:
        player.set_xwindow(handle)

    media = instance.media_new(abs_path)
    player.set_media(media)

    # ここで明示的にアンミュートして音量セット（早すぎると効かないこともあるが一応）
    try:
        if FORCE_UNMUTE_ON_START:
            player.audio_set_mute(False)
        player.audio_set_volume(DEFAULT_VOLUME)
    except Exception:
        pass

    # ====== 操作部品 ======
    # 再生/一時停止
    def do_play():
        player.play()
        play_btn.config(state="disabled")
        pause_btn.config(state="normal")

    def do_pause():
        player.pause()
        play_btn.config(state="normal")
        pause_btn.config(state="disabled")

    play_btn  = ttk.Button(ctrl, text="再生",     width=8, command=do_play)
    pause_btn = ttk.Button(ctrl, text="一時停止", width=8, command=do_pause)
    stop_btn  = ttk.Button(ctrl, text="停止",     width=8, command=lambda: start_countdown())
    play_btn.pack(side="left")
    pause_btn.pack(side="left", padx=(6, 6))
    stop_btn.pack(side="left", padx=(0, 12))

    # 位置スライダー
    pos_var = tk.DoubleVar(value=0.0)
    dragging = {"on": False}

    def on_seek_start(_):
        dragging["on"] = True

    def on_seek_end(_):
        dragging["on"] = False
        try:
            player.set_position(float(pos_var.get()) / 1000.0)
        except Exception:
            pass

    def on_seek(v):
        if dragging["on"]:
            try:
                player.set_position(float(v) / 1000.0)
            except Exception:
                pass

    scale = ttk.Scale(ctrl, from_=0, to=1000, orient="horizontal",
                      variable=pos_var, command=on_seek)
    scale.pack(side="left", fill="x", expand=True)
    scale.bind("<ButtonPress-1>", on_seek_start)
    scale.bind("<ButtonRelease-1>", on_seek_end)

    # 時間表示
    time_label = ttk.Label(ctrl, text="00:00 / 00:00", width=16)
    time_label.pack(side="left", padx=(8, 12))

    # 音量 + ミュート
    ttk.Label(ctrl, text="音量").pack(side="left")

    def on_volume(v):
        try:
            if mute_var.get():
                mute_var.set(False)
                player.audio_set_mute(False)
            player.audio_set_volume(int(float(v)))
        except Exception:
            pass

    vol_scale = ttk.Scale(ctrl, from_=0, to=100, orient="horizontal",
                          length=140, command=on_volume)
    vol_scale.set(DEFAULT_VOLUME)
    vol_scale.pack(side="left", padx=(6, 12))

    mute_var = tk.BooleanVar(value=False)

    def on_mute():
        try:
            player.audio_set_mute(bool(mute_var.get()))
        except Exception:
            pass

    ttk.Checkbutton(ctrl, text="ミュート", variable=mute_var, command=on_mute).pack(side="left", padx=(0, 12))

    # ====== 全画面（修正版：ダブルトグルなし＋フォールバック） ======
    fs_var = tk.BooleanVar(value=False)
    _prev_geom = {"value": None}

    def apply_fullscreen(on: bool):
        # 1) Tk標準のフルスクリーン
        try:
            root.attributes("-fullscreen", on)
            return
        except Exception:
            pass
        # 2) VLCのフルスクリーン
        try:
            player.set_fullscreen(on)
            return
        except Exception:
            pass
        # 3) 枠なし最大化（フォールバック）
        if on:
            _prev_geom["value"] = root.geometry()
            root.overrideredirect(True)
            w, h = root.winfo_screenwidth(), root.winfo_screenheight()
            root.geometry(f"{w}x{h}+0+0")
            try:
                root.attributes("-topmost", True)
            except Exception:
                pass
        else:
            root.overrideredirect(False)
            try:
                root.attributes("-topmost", False)
            except Exception:
                pass
            if _prev_geom["value"]:
                root.geometry(_prev_geom["value"])

    def on_fs_check():
        apply_fullscreen(fs_var.get())

    def toggle_fs_key():
        fs_var.set(not fs_var.get())
        apply_fullscreen(fs_var.get())

    ttk.Checkbutton(ctrl, text="全画面", variable=fs_var, command=on_fs_check).pack(side="left")

    # ====== 閉じる/カウントダウン ======
    flags = {"closing": False, "countdown": False}

    def close_now():
        if flags["closing"]:
            return
        flags["closing"] = True
        try:
            player.stop()
        except Exception:
            pass
        try:
            root.destroy()
        except Exception:
            pass

    def start_countdown():
        if flags["countdown"]:
            return
        flags["countdown"] = True

        # 最後のフレームを維持（黒画面回避）
        try:
            player.set_pause(True)
            player.audio_set_mute(True)
        except Exception:
            pass

        lbl = ttk.Label(video_frame, text=f"{COUNTDOWN_SECS}秒後に閉じます",
                        background="black", foreground="white",
                        font=("Yu Gothic UI", 20))
        lbl.place(relx=0.5, rely=0.5, anchor="center")

        def tick(rem):
            if rem <= 0:
                close_now()
                return
            lbl.config(text=f"{rem}秒後に閉じます")
            root.after(1000, tick, rem - 1)

        # 任意キー/クリックで即閉じ
        for seq in ("<KeyPress>", "<Button-1>", "<Button-2>", "<Button-3>"):
            root.bind(seq, lambda e: close_now())

        tick(COUNTDOWN_SECS)

    # 自然終了 → カウントダウン
    em = player.event_manager()
    em.event_attach(vlc.EventType.MediaPlayerEndReached, lambda _e: start_countdown())

    # ×でも閉じる
    root.protocol("WM_DELETE_WINDOW", close_now)

    # エスケープ：全画面なら解除、そうでなければ閉じる
    def on_escape(_e=None):
        if fs_var.get():
            fs_var.set(False)
            apply_fullscreen(False)
        else:
            close_now()

    # ====== キーバインド ======
    root.bind("<space>", lambda e: (do_pause() if pause_btn['state'] == "normal" else do_play()))
    root.bind("m",      lambda e: (mute_var.set(not mute_var.get()), on_mute()))
    root.bind("<Up>",   lambda e: (vol_scale.set(min(100, float(vol_scale.get()) + 5)), on_volume(vol_scale.get())))
    root.bind("<Down>", lambda e: (vol_scale.set(max(0,   float(vol_scale.get()) - 5)), on_volume(vol_scale.get())))
    root.bind("<F11>",  lambda e: toggle_fs_key())
    root.bind("f",      lambda e: toggle_fs_key())
    root.bind("<Escape>", on_escape)
    root.bind("q",        lambda e: close_now())
    root.bind("<Control-q>", lambda e: close_now())

    # ====== 状態監視（強制終了/エラーでも閉じる） ======
    def poll():
        st = player.get_state()
        if st in (vlc.State.Ended, vlc.State.Stopped, vlc.State.Error):
            start_countdown()
            return
        root.after(250, poll)

    # UI更新（スライダー・時間）
    def fmt_ms(ms):
        s = int(ms // 1000) if ms and ms > 0 else 0
        return f"{s//60:02d}:{s%60:02d}"

    def update_ui():
        L = player.get_length()
        t = player.get_time()
        if L and L > 0:
            if not dragging["on"]:
                try:
                    pos_var.set(player.get_position() * 1000.0)
                except Exception:
                    pass
            time_label.config(text=f"{fmt_ms(t)} / {fmt_ms(L)}")
        root.after(200, update_ui)

    # ====== 起動 ======
    player.play()
    play_btn.config(state="disabled")
    pause_btn.config(state="normal")

    # 起動後にもう一度アンミュート＆UI同期（デバイス初期化のタイムラグに対応）
    def initial_audio_sync():
        try:
            if FORCE_UNMUTE_ON_START:
                player.audio_set_mute(False)
            m = bool(player.audio_get_mute())
            mute_var.set(m)
        except Exception:
            pass

    root.after(400, initial_audio_sync)   # 1回目
    root.after(1200, initial_audio_sync)  # 念のためもう1回

    root.after(300, poll)
    root.after(200, update_ui)

    # 起動エラーチェック
    def check_start():
        if player.get_state() == vlc.State.Error:
            messagebox.showerror("エラー", f"再生を開始できませんでした:\n{abs_path}")
            close_now()
    root.after(800, check_start)

    root.mainloop()

if __name__ == "__main__":
    main()
