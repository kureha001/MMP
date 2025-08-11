import  tkinter     as tk
from    tkinter     import messagebox
import  threading
import  subprocess
import  sys
from    pathlib     import Path

# === メニュー構造 {カテゴリ番号: (カテゴリ名, {プログラム番号: (フォルダ, ファイル, タイトル)})} ===
MENU = {
    "1": ("ゲーム", {
        "1": ("01_MechBullets"  , "main.py", "メック・バレット"     ),
        "2": ("02_MechSaurus"   , "main.py", "メック・サウルス"     ),
        "3": ("03_MechHunter"   , "main.py", "メック・ハンター"     ),
        "4": ("04_MechTornado"  , "main.py", "メック・トルネード"   ),
    }),
    "2": ("テスト", {
        "1": ("00_Test", "game.py", "ゲームの周辺機器"    ),
    }),
    "9": ("ツール", {
        "1": ("00_Tools", "ＳＤカード作成.py" , "FDPlayer用 SDカード作成"),
        "2": ("00_Tools", "キャッシュ削除.py" , "キャッシュ・ファイル削除"),
    }),
}

BASE_DIR = Path(__file__).resolve().parent  # ランチャーのある場所

def run_external_program(folder: str, file: str, root: tk.Tk):
    """フォルダ配下で外部Pythonプログラムを実行（非同期）"""
    target_dir = (BASE_DIR / folder).resolve()
    script_path = target_dir / file

    # 事前チェック
    if not target_dir.is_dir():
        messagebox.showerror("エラー", f"フォルダが見つかりません:\n{target_dir}")
        return
    if not script_path.is_file():
        messagebox.showerror("エラー", f"実行ファイルが見つかりません:\n{script_path}")
        return

    def worker():
        try:
            # sys.executable で現在のPythonを使用
            # cwd=target_dir により「フォルダに移動してから起動」
            proc = subprocess.Popen([sys.executable, file], cwd=target_dir)
            ret = proc.wait()
            root.after(0, lambda: messagebox.showinfo("完了", f"終了コード: {ret}"))
        except Exception as e:
            root.after(0, lambda: messagebox.showerror("エラー", f"実行に失敗しました:\n{e!r}"))

    threading.Thread(target=worker, daemon=True).start()

# === Tkinter 2階層メニュー ===
class MenuApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("ＭＭＰアプリ・ラウンチャー")
        self.root.geometry("360x300")
        self.show_main_menu()

    def clear(self):
        for w in self.root.winfo_children():
            w.destroy()

    def show_main_menu(self):
        self.clear()
        tk.Label(self.root, text="=== メインメニュー ===", font=("Arial", 14)).pack(pady=10)
        for key in sorted(MENU, key=lambda x: int(x)):
            cat_name, _ = MENU[key]
            tk.Button(
                self.root,
                text=f"{int(key):02d}. {cat_name}",
                font=("Arial", 12),
                width=26,
                command=lambda k=key: self.show_sub_menu(k)
            ).pack(pady=6)
        tk.Button(self.root, text="00. 終了", font=("Arial", 12),
                  width=26, command=self.root.quit).pack(pady=12)

    def show_sub_menu(self, cat_key: str):
        self.clear()
        cat_name, programs = MENU[cat_key]
        tk.Label(self.root, text=f"=== {cat_name} ===", font=("Arial", 14)).pack(pady=10)
        for key in sorted(programs, key=lambda x: int(x)):
            folder, file, title = programs[key]
            tk.Button(
                self.root,
                text=f"{int(key):02d}. {title}",
                font=("Arial", 12),
                width=26,
                command=lambda f=folder, p=file: run_external_program(f, p, self.root)
            ).pack(pady=6)
        tk.Button(self.root, text="00. 戻る", font=("Arial", 12),
                  width=26, command=self.show_main_menu).pack(pady=12)

def main():
    root = tk.Tk()
    MenuApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()