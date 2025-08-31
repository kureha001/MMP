#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃＃０２：ＭＥＣＨ　ＳＡＵＲＵＳ
#┃※動画を『関数版：同期型』で再生（ブロッキング）
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# [システム共通]
from pathlib import Path  #☆絶対パス化に使用

# [アプリ共通]
import動画 as VLC

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
    default_volume      = 70    ,
    countdown_secs      = 3     ,
    initial_mute        = False ,
    initial_maximized   = False ,
    initial_fullscreen  = False ,
    initial_autoplay    = True  )

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ 主処理（同期実行）
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
if __name__ == "__main__":

    #☆この呼び出しはウィンドウが閉じるまで戻りません（ブロッキング）
    VLC.play_media(
        title       = VIDEO_TITLE   ,
        path        = VIDEO_PATH    ,
        background  = False         ,   #☆同期指定
        **PLAYER_KW                 )

    #☆ここに来た時点で再生は完全終了
    print("終了しました（同期）")

    #★ここから先にメインとなるの別プログラムを実行する
    import main
    main.開始()
