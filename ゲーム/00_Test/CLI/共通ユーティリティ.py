#======================================================
# 共通ユーティリティ.py
#------------------------------------------------------------
#【ファイル配置方法】
# ・CPython版：
# 　同一 または 共通フォルダー(推奨)
# 　　共通フォルダには、以下の要領でパスを通します
# 　　PYTHONPATH=D:\WS\MMP\ゲーム\共通\
# ・microPython版/circuitPython版：
# 　同一 または libフォルダー(推奨)
#======================================================
import time

#------------------------------------------------------
# 時間計測の差異を吸収
# （MicroPython: ticks_ms/diff, CPython: time.time）
#   ・時刻開始()   -> 計測開始時刻（内部表現は実装依存）
#   ・経過秒(t0)   -> 現在までの経過秒(float)
#------------------------------------------------------
# MicroPython版
try:
    _ticks_ms   = time.ticks_ms    
    _ticks_diff = time.ticks_diff

    def 時刻開始(): return _ticks_ms()
    def 経過秒(t0): return _ticks_diff(_ticks_ms(), t0) / 1000.0

# CPython版/circuitPython版
except AttributeError:
    def 時刻開始(): return time.time()
    def 経過秒(t0): return time.time() - t0


#======================================================
# アナログ入力(HC4067)
#======================================================
def アナログ入力測定(
    MMP,
    スイッチ数      = 4,      # 使用するHC4067の個数(1～4)
    参加人数        = 1,      # 使用するHC4067のPin数(1～16)
    丸め            = 10,     # アナログ値の丸め(数値のバタつきを抑止)
    繰返回数        = 100,    # アドレス切替回数
    待時間          = 0.05,   # ウェイト(秒)
    全件表示        = True,   # True:全件表示／False:先頭末尾のみ表示
    表示プレイヤー  = None,   # 例：(8, 9) のようなタプル/リスト。Noneなら無指定
    表示スイッチ    = None    # 例：(0, 1)。Noneなら無指定
    ):

    # 設定
    MMP.アナログ設定(スイッチ数, 参加人数, 丸め)

    # 計測・取得
    print(" 1.Analog values")
    t0 = 時刻開始()
    for cnt in range(繰返回数):

        MMP.アナログ読取()
        if 待時間 > 0: time.sleep(待時間)

        # フィルタ指定があれば、フィルタ優先表示
        if (表示プレイヤー is not None) or (表示スイッチ is not None):
            # 選択範囲を決定
            pls = 表示プレイヤー if 表示プレイヤー is not None else range(MMP.参加総人数)
            sws = 表示スイッチ   if 表示スイッチ   is not None else range(MMP.スイッチ数)

            parts = []
            for pl in pls:
                for sw in sws:
                    try:
                        v = MMP.mmpAnaVal[pl][sw]
                        parts.append(f"{pl:02d}-{sw}:{v}")
                    except Exception:
                        parts.append(f"{pl:02d}-{sw}:ERR")
            print("  %03d :" % cnt, " ".join(parts))

        else:
            # 従来表示（全件 or 先頭末尾のみ）
            if 全件表示: print("  %03d :" % cnt, MMP.mmpAnaVal)
            else       : print("  %03d :" % cnt, MMP.mmpAnaVal[0], "～", MMP.mmpAnaVal[-1])

    # 結果表示
    sec = 経過秒(t0)
    print("\n 2.Conditions")
    print(" - Repeat Read   : %d times" % (繰返回数))
    print(" - Address Change: %d times" % (MMP.参加総人数 * 繰返回数))

    print("\n 3.Results")
    件数 = 繰返回数 * MMP.スイッチ数 * MMP.参加総人数
    平均 = (sec / 件数) if 件数 else 0.0
    print("  - Total  : %02.06f sec"   % (sec))
    print("  - Average: %01.06f sec\n" % (平均))


#======================================================
# MP3(DFPlayer)
#======================================================
def MP3_再生(
    MMP             , # MMPサーバーの mmp インスタンス
    arg再生一覧     , # [(フォルダ, トラック), ...]
    arg機器No   = 1 , # 1 or 2
    arg音量     = 20, # 0～30
    arg再生sec  = 3 , # 1曲の再生時間(単位：秒)
    ):

    print(" 1.Device Status")
    print("   - 1st: ", MMP.DFP_Info(1))
    print("   - 2nd: ", MMP.DFP_Info(2))

    print(f" 2.Volume: {arg音量}")
    MMP.DFP_Volume(arg機器No, arg音量)

    print(" 3.Play MP3")
    for (f, t) in arg再生一覧:
        print(f"  - Folder={f} Track={t}")
        MMP.DFP_PlayFolderTrack(arg機器No, f, t)
        time.sleep(arg再生sec)

    print(" 4.Stop")
    MMP.DFP_Stop(arg機器No)


#======================================================
# PWM出力(PCA9685)
#======================================================
#------------------------------------------------------
# チャンネルリスト変換
#------------------------------------------------------
def _to_ch_list(argCh一覧):
    if isinstance(argCh一覧, (list, tuple, set)): return list(argCh一覧)
    return [argCh一覧]

#------------------------------------------------------
# 電源ON/OFF
#------------------------------------------------------
def PWM_電源(MMP, argCh一覧, argスイッチ):

    ok_all = True

    for ch in _to_ch_list(argCh一覧):
        ok = bool(MMP.PWM_POWER(ch, argスイッチ))
        if ok: continue
        print(
            "NG: PWM 電源{}  CH={:02X}".format(
                "ON" if argスイッチ else "OFF",
                ch
                )
            )
        ok_all = False

    return ok_all

#------------------------------------------------------
# 指定値をそのまま出力
#------------------------------------------------------
def PWM_出力(MMP, argCh一覧, pwm値):
    Ch一覧 = _to_ch_list(argCh一覧)
    ok_all = True
    for ch in Ch一覧:
        try             : ok = bool(MMP.PWM_VALUE(ch, pwm値))
        except Exception: ok = False
        if ok: continue
        try             : print("NG: CH={} PWM={:04X}".format(ch, int(pwm値) & 0xFFFF))
        except Exception: print("NG: CH={} PWM={}".format(ch, pwm値))
        ok_all = False
    return ok_all

#------------------------------------------------------
# PWM値を徐々に移動
#------------------------------------------------------
def PWM_移動(
    MMP                 ,
    argCh一覧           ,
    arg開始値           ,
    arg終了値           ,
    arg増減     = 2     ,
    arg待ちsec  = 0.02  ,
    ):
    Ch一覧 = _to_ch_list(argCh一覧)
    ok_all = True
    try:
        ok = MMP.PWM_MOVE(
            Ch一覧      ,
            arg開始値   ,
            arg終了値   ,
            arg増減     ,
            arg待ちsec  ,
            )
    except Exception:
        ok = False
        print(f"NG: CH={ch}")
        ok_all = False
    return ok_all

#------------------------------------------------------
# PWM値を中→上→下→中に移動
#------------------------------------------------------
def PWM_移動_上中下(
    MMP                     ,
    argCh一覧               ,
    arg最小値               ,
    arg最大値               ,
    arg中央補正     = 0     , # 中央がずれる場合、この値で調整
    arg増分         = 2     ,
    arg待ちsec      = 0.02  ,
    arg一時停止sec  = 1.0   ,
    ):

    print("  1/4.PWM: Cnter")
    中央値 = int((arg最小値 + arg最大値) / 2) + arg中央補正
    PWM_出力(MMP, argCh一覧, 中央値)
    time.sleep(arg一時停止sec)

    print("  2/4.PWM: Mid to Max")
    PWM_移動(
        MMP         ,
        argCh一覧   ,
        中央値      ,
        arg最大値   ,
        arg増分     ,
        arg待ちsec  ,
        )
    time.sleep(arg一時停止sec)

    print("  3/4.PWM: Max to Min")
    PWM_移動(
        MMP         ,
        argCh一覧   ,
        arg最大値   ,
        arg最小値   ,
        arg増分     ,
        arg待ちsec  ,
        )
    time.sleep(arg一時停止sec)

    print("  4/4.PWM: Min to Mid")
    PWM_移動(
        MMP         ,
        argCh一覧   ,
        arg最小値   ,
        中央値      ,
        arg増分     ,
        arg待ちsec  ,
        )