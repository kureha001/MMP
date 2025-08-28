#======================================================
# 共通ユーティリティ.py
#======================================================
import time

#------------------------------------------------------
# 時間計測の差異を吸収
# （MicroPython: ticks_ms/diff, CPython: time.time）
#   ・時刻開始()   -> 計測開始時刻（内部表現は実装依存）
#   ・経過秒(t0)   -> 現在までの経過秒(float)
#------------------------------------------------------
try:
    _ticks_ms = time.ticks_ms            # MicroPythonのみ
    _ticks_diff = time.ticks_diff        # MicroPythonのみ

    def 時刻開始():
        return _ticks_ms()

    def 経過秒(t0):
        return _ticks_diff(_ticks_ms(), t0) / 1000.0

except AttributeError:
    # CPython など
    def 時刻開始():
        return time.time()

    def 経過秒(t0):
        return time.time() - t0


#======================================================
# アナログ入力(HC4067)
#======================================================
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
        if 待時間 > 0:
            time.sleep(待時間)

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
            if 全件表示:
                print("  %03d :" % cnt, MMP.mmpAnaVal)
            else:
                print("  %03d :" % cnt, MMP.mmpAnaVal[0], "～", MMP.mmpAnaVal[-1])

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
    MMP,                # MMPサーバーの mmp インスタンス
    再生リスト,         # [(フォルダ, トラック), ...]
    機器番号    = 1,    # 1 or 2
    音量        = 20,   # 0～30
    再生時間    = 3     # 1曲の再生時間(単位：秒)
    ):

    print(" 1.Device Status")
    print("   - 1st: ", MMP.DFP_Info(1))
    print("   - 2nd: ", MMP.DFP_Info(2))

    print(f" 2.Volume: {音量}")
    MMP.DFP_Volume(機器番号, 音量)

    print(" 3.Play MP3")
    for (f, t) in 再生リスト:
        print(f"  - Folder={f} Track={t}")
        MMP.DFP_PlayFolderTrack(機器番号, f, t)
        time.sleep(再生時間)

    print(" 4.Stop")
    MMP.DFP_Stop(機器番号)


#======================================================
# PWM出力(PCA9685)
#======================================================
#------------------------------------------------------
# チャンネルリスト変換
#------------------------------------------------------
def _to_ch_list(ch_or_list):

    if isinstance(ch_or_list, (list, tuple, set)):
        return list(ch_or_list)
    return [ch_or_list]

#------------------------------------------------------
# 電源ON/OFF
#------------------------------------------------------
def _pwm_set_value(MMP, ch_or_list, value):

    ok_all = True

    for ch in _to_ch_list(ch_or_list):
        ok = bool(MMP.PWM_VALUE(ch, value))
        if ok: continue
        print(
            "NG: PWM 電源{}  CH={:02X} 値={:04X}".format(
                "ON" if value else "OFF", ch, value
                )
            )
        ok_all = False

    return ok_all
#------------------------------------------------------
def PWM_電源_ON(MMP, ch_or_list):
    return _pwm_set_value(MMP, ch_or_list, 0x0FFF)
#------------------------------------------------------
def PWM_電源_OFF(MMP, ch_or_list):
    return _pwm_set_value(MMP, ch_or_list, 0x0000)

#------------------------------------------------------
# 指定値をそのまま出力
#------------------------------------------------------
def PWM_出力(MMP, ch_or_list, pwm値):
    chs = _to_ch_list(ch_or_list)
    ok_all = True
    for ch in chs:
        try             : ok = bool(MMP.PWM_VALUE(ch, pwm値))
        except Exception: ok = False
        if ok: continue
        try             : print("NG: CH={} PWM={:04X}".format(ch, int(pwm値) & 0xFFFF))
        except Exception: print("NG: CH={} PWM={}".format(ch, pwm値))
        ok_all = False
    return ok_all
