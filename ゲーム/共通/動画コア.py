# -*- coding: utf-8 -*-
# 動画コア.py
"""
VLC埋め込みプレイヤーのコア実装
- VlcTkPlayer: Tkinterフレーム内にVLCを描画し、基本コントロールを提供
- URL/拡張子の判定や VLC 起動引数の構築を共通化
"""
import os
import sys
import re
import tempfile
import tkinter as tk
from tkinter import ttk
from typing import List

# ====== 判定用（共通） ======
URL_RE = re.compile(r"^https?://", re.IGNORECASE)
STREAM_EXTS = (".mp4", ".mpg", ".mpeg", ".avi", ".mov", ".mkv", ".m3u8")

# ====== VLC ロード ======
VLC_DIR = os.path.dirname(os.path.abspath(__file__))
if sys.platform.startswith("win"):
    try:
        os.add_dll_directory(VLC_DIR)
    except Exception:
        pass

import vlc  # pip install python-vlc


def build_vlc_args() -> List[str]:
    """OSDを完全に抑止し、pluginsパスを指定した VLC 引数を返す。"""
    args: List[str] = []
    plugins_dir = os.path.join(VLC_DIR, "plugins")
    if os.path.isdir(plugins_dir):
        args.append(f"--plugin-path={plugins_dir}")
    # OSD抑止
    args.append("--no-video-title-show")
    args.append("--no-snapshot-preview")
    args.append("--no-osd")
    return args


def is_direct_video_url(path: str) -> bool:
    """URLが映像ファイルの直リンクかどうかを判定"""
    return bool(URL_RE.match(path) and any(path.lower().endswith(ext) for ext in STREAM_EXTS))


class VlcTkPlayer:
    """
    Tkinter に VLC を埋め込むプレイヤー。
    機能: 再生/一時停止、停止、シーク、音量、ミュート、最大化、フルスクリーン、
         起動時プレビュー（autoplay=False時）、終了時の最終フレーム保持＋カウントダウン復帰。
    """
    def __init__(
        self,
        root: tk.Tk,
        video_path: str,
        video_title: str,
        on_finish,
        *,
        default_volume: int = 80,
        countdown_secs: int = 3,
        initial_mute: bool = False,
        initial_maximized: bool = False,
        initial_fullscreen: bool = False,
        initial_autoplay: bool = True,
    ):
        self.root = root
        self.root.title(video_title or "Video Player")
        self.root.geometry("960x540")

        self.video_path = video_path
        self.on_finish = on_finish
        self._countdown_secs = int(countdown_secs)
        self._default_volume = int(default_volume)
        self._initial_autoplay = bool(initial_autoplay)

        # ---- 状態
        self._ending = False
        self._updating = False
        self._ever_played = False
        self._prerolling = False

        # after() のID（破棄時にキャンセルする）
        self._after_update_id = None
        self._after_tick_id = None

        # ウィンドウ状態
        self._prev_geom = None
        self._zoom_method = None
        self._fs_prev_geom = None
        self._fs_method = None
        self._is_fs = False

        # 最後のフレーム用スナップショット
        self._snap_path = os.path.join(tempfile.gettempdir(), f"vlc_lastframe_{id(self)}.png")
        self._snap_last_ms = -1
        self._snap_interval_ms = 700

        # オーバーレイ要素（常に初期化しておく）
        self._overlay = None
        self._ov_img = None
        self._ov_img_id = None
        self._ov_band_id = None
        self._ov_text_id = None

        # --- UI ---
        self.video_frame = ttk.Frame(self.root)
        self.video_frame.pack(fill="both", expand=True)

        self.ctrl = ttk.Frame(self.root, padding=6)
        self.ctrl.pack(fill="x")

        self.btn_play = ttk.Button(self.ctrl, text="再生", width=6, command=self.toggle_play)
        self.btn_play.pack(side="left")
        ttk.Button(self.ctrl, text="停止", width=6, command=self.stop).pack(side="left", padx=(6, 12))

        self.pos_var = tk.DoubleVar()
        self.scale = ttk.Scale(self.ctrl, from_=0, to=1000, orient="horizontal",
                               variable=self.pos_var, command=self._on_seek_drag)
        self.scale.pack(side="left", fill="x", expand=True)

        self.time_label = ttk.Label(self.ctrl, text="00:00 / 00:00", width=16)
        self.time_label.pack(side="left", padx=(8, 8))

        vol_box = ttk.Frame(self.ctrl)
        vol_box.pack(side="right")
        ttk.Label(vol_box, text="音量").pack(side="left")
        self.vol_scale = ttk.Scale(vol_box, from_=0, to=100, orient="horizontal",
                                   length=140, command=self.on_volume)
        self.vol_scale.set(self._default_volume)
        self.vol_scale.pack(side="left", padx=(6, 6))

        self.mute_var = tk.BooleanVar(value=bool(initial_mute))
        ttk.Checkbutton(vol_box, text="ミュート", variable=self.mute_var,
                        command=self.toggle_mute).pack(side="left", padx=(0, 8))

        self.max_var = tk.BooleanVar(value=bool(initial_maximized))
        ttk.Checkbutton(vol_box, text="最大化", variable=self.max_var,
                        command=self.toggle_maximize).pack(side="left")

        # --- VLC セットアップ ---
        vlc_args = build_vlc_args()
        self.instance = vlc.Instance(vlc_args)
        self.player = self.instance.media_player_new()

        # ロゴ/マーキー無効
        try:
            self.player.video_set_logo_int(vlc.VideoLogoOption.enable, 0)
            self.player.video_set_marquee_int(vlc.VideoMarqueeOption.Enable, 0)
        except Exception:
            pass

        self.root.update_idletasks()
        handle = self.video_frame.winfo_id()
        if sys.platform.startswith("win"):
            self.player.set_hwnd(handle)
        elif sys.platform == "darwin":
            self.player.set_nsobject(handle)
        else:
            self.player.set_xwindow(handle)

        media = self.instance.media_new(self.video_path)
        self.player.set_media(media)

        # 初期音量/ミュート（起動時の鳴り回避）
        self._desired_volume = int(self._default_volume)
        try:
            if self.mute_var.get():
                self.player.audio_set_volume(0)
                self.player.audio_set_mute(True)
                self.player.audio_set_volume(self._desired_volume)
            else:
                self.player.audio_set_volume(self._desired_volume)
                self.player.audio_set_mute(False)
        except Exception:
            pass

        # イベント
        self.events = self.player.event_manager()
        self.events.event_attach(vlc.EventType.MediaPlayerEndReached, self._on_end)
        self.events.event_attach(vlc.EventType.MediaPlayerPlaying,
                                 lambda e: self._enforce_audio_state())

        # キーバインド
        root.bind("<space>", lambda e: self.toggle_play())
        root.bind("<Up>",   lambda e: self.nudge_volume(+5))
        root.bind("<Down>", lambda e: self.nudge_volume(-5))
        root.bind("<F11>",  lambda e: self._toggle_fullscreen())
        root.bind("<Escape>", lambda e: self._on_escape())
        root.bind("q",        lambda e: self._finish())
        root.bind("<Control-q>", lambda e: self._finish())

        # ×でも必ず on_finish
        self.root.protocol("WM_DELETE_WINDOW", self.close)

        # 初期表示：フルスクリーン優先→最大化
        if initial_fullscreen:
            self._apply_fullscreen(True)
        else:
            self._apply_maximize(self.max_var.get())

        # 再生開始 or プレビュー
        if self._initial_autoplay:
            self.player.play()
            self.btn_play.config(text="一時停止")
            self._ever_played = True
        else:
            self._prepare_preview_frame()

        # デバイス初期化遅延への再適用
        self.root.after(200,  self._enforce_audio_state)
        self.root.after(1000, self._enforce_audio_state)

        # 進捗監視開始
        self._after_update_id = self.root.after(200, self._schedule_update)

    # -------- ユーティリティ群 --------
    def _prepare_preview_frame(self):
        self._prerolling = True
        try:
            self.player.audio_set_mute(True)
            self.player.audio_set_volume(0)
        except Exception:
            pass

        self.player.play()

        def _pause_when_ready():
            st = self.player.get_state()
            if st not in (vlc.State.Playing, vlc.State.Buffering):
                self._after_update_id = self.root.after(50, _pause_when_ready); return
            try:
                self.player.set_time(0)
                self.player.set_pause(True)
            except Exception:
                pass
            self._prerolling = False
            self._enforce_audio_state()
            self.btn_play.config(text="再生")
        self._after_update_id = self.root.after(50, _pause_when_ready)

    def _maybe_snapshot(self, current_ms: int):
        if current_ms is None or current_ms < 0:
            return
        if abs(current_ms - self._snap_last_ms) < self._snap_interval_ms:
            return
        try:
            self.player.video_take_snapshot(0, self._snap_path, 0, 0)
            self._snap_last_ms = current_ms
        except Exception:
            pass

    def _enforce_audio_state(self):
        def _apply():
            if getattr(self, "_prerolling", False):
                return
            target_muted = bool(self.mute_var.get())
            try:
                self.player.audio_set_mute(target_muted)
                if target_muted:
                    self.player.audio_set_volume(self._desired_volume)
                else:
                    self.player.audio_set_volume(int(float(self.vol_scale.get())))
            except Exception:
                pass
        try:
            self.root.after(0, _apply)
        except tk.TclError:
            pass

    # -------- 操作 --------
    def toggle_play(self):
        state = self.player.get_state()
        if state in (vlc.State.Playing, vlc.State.Buffering):
            self.player.pause()
            self.btn_play.config(text="再生")
        else:
            self.player.play()
            self.btn_play.config(text="一時停止")
            self._ever_played = True

    def stop(self):
        self.player.stop()
        self._finish()

    def on_volume(self, value):
        try:
            if self.mute_var.get():
                self.mute_var.set(False)
                self.player.audio_set_mute(False)
            vol = int(float(value))
            self._desired_volume = vol
            self.player.audio_set_volume(vol)
        except Exception:
            pass

    def toggle_mute(self):
        try:
            muted = bool(self.mute_var.get())
            self.player.audio_set_mute(muted)
            if not muted:
                self.player.audio_set_volume(int(float(self.vol_scale.get())))
        except Exception:
            pass

    def nudge_volume(self, delta):
        try:
            cur = int(float(self.vol_scale.get()))
            new = max(0, min(100, cur + delta))
            self.vol_scale.set(new)
            self.on_volume(new)
        except Exception:
            pass

    def _on_seek_drag(self, value):
        try:
            self.player.set_position(float(value) / 1000.0)
        except Exception:
            pass

    def toggle_maximize(self):
        if self._is_fs:
            return
        self._apply_maximize(self.max_var.get())

    def _apply_maximize(self, on: bool):
        try:
            if on:
                self._prev_geom = self.root.geometry()
                try:
                    self.root.state("zoomed"); self._zoom_method = "state"; return
                except Exception:
                    pass
                try:
                    self.root.attributes("-zoomed", True); self._zoom_method = "attr"; return
                except Exception:
                    pass
                self._zoom_method = None
                w, h = self.root.winfo_screenwidth(), self.root.winfo_screenheight()
                self.root.geometry(f"{w}x{h}+0+0")
            else:
                if self._zoom_method == "state":
                    try: self.root.state("normal")
                    except Exception: pass
                elif self._zoom_method == "attr":
                    try: self.root.attributes("-zoomed", False)
                    except Exception: pass
                else:
                    if self._prev_geom:
                        self.root.geometry(self._prev_geom)
        except Exception:
            pass

    def _toggle_fullscreen(self):
        self._apply_fullscreen(not self._is_fs)

    def _apply_fullscreen(self, on: bool):
        try:
            if on:
                self._fs_prev_geom = self.root.geometry()
                try:
                    self.root.attributes("-fullscreen", True)
                    self._fs_method = "tk"; self._is_fs = True; return
                except Exception:
                    pass
                try:
                    self.player.set_fullscreen(True)
                    self._fs_method = "vlc"; self._is_fs = True; return
                except Exception:
                    pass
                self.root.overrideredirect(True)
                w, h = self.root.winfo_screenwidth(), self.root.winfo_screenheight()
                self.root.geometry(f"{w}x{h}+0+0")
                try: self.root.attributes("-topmost", True)
                except Exception: pass
                self._fs_method = "override"; self._is_fs = True
            else:
                if self._fs_method == "tk":
                    try: self.root.attributes("-fullscreen", False)
                    except Exception: pass
                elif self._fs_method == "vlc":
                    try: self.player.set_fullscreen(False)
                    except Exception: pass
                elif self._fs_method == "override":
                    try: self.root.overrideredirect(False)
                    except Exception: pass
                    try: self.root.attributes("-topmost", False)
                    except Exception: pass
                    if self._fs_prev_geom:
                        self.root.geometry(self._fs_prev_geom)
                self._fs_method = None
                self._is_fs = False
        except Exception:
            pass

    def _on_escape(self):
        if self._is_fs:
            self._apply_fullscreen(False)
        else:
            self._finish()

    @staticmethod
    def _fmt_ms(ms):
        s = int(ms // 1000) if ms and ms > 0 else 0
        return f"{s//60:02d}:{s%60:02d}"

    def _schedule_update(self):
        self._updating = True
        try:
            length_ms = self.player.get_length()
            time_ms = self.player.get_time()
            if length_ms and length_ms > 0:
                try:
                    self.pos_var.set(self.player.get_position() * 1000.0)
                except tk.TclError:
                    pass
                self.time_label.config(text=f"{self._fmt_ms(time_ms)} / {self._fmt_ms(length_ms)}")

                if self.player.get_state() in (vlc.State.Playing, vlc.State.Buffering) and not self._prerolling:
                    self._maybe_snapshot(time_ms)
            else:
                # 長さが取れないストリーム向けの表記（簡易）
                try:
                    self.time_label.config(text=f"{self._fmt_ms(time_ms)} / --:--")
                except Exception:
                    pass

            # ---- 終了検知 ----
            state = self.player.get_state()
            # 1) 通常の終端・エラー
            if state in (vlc.State.Ended, vlc.State.Error) or (state == vlc.State.Stopped and self._ever_played):
                self._finish(); return
            # 2) 長さが分かる場合の末尾フェイルセーフ
            if (not self._ending and length_ms and length_ms > 0
                    and time_ms is not None and time_ms >= length_ms - 250):
                self._finish(); return
            # 3) 長さが分からない場合の位置ベース判定
            if (not length_ms or length_ms <= 0) and self._ever_played:
                try:
                    pos = self.player.get_position()
                    if pos >= 0.995 and state not in (vlc.State.Paused, vlc.State.Stopped):
                        self._finish(); return
                except Exception:
                    pass

        finally:
            self._updating = False
            # 次回予約（ウィンドウが残っているときのみ）
            try:
                if self.root.winfo_exists():
                    self._after_update_id = self.root.after(200, self._schedule_update)
            except tk.TclError:
                self._after_update_id = None

    def _on_end(self, _event):
        self._finish()

    # ---- 終了カウントダウン ----
    def _finish(self):
        if self._ending:
            return
        self._ending = True
        try:
            self.player.set_pause(True)
            self.player.audio_set_mute(True)
            self._freeze_last_frame_with_overlay()
        finally:
            self._start_countdown(self._countdown_secs)

    def _freeze_last_frame_with_overlay(self):
        path_to_show = None
        if os.path.exists(self._snap_path):
            path_to_show = self._snap_path
        else:
            try:
                self.player.video_take_snapshot(0, self._snap_path, 0, 0)
                if os.path.exists(self._snap_path):
                    path_to_show = self._snap_path
            except Exception:
                pass

        if self._overlay is None:
            self._overlay = tk.Canvas(self.video_frame, highlightthickness=0, bd=0, bg="")
            self._overlay.place(relx=0, rely=0, relwidth=1, relheight=1)
            self._overlay.bind("<Configure>", lambda e: self._layout_overlay())

        if path_to_show and os.path.exists(path_to_show):
            try:
                self._ov_img = tk.PhotoImage(file=path_to_show)
                if self._ov_img_id is None:
                    self._ov_img_id = self._overlay.create_image(0, 0, anchor="nw", image=self._ov_img)
                else:
                    self._overlay.itemconfigure(self._ov_img_id, image=self._ov_img)
            except Exception:
                pass

        self._layout_overlay(initial_text=True)

    def _layout_overlay(self, initial_text=False):
        if self._overlay is None:
            return
        w = max(self._overlay.winfo_width(), 1)
        h = max(self._overlay.winfo_height(), 1)
        band_h = max(int(h * 0.12), 56)
        y1, y2 = h - band_h, h

        if self._ov_band_id is None:
            self._ov_band_id = self._overlay.create_rectangle(
                0, y1, w, y2, fill="#000000", outline="", stipple="gray25"
            )
        else:
            self._overlay.coords(self._ov_band_id, 0, y1, w, y2)

        if self._ov_text_id is None:
            msg = f"あと {self._countdown_secs} 秒でメニューに戻ります（キー/クリックで即戻る）" if initial_text else ""
            self._ov_text_id = self._overlay.create_text(
                w//2, (y1 + y2)//2, text=msg, fill="#FFFFFF",
                font=("Yu Gothic UI", max(int(band_h * 0.45), 14), "bold")
            )
        else:
            self._overlay.coords(self._ov_text_id, w//2, (y1 + y2)//2)
            self._overlay.itemconfigure(
                self._ov_text_id,
                font=("Yu Gothic UI", max(int(band_h * 0.45), 14), "bold")
            )

    def _start_countdown(self, seconds: int):
        for seq in ("<KeyPress>", "<Button-1>", "<Button-2>", "<Button-3>"):
            self.root.bind(seq, self._close_now)
        self._remaining = max(0, int(seconds))
        self._update_countdown_text()
        self._tick()

    def _tick(self):
        if self._remaining <= 0:
            self._close_and_return(); return
        self._remaining -= 1
        self._update_countdown_text()
        try:
            if self.root.winfo_exists():
                self._after_tick_id = self.root.after(1000, self._tick)
        except tk.TclError:
            self._after_tick_id = None

    def _update_countdown_text(self):
        if self._overlay is None or self._ov_text_id is None:
            return
        msg = f"あと {self._remaining} 秒でメニューに戻ります（キー/クリックで即戻る）"
        try:
            self._overlay.itemconfigure(self._ov_text_id, text=msg)
        except Exception:
            pass

    def _close_now(self, _event=None):
        self._close_and_return()

    def _close_and_return(self):
        # after のキャンセル
        try:
            if self._after_update_id:
                self.root.after_cancel(self._after_update_id)
            self._after_update_id = None
        except Exception:
            pass
        try:
            if self._after_tick_id:
                self.root.after_cancel(self._after_tick_id)
            self._after_tick_id = None
        except Exception:
            pass

        for seq in ("<KeyPress>", "<Button-1>", "<Button-2>", "<Button-3>"):
            try:
                self.root.unbind(seq)
            except Exception:
                pass
        try:
            self.root.destroy()
        finally:
            if callable(self.on_finish):
                self.on_finish()

    def close(self):
        self._close_and_return()
