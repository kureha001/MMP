
# [アプリ共通]
import sys; sys.path.append('..'); import 共通.動画ファイラ

VIDEOS = {
    "02-Story": {
        "title": "[メック・サウルス] 物語",
        "desc" : "恐竜を救出する物語",
        "path" : "./games/02_MechSaurus.mpg",
    },
    "02-Paly": {
        "title": "[メック・サウルス] 遊び方",
        "desc" : "遊び方の紹介",
        "path" : "./games/02_MechSaurus実録.mp4",
    },

    "03-Story": {
        "title": "[メック・ハンター] 物語",
        "desc" : "カモロボを撃退する物語",
        "path" : "./games/03_MechHunter.mp4",
    },

}

共通.動画ファイラ.open_video_menu(
    VIDEOS                                ,
    window_title        = "ゲーム解説動画",
    default_volume      = 80              ,
    countdown_secs      = 3               ,
    initial_fullscreen  = False           ,
    initial_autoplay    = True            ,
    initial_mute        = False           ,
    initial_maximized   = True            ,
    )
