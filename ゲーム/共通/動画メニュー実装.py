# -*- coding: utf-8 -*-
# 動画メニュー実装.py
"""
メニューUIの実装。動画リストを受け取り、選択して 動画コア.VlcTkPlayer を開く。
"""
import os
import tkinter as tk
from tkinter import ttk, messagebox
import webbrowser

from .動画コア import VlcTkPlayer, URL_RE, is_direct_video_url


def open_video_menu(
    videos: dict,
    window_title: str = "動画メニュー",
    *,
    default_volume: int = 80,
    countdown_secs: int = 3,
    initial_mute: bool = False,
    initial_maximized: bool = False,
    initial_fullscreen: bool = False,
    initial_autoplay: bool = True,
) -> None:
    if not isinstance(videos, dict):
        raise TypeError("videos は dict で指定してください")

    def start_player(path: str, title: str):
        # 通常URL（直リンクでない）は既定ブラウザで開く
        if URL_RE.match(path) and not is_direct_video_url(path):
            webbrowser.open(path)
            show_menu()
            return

        # ローカル/直リンク動画はVLC埋め込みで再生
        root = tk.Tk()
        def back_to_menu():
            show_menu()
        VlcTkPlayer(
            root, path, video_title=title, on_finish=back_to_menu,
            default_volume=default_volume, countdown_secs=countdown_secs,
            initial_mute=initial_mute, initial_maximized=initial_maximized,
            initial_fullscreen=initial_fullscreen, initial_autoplay=initial_autoplay
        )
        root.mainloop()

    def start_selected_from(tree, menu_root):
        sel = tree.selection()
        if not sel:
            messagebox.showinfo("情報", "再生する項目を選んでください。")
            return
        key = sel[0]
        info = videos[key]
        path = info.get("path", "")
        title = info.get("title", key)

        # ローカル存在チェック（URLはスキップ）
        if not URL_RE.match(path) and not os.path.exists(path):
            messagebox.showerror("エラー", f"ファイルが見つかりません:\n{path}")
            return

        menu_root.destroy()
        start_player(path, title)

    def show_menu():
        menu_root = tk.Tk()
        menu_root.title(window_title)
        menu_root.geometry("720x460")

        ttk.Label(menu_root, text="再生する動画を選んでください", font=("Yu Gothic UI", 16)).pack(pady=10)

        tree = ttk.Treeview(menu_root, columns=("title", "desc"), show="headings")
        tree.heading("title", text="タイトル")
        tree.heading("desc", text="説明")
        tree.column("title", width=260)
        tree.column("desc", width=420)
        tree.pack(fill="both", expand=True, padx=10, pady=10)

        for key, info in videos.items():
            title = info.get("title", key)
            desc  = info.get("desc", "")
            tree.insert("", "end", iid=key, values=(title, desc))

        tree.bind("<Double-1>", lambda e: start_selected_from(tree, menu_root))

        btn_row = ttk.Frame(menu_root)
        btn_row.pack(pady=6)
        ttk.Button(btn_row, text="再生", command=lambda: start_selected_from(tree, menu_root)).pack(side="left", padx=6)
        ttk.Button(btn_row, text="終了", command=menu_root.destroy).pack(side="left", padx=6)

        menu_root.mainloop()

    show_menu()
