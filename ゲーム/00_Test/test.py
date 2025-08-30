#======================================================
# ゲームの周辺機器テスト
#======================================================
import tkinter as tk
from   tkinter import ttk, messagebox, scrolledtext
import importlib, pkgutil, threading
import sys; sys.path.append('..'); import mmpC

#───────────────────────────    
# ＭＭＰ接続
#───────────────────────────    
class 通信コネクション:

    # ----------------- 初期化 -----------------
    def __init__(self):
        self.mmp = 共通.mmpC.mmp()

    # ----------------- MMP接続 -----------------
    def 通信接続(self):
        self.mmp.通信接続()

    # ----------------- MMP切断 -----------------
    def 通信切断(self):
        self.mmp.通信切断()

#───────────────────────────    
# プラグイン実行スレッド
#───────────────────────────    
class PluginRunner(threading.Thread):

    # ----------------- 初期化 -----------------
    def __init__(self, action_cls, params, svc, emit_callback):
        super().__init__(daemon=True)
        self.action_cls     = action_cls
        self.params         = params
        self.svc            = svc
        self.emit_callback  = emit_callback
        self.stop_event     = threading.Event()

    # ----------------- 開始 -----------------
    def run(self):

        # emit を第4引数で渡す仕様
        try:
            action = self.action_cls()
            action.run(self.svc, self.params, self.stop_event, self.emit_callback)

        except Exception as e:
            self.emit_callback(f"[エラー] {e}")

    # ----------------- 停止 -----------------
    def stop(self):
        self.stop_event.set()

#───────────────────────────    
# アプリ本体
#───────────────────────────    
class MainApp(tk.Tk):

    # ----------------- 初期化 -----------------
    def __init__(self):

        super().__init__()
        self.title("ゲームの周辺機器テスト")
        self.geometry("900x600")

        # --- 全体フレーム ---
        main_frame = ttk.Frame(self)
        main_frame.pack(fill=tk.BOTH, expand=True)

        # 左メニュー（幅固定で2倍=300px）
        self.frame_left = ttk.Frame(main_frame, width=300)
        self.frame_left.pack(side=tk.LEFT, fill=tk.Y)
        self.frame_left.pack_propagate(False)

        self.plugin_list = tk.Listbox(self.frame_left, exportselection=False)
        self.plugin_list.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.plugin_list.bind("<<ListboxSelect>>", self.on_plugin_select)

        # 右側（アクション+コントロール+ログ）
        self.frame_right = ttk.Frame(main_frame)
        self.frame_right.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        # 上部：アクション一覧
        self.action_list = tk.Listbox(self.frame_right, exportselection=False)
        self.action_list.pack(fill=tk.X, padx=5, pady=(5, 0))

        # 実行/終了ボタン行
        controls = ttk.Frame(self.frame_right)
        controls.pack(fill="x", padx=5, pady=5)
        self.btn_run = ttk.Button(controls, text="実行", command=self.on_run)
        self.btn_run.pack(side="left")
        self.btn_exit = ttk.Button(controls, text="終了", command=self.on_exit)
        self.btn_exit.pack(side="right")

        # 下部：ログ
        self.log_area = scrolledtext.ScrolledText(self.frame_right, wrap=tk.WORD, height=25)
        self.log_area.pack(fill=tk.BOTH, expand=True, padx=5, pady=(0, 5))

        # 状態
        self.plugins = {}
        self.current_plugin = None
        self.runner: PluginRunner | None = None
        self.is_running = False  # ★同時実行禁止フラグ

        # プラグイン読込
        self.load_plugins()

        # ウィンドウ× クローズでも on_exit と同一処理
        self.protocol("WM_DELETE_WINDOW", self.on_exit)

    # ----------------- プラグイン読込 -----------------
    def load_plugins(self):

        import plugins

        for _, name, _ in pkgutil.iter_modules(plugins.__path__):
            module = importlib.import_module(f"plugins.{name}")
            self.plugins[module.PLUGIN_NAME] = module
            self.plugin_list.insert(tk.END, module.PLUGIN_NAME)

    # ----------------- プラグイン選択 -----------------
    def on_plugin_select(self, event):

        if self.is_running: return  # 実行中は切替不可

        sel = self.plugin_list.curselection()
        if not sel: return

        plugin_name         = self.plugin_list.get(sel[0])
        self.current_plugin = self.plugins[plugin_name]

        # アクション一覧を更新
        self.action_list.delete(0, tk.END)
        for action_cls in self.current_plugin.ACTIONS:
            self.action_list.insert(tk.END, action_cls.name)

    # ----------------- 実行開始/禁止制御 -----------------
    def on_run(self):

        # 同時実行チェック
        if self.is_running and self.runner and self.runner.is_alive():
            messagebox.showinfo(
                "実行中",
                "すでにアクションが実行中です。\n複数同時実行はできません。"
                )
            return

        # 未選択(プラグイン)チェック
        if not self.current_plugin:
            messagebox.showwarning(
                "警告",
                "プラグインを選択してください。"
                )
            return

        # 未選択(アクション)チェック
        sel = self.action_list.curselection()
        if not sel:
            messagebox.showwarning(
                "警告",
                "アクションを選択してください。"
                )
            return

        action_cls  = self.current_plugin.ACTIONS[sel[0]]
        params      = action_cls.Params()  # デフォルト値で実行

        # UIロック
        self._lock_ui_for_running(True)

        # ログ初期化
        self.log_area.delete("1.0", tk.END)
        self._emit_log(f"[開始] {self.current_plugin.PLUGIN_NAME} / {action_cls.name}")
        self._emit_log("------------------")

        # 起動時に強制接続
        self.svc = 通信コネクション()
        try                     : self.svc.通信接続()
        except Exception as e   :messagebox.showerror("接続エラー", str(e))

        # 実行開始
        self.runner = PluginRunner(action_cls, params, self.svc, self._emit_log)
        self.is_running = True
        self.runner.start()

        # 終了監視
        self.after(100, self._poll_runner_done)

    # ----------------- 実行完了 -----------------
    def _poll_runner_done(self):

        if self.runner and self.runner.is_alive():
            self.after(100, self._poll_runner_done)
            return

        # ここに来たら終了
        self._emit_log("------------------")
        self._emit_log("[完了]")

        # MMP 切断
        try             : self.svc.通信切断()
        except Exception: pass

        self.is_running = False
        self._lock_ui_for_running(False)

    # ----------------- 重複起動禁止 -----------------
    def _lock_ui_for_running(self, running: bool):
        """実行中は再実行やメニュー操作を禁止（終了ボタンも無効化）"""
        if running:
            self.btn_run.state(["disabled"])
            self.btn_exit.state(["disabled"])
            self.plugin_list.configure(state="disabled")  # tk.Listboxは configure で状態変更
            self.action_list.configure(state="disabled")
        else:
            self.btn_run.state(["!disabled"])
            self.btn_exit.state(["!disabled"])
            self.plugin_list.configure(state="normal")
            self.action_list.configure(state="normal")

    # ----------------- ログ出力 -----------------
    def _emit_log(self, text: str):
        # スレッドセーフに追記
        def append():
            self.log_area.insert(tk.END, (text if text.endswith("\n") else text + "\n"))
            self.log_area.see(tk.END)
        self.after(0, append)

    # ----------------- 終了処理 -----------------
    def on_exit(self):

        # 実行中なら停止要求（少しだけ待機）
        try:
            if self.runner and self.runner.is_alive():
                self.runner.stop()
                self.runner.join(timeout=1.5)
        except Exception: pass

        # アプリ終了
        self.destroy()

#======================================================
if __name__ == "__main__": MainApp().mainloop()
