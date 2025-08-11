import tkinter as tk
from tkinter import messagebox
import importlib
import os
import sys

PLUGIN_DIR = "plugins"
sys.path.append(PLUGIN_DIR)

# プラグイン読み込み
def load_plugins():
    plugins = []
    for file in os.listdir(PLUGIN_DIR):
        if file.endswith(".py") and not file.startswith("__"):
            module_name = file[:-3]
            try:
                module = importlib.import_module(module_name)
                plugins.append(module)
            except Exception as e:
                print(f"プラグイン {module_name} 読込失敗: {e}")
    return plugins

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("ゲーム周辺機器のテスト")
        self.geometry("500x400")
        self.plugins = load_plugins()
        self.show_plugins()

    def clear_frame(self):
        for widget in self.winfo_children():
            widget.destroy()

    def show_plugins(self):
        self.clear_frame()
        tk.Label(self, text="ゲーム選択", font=("Arial", 16)).pack(pady=10)
        for plugin in self.plugins:
            tk.Button(self, text=plugin.PLUGIN_NAME,
                      command=lambda p=plugin: self.show_actions(p),
                      width=40).pack(pady=5)
        tk.Button(self, text="終了", command=self.destroy, width=40).pack(pady=20)

    def show_actions(self, plugin):
        self.clear_frame()
        tk.Label(self, text=f"{plugin.PLUGIN_NAME}", font=("Arial", 14)).pack(pady=10)
        for name, func in plugin.ACTIONS.items():
            tk.Button(self, text=name, command=lambda f=func: self.run_action(f), width=50).pack(pady=5)
        tk.Button(self, text="戻る", command=self.show_plugins, width=50).pack(pady=20)

    def run_action(self, func):
        try:
            func()
            messagebox.showinfo("完了", "処理が終了しました")
        except Exception as e:
            messagebox.showerror("エラー", str(e))

if __name__ == "__main__":
    app = App()
    app.mainloop()
