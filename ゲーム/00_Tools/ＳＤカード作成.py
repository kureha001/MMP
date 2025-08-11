# -*- coding: utf-8 -*-
# Windows専用 / Tkinter GUI 版
# 仕様:
# - カレント直下 "FDPlayer" の“中身”を、選択したSDカードのルート直下に再帰コピー
# - フォーマットは任意（FAT32固定、フォーマットしたときのみラベル "FDPLAYER"）
# - 進捗バー、安全取り外し（任意）
# - フォーマット確認は「はい／いいえ」のダイアログで実施（ドライブ再入力なし）

import os
import sys
import shutil
import string
import ctypes
import subprocess
import threading
import time
from pathlib import Path
import tkinter as tk
from tkinter import ttk, messagebox

# ===== 設定 =====
SRC_FOLDER_NAME  = "../DFPlayer"
FORMAT_FS        = "FAT32"
FORMAT_LABEL     = "DFPlayer"
CHUNK_SIZE       = 4 * 1024 * 1024  # 4MB
# ================

# ---- Windows API helpers ----
def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except Exception:
        return False

def _get_drive_type(root):
    return ctypes.windll.kernel32.GetDriveTypeW(ctypes.c_wchar_p(root))

def find_sd_drives():
    DRIVE_REMOVABLE = 2
    drives = []
    for letter in string.ascii_uppercase:
        root = f"{letter}:\\"
        if os.path.exists(root) and _get_drive_type(root) == DRIVE_REMOVABLE:
            drives.append(root)
    return drives

def get_volume_label(drive_root):
    buf = ctypes.create_unicode_buffer(1024)
    ctypes.windll.kernel32.GetVolumeInformationW(
        ctypes.c_wchar_p(drive_root), buf, ctypes.sizeof(buf),
        None, None, None, None, 0
    )
    return buf.value if buf.value else "(ラベルなし)"

# ---- Format / Eject ----
def format_with_powershell(letter, fs, label, quick=True):
    cmd = [
        "powershell", "-NoProfile", "-Command",
        ("Try { "
         f"Format-Volume -DriveLetter '{letter}' -FileSystem {fs} "
         f"-NewFileSystemLabel '{label}' "
         f"{'-Full' if not quick else ''} -Force -Confirm:$false -ErrorAction Stop; "
         "$true } Catch { $false }")
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    return r.returncode == 0 and "True" in r.stdout

def format_with_formatcom(root, fs, label, quick=True):
    drive_spec = root[:2]  # 'E:'
    args = ["cmd", "/c", "format", drive_spec, f"/FS:{fs}", f"/V:{label}", "/Y"]
    if quick:
        args.append("/Q")
    args.append("/X")
    r = subprocess.run(args, capture_output=True, text=True)
    return r.returncode == 0

def try_format(sd_root, quick=True, log=lambda *_: None):
    letter = sd_root[0]
    log(f"[{sd_root}] フォーマット実行中 ({FORMAT_FS}, ラベル='{FORMAT_LABEL}')...")
    ok = format_with_powershell(letter, FORMAT_FS, FORMAT_LABEL, quick=quick)
    if not ok:
        log("PowerShell 失敗。format.com にフォールバックします...")
        ok = format_with_formatcom(sd_root, FORMAT_FS, FORMAT_LABEL, quick=quick)
    return ok

def try_safe_remove(sd_root, log=lambda *_: None):
    letter = sd_root[0]
    # 1) PowerShell: Dismount-Volume
    cmd1 = [
        "powershell", "-NoProfile", "-Command",
        ("Try { "
         f"$v=Get-Volume -DriveLetter '{letter}'; "
         "Dismount-Volume -InputObject $v -Force -ErrorAction Stop; "
         "$true } Catch { $false }")
    ]
    r1 = subprocess.run(cmd1, capture_output=True, text=True)
    if r1.returncode == 0 and "True" in r1.stdout:
        return True
    # 2) Shell.Application Eject
    cmd2 = [
        "powershell", "-NoProfile", "-Command",
        ("$sh=New-Object -ComObject Shell.Application; "
         f"$drv=$sh.NameSpace(17).ParseName('{letter}:'); "
         "if($drv -and $drv.Verbs() | ? { $_.Name -match 'Eject' }) { "
         "($drv.Verbs() | ? { $_.Name -match 'Eject' })[0].DoIt(); $true } else { $false }")
    ]
    r2 = subprocess.run(cmd2, capture_output=True, text=True)
    if r2.returncode == 0 and "True" in r2.stdout:
        return True
    # 3) mountvol
    cmd3 = ["cmd", "/c", "mountvol", f"{letter}:", "/p"]
    r3 = subprocess.run(cmd3, capture_output=True, text=True)
    return r3.returncode == 0

# ---- Copy helpers ----
def walk_contents_files(src_root: Path):
    # 直下のファイル
    for f in src_root.iterdir():
        if f.is_file():
            try:
                yield f, Path(f.name), f.stat().st_size
            except FileNotFoundError:
                continue
    # 直下のフォルダ（再帰）
    for d in src_root.iterdir():
        if d.is_dir():
            for dirpath, _, filenames in os.walk(d):
                dp = Path(dirpath)
                for fn in filenames:
                    p = dp / fn
                    try:
                        rel = p.relative_to(src_root)
                        yield p, rel, p.stat().st_size
                    except FileNotFoundError:
                        continue

def calc_total_size(src_root: Path):
    return sum(sz for _, _, sz in walk_contents_files(src_root))

def copy_with_progress(src_root: Path, sd_root: Path, progress_cb, log=lambda *_: None):
    dest_parent = Path(sd_root)
    total = calc_total_size(src_root)
    done = 0
    start = time.time()

    def fmt_bytes(n):
        units = ["B","KB","MB","GB","TB"]
        i = 0
        n = float(n)
        while n >= 1024 and i < len(units)-1:
            n /= 1024
            i += 1
        return f"{n:.1f} {units[i]}"

    def copy_one(src_file: Path, dst_file: Path):
        nonlocal done
        dst_file.parent.mkdir(parents=True, exist_ok=True)
        with open(src_file, "rb") as rf, open(dst_file, "wb") as wf:
            while True:
                buf = rf.read(CHUNK_SIZE)
                if not buf:
                    break
                wf.write(buf)
                done += len(buf)
                elapsed = max(0.001, time.time() - start)
                speed = done / elapsed
                eta = (total - done) / speed if speed > 0 else 0
                progress_cb(done, total, speed, eta)
        try:
            shutil.copystat(src_file, dst_file)
        except Exception:
            pass

    progress_cb(0, total, 0, 0)
    for src_file, rel, _sz in walk_contents_files(src_root):
        dst_file = dest_parent / rel
        copy_one(src_file, dst_file)
    log("コピー完了。")

# ---- GUI ----
class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("FDPlayer → SD Copier (Tkinter)")
        self.geometry("720x520")
        self.resizable(False, False)

        # state
        self.sd_list = []  # [(root, label, total, free)]
        self.worker = None

        # widgets
        frm = ttk.Frame(self, padding=12)
        frm.pack(fill="both", expand=True)

        # Source
        src_path = Path(os.getcwd()) / SRC_FOLDER_NAME
        ttk.Label(frm, text="コピー元（固定）:").grid(row=0, column=0, sticky="w")
        self.src_entry = ttk.Entry(frm, width=70)
        self.src_entry.grid(row=0, column=1, columnspan=3, sticky="we", pady=4)
        self.src_entry.insert(0, str(src_path))
        self.src_entry.configure(state="readonly")

        # SD list
        ttk.Label(frm, text="コピー先SDカード:").grid(row=1, column=0, sticky="w")
        self.sd_combo = ttk.Combobox(frm, state="readonly", width=60)
        self.sd_combo.grid(row=1, column=1, sticky="we")
        self.refresh_btn = ttk.Button(frm, text="再検出", command=self.refresh_sd_list)
        self.refresh_btn.grid(row=1, column=2, padx=6)

        # Options
        self.var_format = tk.BooleanVar(value=False)
        self.var_eject  = tk.BooleanVar(value=True)
        ttk.Checkbutton(frm, text=f"コピー直前にフォーマットする（{FORMAT_FS} / ラベル: {FORMAT_LABEL}）", variable=self.var_format).grid(row=2, column=0, columnspan=3, sticky="w", pady=4)
        ttk.Checkbutton(frm, text="コピー完了後に安全取り外しを試みる", variable=self.var_eject).grid(row=3, column=0, columnspan=3, sticky="w")

        # Progress
        ttk.Label(frm, text="進捗:").grid(row=4, column=0, sticky="w", pady=(12, 0))
        self.pb = ttk.Progressbar(frm, orient="horizontal", length=560, mode="determinate")
        self.pb.grid(row=4, column=1, columnspan=3, sticky="we", pady=(12, 0))
        self.progress_label = ttk.Label(frm, text="0%  0.0 MB / 0.0 MB  0.0 MB/s  ETA --s")
        self.progress_label.grid(row=5, column=1, columnspan=3, sticky="w")

        # Log
        ttk.Label(frm, text="ログ:").grid(row=6, column=0, sticky="nw", pady=(12, 0))
        self.log_text = tk.Text(frm, width=84, height=14)
        self.log_text.grid(row=6, column=1, columnspan=3, sticky="we", pady=(12, 0))
        self.log_text.configure(state="disabled")

        # Buttons
        self.start_btn = ttk.Button(frm, text="コピー開始", command=self.on_start)
        self.start_btn.grid(row=7, column=1, sticky="e", pady=12)
        self.quit_btn = ttk.Button(frm, text="閉じる", command=self.destroy)
        self.quit_btn.grid(row=7, column=2, sticky="w", pady=12)

        for i in range(4):
            frm.columnconfigure(i, weight=1)

        self.refresh_sd_list()

    # helpers UI
    def log(self, msg):
        self.log_text.configure(state="normal")
        self.log_text.insert("end", msg + "\n")
        self.log_text.see("end")
        self.log_text.configure(state="disabled")
        self.update_idletasks()

    def set_progress(self, done, total, speed, eta):
        def fmt_bytes(n):
            units = ["B","KB","MB","GB","TB"]
            i = 0
            n = float(n)
            while n >= 1024 and i < len(units)-1:
                n /= 1024
                i += 1
            return f"{n:.1f} {units[i]}"
        percent = int(0 if total == 0 else (done / total) * 100)
        self.pb["maximum"] = max(1, total)
        self.pb["value"] = done
        self.progress_label.config(
            text=f"{percent:3d}%  {fmt_bytes(done)} / {fmt_bytes(total)}  {fmt_bytes(speed)}/s  ETA {int(eta)}s"
        )
        self.update_idletasks()

    def refresh_sd_list(self):
        self.sd_list = []
        roots = find_sd_drives()
        items = []
        for r in roots:
            label = get_volume_label(r)
            usage = shutil.disk_usage(r)
            total_gb = usage.total / (1024**3)
            free_gb  = usage.free  / (1024**3)
            items.append(f"{r}  [{label}]  容量: {total_gb:.1f} GB / 空き: {free_gb:.1f} GB")
            self.sd_list.append((r, label, usage.total, usage.free))
        self.sd_combo["values"] = items
        if items:
            self.sd_combo.current(0)
        else:
            self.sd_combo.set("")
        self.log("SDカード一覧を更新しました。")

    # actions
    def on_start(self):
        # checks
        src = Path(os.getcwd()) / SRC_FOLDER_NAME
        if not src.is_dir():
            messagebox.showerror("エラー", f"コピー元フォルダが見つかりません:\n{src}")
            return
        if not self.sd_list or not self.sd_combo.get():
            messagebox.showwarning("警告", "SDカードが見つかりません。『再検出』を試してください。")
            return

        idx = self.sd_combo.current()
        sd_root, sd_label, sd_total, sd_free = self.sd_list[idx]

        # FAT32 4GB制限の注意（例示）
        too_big = []
        for f, _, sz in walk_contents_files(src):
            if sz >= (4 * 1024**3):
                too_big.append(str(f))
                if len(too_big) >= 3:
                    break
        if too_big:
            messagebox.showwarning("注意", "FAT32は 1ファイル4GB未満 です。")

        # 容量チェック
        need = calc_total_size(src)
        if sd_free < need:
            messagebox.showerror("エラー", f"空き容量不足です。\n必要: {need} bytes / 空き: {sd_free} bytes")
            return

        # フォーマット確認（はい／いいえ）
        do_format = self.var_format.get()
        if do_format:
            warn = "（注意）管理者権限でないとフォーマットが失敗する場合があります。\n" if not is_admin() else ""
            if not messagebox.askyesno(
                "フォーマット確認",
                f"{warn}選択したドライブ {sd_root} を {FORMAT_FS} でフォーマットし、ラベルを '{FORMAT_LABEL}' に設定します。\n"
                "この操作は元に戻せません。よろしいですか？"
            ):
                return

        # 実行
        self.start_btn.configure(state="disabled")
        self.refresh_btn.configure(state="disabled")
        self.sd_combo.configure(state="disabled")
        self.pb["value"] = 0
        self.set_progress(0, 1, 0, 0)
        self.log(f"コピー元: {src}")
        self.log(f"コピー先: {sd_root} （ラベル: {sd_label}）")
        self.log("事前フォーマット: " + ("有効（FAT32 / ラベル 'FDPLAYER'）" if do_format else "なし（既存ラベルは変更しません）"))

        def worker():
            try:
                if do_format:
                    ok = try_format(sd_root, quick=True, log=self.log)
                    if not ok:
                        self.log("フォーマットに失敗しました。処理を中止します。")
                        raise RuntimeError("format failed")

                # コピー
                self.log("コピー開始...")
                copy_with_progress(src, Path(sd_root), self.set_progress, log=self.log)
                self.log("コピー処理が完了しました。")

                # 取り外し
                if self.var_eject.get():
                    self.log("安全取り外しを試行します...")
                    ok = try_safe_remove(sd_root, log=self.log)
                    if ok:
                        self.log("安全取り外しに成功しました。")
                    else:
                        self.log("安全取り外しに失敗しました。使用中の可能性があります。")
                messagebox.showinfo("完了", "すべての処理が完了しました。")
            except Exception as e:
                messagebox.showerror("エラー", f"処理中にエラーが発生しました:\n{e}")
            finally:
                self.start_btn.configure(state="normal")
                self.refresh_btn.configure(state="normal")
                self.sd_combo.configure(state="readonly")

        threading.Thread(target=worker, daemon=True).start()

def main():
    app = App()
    app.mainloop()

if __name__ == "__main__":
    main()
