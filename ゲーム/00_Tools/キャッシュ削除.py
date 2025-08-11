# -*- coding: utf-8 -*-
# tk_pycache_cleaner_checklist.py
#
# 概要：
# ・辞書で定義した各グループ（共通/各Mech〜）をチェックボックスで選択
# ・「全選択」「全解除」ボタンを追加
# ・選択されたグループのみキャッシュを削除
# ・存在しないパスはスキップ、完了ダイアログ表示

from pathlib import Path
import shutil
import tkinter as tk
from tkinter import ttk, messagebox, filedialog

# ------------------------------------------------------------
# 対象リスト定義（基準フォルダからの相対パス）
# ------------------------------------------------------------
MECH_BULLETS_DIRS = [
    "__pycache__",
    "main/__pycache__",
    "main/データセット/__pycache__",
    "main/コントローラ/変更/__pycache__",
    "main/コントローラ/変更/P動作/__pycache__",
    "main/コントローラ/変更/P衝突/__pycache__",
    "main/コントローラ/変更/P発射/__pycache__",
    "main/コントローラ/変更/P出現/__pycache__",
    "main/コントローラ/結果/__pycache__",
    "main/オブジェクト/キャラ/__pycache__",
    "main/オブジェクト/キャラ/自機/__pycache__",
    "main/オブジェクト/キャラ/敵機/__pycache__",
    "main/オブジェクト/キャラ/弾/__pycache__",
    "main/オブジェクト/演出/爆発/__pycache__",
    "main/オブジェクト/演出/背景/__pycache__",
    "main/シーン/__pycache__",
    "main/シーン/プレイ画面/__pycache__",
    "main/シーン/タイトル画面/__pycache__",
    "main/シーン/終了画面/__pycache__",
    "main/オブジェクト/管理/特殊効果/__pycache__",
    "main/汎用部品/__pycache__",
]

MECH_SAURUS_DIRS = [
    "__pycache__",
    "main/__pycache__",
    "main/データセット/__pycache__",
    "main/コントローラ/変更/__pycache__",
    "main/コントローラ/結果/__pycache__",
    "main/オブジェクト/__pycache__",
    "main/オブジェクト/運搬機/__pycache__",
    "main/オブジェクト/発電機/__pycache__",
    "main/シーン/__pycache__",
    "main/シーン/タイトル画面/__pycache__",
    "main/シーン/プレイ画面/__pycache__",
    "main/シーン/終了画面/__pycache__",
]

MECH_HUNTER_DIRS = [
    "__pycache__",
    "main/__pycache__",
    "main/データセット/__pycache__",
    "main/コントローラ/変更/__pycache__",
    "main/コントローラ/結果/__pycache__",
    "main/オブジェクト/__pycache__",
    "main/オブジェクト/カモロボ/__pycache__",
    "main/シーン/__pycache__",
    "main/シーン/タイトル画面/__pycache__",
    "main/シーン/プレイ画面/__pycache__",
    "main/シーン/終了画面/__pycache__",
]

MECH_TORNADO_DIRS = [
    "__pycache__",
    "main/__pycache__",
    "main/データセット/__pycache__",
    "main/コントローラ/変更/__pycache__",
    "main/コントローラ/結果/__pycache__",
    "main/オブジェクト/__pycache__",
    "main/オブジェクト/選手/__pycache__",
    "main/シーン/__pycache__",
    "main/シーン/タイトル画面/__pycache__",
    "main/シーン/プレイ画面/__pycache__",
    "main/シーン/終了画面/__pycache__",
]

def get_target_map(base: Path) -> dict[str, list[Path]]:
    return {
        "共通": [base / "../共通" / "__pycache__"],
        "01_MechBullets": [base / "../01_MechBullets" / p for p in MECH_BULLETS_DIRS],
        "02_MechSaurus":  [base / "../02_MechSaurus"  / p for p in MECH_SAURUS_DIRS],
        "03_MechHunter":  [base / "../03_MechHunter"  / p for p in MECH_HUNTER_DIRS],
        "04_MechTornado": [base / "../04_MechTornado" / p for p in MECH_TORNADO_DIRS],
    }

def rmdir_quiet(path: Path) -> tuple[bool, str]:
    if not path.exists():
        return (False, "skip")
    try:
        shutil.rmtree(path, ignore_errors=False)
        return (True, "ok")
    except Exception as e:
        return (False, f"error: {e}")

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("キャッシュ・クリーナー")
        self.geometry("880x620")

        self.base_dir = tk.StringVar(value=str(Path.cwd()))

        # グループ選択状態
        self.group_vars: dict[str, tk.BooleanVar] = {}

        self._build_ui()
        self.refresh_targets()

    # ---------------- UI ----------------
    def _build_ui(self):
        # 上段：基準フォルダ選択
        frm_top = ttk.Frame(self)
        frm_top.pack(fill="x", padx=12, pady=(12, 6))

        ttk.Label(frm_top, text="基準フォルダ:").pack(side="left")
        ent = ttk.Entry(frm_top, textvariable=self.base_dir, width=80)
        ent.pack(side="left", padx=6)
        ttk.Button(frm_top, text="基準フォルダを変更…", command=self.choose_base).pack(side="left")

        # 中段：チェックボックス群＋全選択/全解除
        frm_chk = ttk.LabelFrame(self, text="実行対象グループ（チェックして選択）")
        frm_chk.pack(fill="x", padx=12, pady=6)

        self.chk_container = ttk.Frame(frm_chk)
        self.chk_container.pack(fill="x", padx=8, pady=8)

        btns = ttk.Frame(frm_chk)
        btns.pack(fill="x", padx=8, pady=(0, 8))
        ttk.Button(btns, text="全選択", command=self.select_all).pack(side="left")
        ttk.Button(btns, text="全解除", command=self.clear_all).pack(side="left", padx=6)

        # 下段：対象一覧（読み取り専用のプレビュー）
        frm_list = ttk.LabelFrame(self, text="対象パスのプレビュー（相対パス表示）")
        frm_list.pack(fill="both", expand=True, padx=12, pady=6)

        self.listbox = tk.Listbox(frm_list, height=18)
        self.listbox.pack(fill="both", expand=True, padx=8, pady=8)

        # 実行ボタンとオプション
        frm_run = ttk.Frame(self)
        frm_run.pack(fill="x", padx=12, pady=8)

        self.ask_confirm = tk.BooleanVar(value=True)
        ttk.Checkbutton(frm_run, text="削除前に確認ダイアログを表示", variable=self.ask_confirm).pack(side="left")
        ttk.Button(frm_run, text="削除実行", command=self.run_cleanup).pack(side="right", padx=4)
        ttk.Button(frm_run, text="終了", command=self.destroy).pack(side="right")

        # ステータスバー
        self.status = tk.StringVar(value="準備完了")
        ttk.Label(self, textvariable=self.status, anchor="w").pack(fill="x", padx=12, pady=(0, 10))

    # ------------ データ更新 ------------
    def choose_base(self):
        selected = filedialog.askdirectory(title="基準フォルダを選択")
        if selected:
            self.base_dir.set(selected)
            self.refresh_targets()

    def refresh_targets(self):
        # チェックボックスを再構築
        for w in self.chk_container.winfo_children():
            w.destroy()

        base = Path(self.base_dir.get())
        tmap = get_target_map(base)

        # 既存の選択状態を保持（初回は全部Trueに）
        prev = {k: v.get() for k, v in self.group_vars.items()} if self.group_vars else {}

        self.group_vars.clear()
        col_frame = ttk.Frame(self.chk_container)
        col_frame.pack(fill="x")

        # チェックボックスを横並びに配置
        for idx, group in enumerate(tmap.keys()):
            var = tk.BooleanVar(value=prev.get(group, True))
            self.group_vars[group] = var
            ttk.Checkbutton(col_frame, text=group, variable=var).grid(row=0, column=idx, padx=6, pady=4, sticky="w")

        # プレビュー更新
        self.refresh_preview_list()

    def refresh_preview_list(self):
        self.listbox.delete(0, tk.END)
        base = Path(self.base_dir.get())
        tmap = get_target_map(base)

        for group, paths in tmap.items():
            sel = "✔" if self.group_vars.get(group, tk.BooleanVar(value=False)).get() else " "
            self.listbox.insert(tk.END, f"[{sel}] {group}")
            for p in paths:
                try:
                    rel = p.relative_to(base)
                except Exception:
                    rel = p
                self.listbox.insert(tk.END, f"  {rel}")
        self.status.set("対象プレビューを更新しました。")

    # ------------ チェック操作 ------------
    def select_all(self):
        for var in self.group_vars.values():
            var.set(True)
        self.refresh_preview_list()

    def clear_all(self):
        for var in self.group_vars.values():
            var.set(False)
        self.refresh_preview_list()

    # ------------ 実行 ------------
    def run_cleanup(self):
        base = Path(self.base_dir.get())
        tmap = get_target_map(base)

        # 選択されたグループを抽出
        selected_groups = [g for g, v in self.group_vars.items() if v.get()]
        if not selected_groups:
            messagebox.showinfo("情報", "実行対象のグループが選択されていません。")
            return

        if self.ask_confirm.get():
            detail = "\n".join(selected_groups)
            ok = messagebox.askyesno(
                "確認",
                "以下のグループでキャッシュを削除します。\n"
                f"基準フォルダ：\n{base}\n\n"
                f"対象グループ：\n{detail}\n\n実行してよろしいですか？"
            )
            if not ok:
                self.status.set("キャンセルしました。")
                return

        removed = skipped = errors = 0
        error_msgs = []

        for group in selected_groups:
            for p in tmap[group]:
                done, msg = rmdir_quiet(p)
                if msg == "ok":
                    removed += 1
                elif msg == "skip":
                    skipped += 1
                else:
                    errors += 1
                    error_msgs.append(f"{p}: {msg}")

        summary = f"[{', '.join(selected_groups)}] → 削除:{removed} / なし:{skipped} / エラー:{errors}"
        self.status.set(summary)

        if errors == 0:
            messagebox.showinfo("完了", f"キャッシュの削除が完了しました。\n{summary}")
        else:
            messagebox.showwarning(
                "完了（エラーあり）",
                f"処理は完了しましたが、エラーが発生しました。\n{summary}\n\n詳細：\n" + "\n".join(error_msgs[:20])
            )

if __name__ == "__main__":
    # DPI対策（任意）
    try:
        from ctypes import windll
        windll.shcore.SetProcessDpiAwareness(1)
    except Exception:
        pass

    app = App()
    # 基準が変わった/チェックが変わったときのプレビュー更新
    app.base_dir.trace_add("write", lambda *_: app.refresh_targets())
    app.mainloop()
