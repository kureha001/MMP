#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃＃０２：ＭＥＣＨ　ＳＡＵＲＵＳ
#┃※動画を『関数版：非同期型』で再生
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# [システム共通]
import threading          #☆スレッド処理に使用
from pathlib import Path  #☆絶対パス化に使用

# [アプリ共通]
import 動画 as VLC

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃再生するタイトルとパス（相対パスOK／URLもOK）
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
VIDEO_TITLE = "MECH SAURUS ストーリー"
VIDEO_PATH  = "../00_PV/games/02_MechSaurus.mpg"

#☆このファイル基準で絶対パスへ変換　※URLはそのまま
if not (VIDEO_PATH.startswith("http://") or VIDEO_PATH.startswith("https://")):
    HERE = Path(__file__).resolve().parent
    VIDEO_PATH = str((HERE / VIDEO_PATH).resolve())

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ 起動オプション（必要に応じて調整）
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
PLAYER_KW = dict(
    default_volume      = 70    , # 音量
    countdown_secs      = 3     , # 再生終了後に待つ秒数
    initial_mute        = False , # 音量ミュートの要否
    initial_maximized   = False , # ウィンドウの最大化の要否
    initial_fullscreen  = False , # フルスクリーン化の要否
    initial_autoplay    = True  ) # 自動再生の要否

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ 動画再生終了で呼び出す関数
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
def 再生終了コールバック():

    #★ここにコールバックの処理を記述する
    print("終了しました")

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ ＶＬＣを非同期で実行
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
def 動画を非同期起動():

    proc = VLC.play_media(
        title       = VIDEO_TITLE   ,
        path        = VIDEO_PATH    ,
        background  = True          , #☆非同期指定
        **PLAYER_KW                 )

    #┌───────────────────────────────────
    #│ 子プロセス終了を監視してコールバックを呼ぶ監視スレッド
    #└───────────────────────────────────
    def _watch():

        try:
            proc.wait()             #☆プロセス終了を待つ（親はブロックしない）
            再生終了コールバック()  #☆動画再生終了でおこなう処理
        except Exception:
            pass

    #┌───────────────────────────────────
    #│ 非同期で実行
    #└───────────────────────────────────
    threading.Thread(target=_watch, daemon=True).start()
    return proc

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ 主処理
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
if __name__ == "__main__":

    #☆先に動画を非同期で起動（この直後から親は自由に動ける）
    proc = 動画を非同期起動()
    print("動画プレイヤーを起動しました（PID）:", proc.pid)

    #★ここから先にメインとなるの別プログラムを実行する
    import main
    main.開始()
