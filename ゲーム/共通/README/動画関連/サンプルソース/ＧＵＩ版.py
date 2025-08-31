#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ＰＶ 動画ファイル プレイヤー
#┃※動画を『ＧＵＩ版』で再生（ＵＩ付き）
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# [アプリ共通]
import 動画メニュー実装 as VLC

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ファイル一覧
#┃・ファイルパス：ＶＬＣで再生する
#┃・ＵＲＬ　　　：既定ブラウザで開く
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
動画一覧 = {
    "02-Story"  : {
        "title" : "[メック・サウルス] 物語",
        "desc"  : "恐竜を救出する物語",
        "path"  : "./games/02_MechSaurus.mpg",
        },
    "02-Paly"   : {
        "title" : "[メック・サウルス] 遊び方",
        "desc"  : "遊び方の紹介",
        "path"  : "./games/02_MechSaurus実録.mp4",
        },

    "03-Story"  : {
        "title" : "[メック・ハンター] 物語",
        "desc"  : "カモロボを撃退する物語",
        "path"  : "./games/03_MechHunter.mp4",
        },

    "99-URL"    : {
        "title" : "サンプル・サイト",
        "desc"  : "ブラウザで開きます",
        "path"  : "https://youtu.be/Mr_KgUFz_28"}
    }

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃メイン処理
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
if __name__ == "__main__":

    VLC.open_video_menu(
        動画一覧                        ,
        window_title="ゲームの解説動画" ,
        default_volume      = 70        ,
        countdown_secs      = 1         ,
        initial_mute        = False     ,
        initial_maximized   = False     ,
        initial_fullscreen  = False     ,
        initial_autoplay    = True      )