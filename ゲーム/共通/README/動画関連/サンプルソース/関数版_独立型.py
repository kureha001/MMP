#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃＃０２：ＭＥＣＨ　ＳＡＵＲＵＳ
#┃※動画を『関数版：独立型（完全分離）』で再生
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# [システム共通]
import os, sys, subprocess
from pathlib import Path  #☆絶対パス化に使用

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ ここでは直接 子モジュール(-m 動画_子プロセス) を起動します
#┃ ※ 共通/ があるフォルダを PYTHONPATH に追加できるよう cwd/env を整えます
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
HERE = Path(__file__).resolve().parent
TOP  = HERE.parent             # 共通/ の1つ上
PKG  = "共通"                  # パッケージ名（共通/）

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃再生するタイトルとパス（相対パスOK／URLもOK）
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
VIDEO_TITLE = "MECH SAURUS ストーリー"
VIDEO_PATH  = "../00_PV/games/02_MechSaurus.mpg"

#☆このファイル基準で絶対パスへ変換　※URLはそのまま
if not (VIDEO_PATH.startswith("http://") or VIDEO_PATH.startswith("https://")):
    VIDEO_PATH = str((HERE / VIDEO_PATH).resolve())

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ 起動オプション（必要に応じて調整）
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
PLAYER_KW = dict(
    default_volume      = 70    ,
    countdown_secs      = 3     ,
    initial_mute        = False ,
    initial_maximized   = False ,
    initial_fullscreen  = False ,
    initial_autoplay    = True  )

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ ＶＬＣを“独立”で実行（デタッチ）
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
def 動画を独立起動():

    module_name = f"{PKG}.動画_子プロセス"
    args = [
        "--title"       , VIDEO_TITLE   ,
        "--path"        , VIDEO_PATH    ,
        "--volume"      , str(int(PLAYER_KW["default_volume"    ])),
        "--countdown"   , str(int(PLAYER_KW["countdown_secs"    ])),
        "--mute"        , "1" if  PLAYER_KW["initial_mute"      ] else "0",
        "--maximized"   , "1" if  PLAYER_KW["initial_maximized" ] else "0",
        "--fullscreen"  , "1" if  PLAYER_KW["initial_fullscreen"] else "0",
        "--autoplay"    , "1" if  PLAYER_KW["initial_autoplay"  ] else "0"]

    #☆実行バイナリ　※Windows は pythonw.exe 優先
    exe = sys.executable
    if os.name == "nt":
        pyw = exe.replace("python.exe", "pythonw.exe")
        if os.path.exists(pyw):
            exe = pyw

    cmd = [exe, "-m", module_name, *args]

    #☆子からパッケージが見えるように PYTHONPATH / cwd を設定
    env = os.environ.copy()
    env["PYTHONPATH"] = str(TOP) + os.pathsep + env.get("PYTHONPATH", "")

    #☆デタッチ起動
    if os.name == "nt":
        CREATE_NEW_PROCESS_GROUP = 0x00000200
        DETACHED_PROCESS = 0x00000008
        return subprocess.Popen(
            cmd                                 ,
            cwd             = str(TOP)          ,
            env             = env               ,
            stdin           = subprocess.DEVNULL,
            stdout          = subprocess.DEVNULL,
            stderr          = subprocess.DEVNULL,
            close_fds       = True              ,
            creationflags   = CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS)
    else:
        # POSIX: 新しいセッション（親シグナルの影響を受けない）
        return subprocess.Popen(
            cmd                                 ,
            cwd             = str(TOP)          ,
            env             = env               ,
            stdin           = subprocess.DEVNULL,
            stdout          = subprocess.DEVNULL,
            stderr          = subprocess.DEVNULL,
            close_fds       = True              ,
            preexec_fn      = os.setsid         )  # type: ignore[attr-defined]

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ 主処理（独立実行）
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
if __name__ == "__main__":

    #☆独立起動する
    proc = 動画を独立起動()
    print("動画プレイヤーを“独立”で起動しました（PID）:", proc.pid)

    #★ここから先にメインとなるの別プログラムを実行する（親が終了しても子は継続）
    import main
    main.開始()
