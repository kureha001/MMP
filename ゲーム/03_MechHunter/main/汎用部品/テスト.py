#====================================================== 
# ＭＭＰライブラリ Ver 0.03 Rottenmeier用 サンプル
#------------------------------------------------------
# Ver 0.02.005　2025/04/28 By Takanari.Kureha
#       1.PWMエキスパンダ(PCA9685)マルチ対応
#====================================================== 

#====================================================== 
# インクルード
#====================================================== 
import mmpRottenmeier
import time


#====================================================== 
# メイン処理
#====================================================== 
#┬
#〇MMPを実体化する。
MMP = mmpRottenmeier.mmp()
#│
#〇MMPを接続する。
MMP.通信接続_自動()
#│
#◇┐MMPをテストする。
#　├→（アナログ入力（繰返））
mode = 0 # 100:ANA入力／200:DCモータ／210：電力ON-OFF／
if mode == 0:
    繰返回数 = 200    # アドレス切替回数
    待時間   = 0.05  # ウェイト(秒)
    全権表示 = True  # True:全件表示／False:先頭末尾のみ表示

    print("--------------------")
    print(" アナログ入力テスト")
    print("--------------------")
    print("(測定データ)")
    MMP.アナログ設定(
            1, # 使用するHC4067の個数(1～4)
            2, # 使用するHC4067のPin数(1～16)
            50 # アナログ値の丸め(この数値以下は切り捨て)
            )
    #◎└┐繰り返し読み取る。
    for cntLoop in range(2): MMP.PWM_VALUE( cntLoop, 4095 )
    time.sleep(3)

    for cntLoop in range(繰返回数):        
        #◎└┐全アドレスから読み取る。
        MMP.アナログ読取()
        if 待時間 > 0 : time.sleep(待時間)
        if 全権表示 : print("  %03i" % cntLoop,":", MMP.mmpAnaVal)
        else        : print("  %03i" % cntLoop,":", MMP.mmpAnaVal[0],"～", MMP.mmpAnaVal[-1])

    for cntLoop in range(2): MMP.PWM_VALUE( cntLoop, -1 )


#　├→（ＰＷＤ：電力供給）
elif mode == 1:
    for cntLoop in range(2): MMP.PWM_VALUE( cntLoop, 4095 )
    time.sleep(5)
    for cntLoop in range(2): MMP.PWM_VALUE( cntLoop, -1 )

#　├→（DFPlayer）
elif mode == 300:
    print("ｍｐ３プレイヤー")

    print("・ボリューム設定")
    print(MMP.DFP_Volume(20))

    print("・連続再生")
    for track in [1, 2, 3]:
        print(MMP.DFP_Play(track))
        time.sleep(3)

    print("・１曲目を再生")
    print(MMP.DFP_Play(1))
    time.sleep(5)

    print("・一時停止")
    print(MMP.DFP_Pause())
    time.sleep(3)

    print("・再開")
    print(MMP.DFP_Resume())
    time.sleep(5)

    print("・停止")
    print(MMP.DFP_Stop())

#　├→（DFPlayer）
elif mode == 310:
    print("ｍｐ３プレイヤー")

    機器番号 = 1 # 1 or 2

    print("・機器情報")
    print("1台目：",MMP.DFP_Info(1))
    print("2台目：",MMP.DFP_Info(2))

    print("・ボリューム設定")
    print(MMP.DFP_Volume(機器番号,20))

    print("・１曲目を再生")
    print(MMP.DFP_Play(機器番号,1))
    time.sleep(5)

    print("・停止")
    print(MMP.DFP_Stop(機器番号))

#　├→（DFPlayer）
elif mode == 320:
    print("ｍｐ３プレイヤー")

    機器番号 = 1 # 1 or 2

    print("・１曲目を再生")
#    print(MMP.DFP_Play(機器番号,1))
    print(MMP.DFP_PlayFolderTrack(機器番号,3,1))
    time.sleep(5)

    print("・停止")
    print(MMP.DFP_Stop(機器番号))
#│
#〇MMPを切断する。
MMP.通信切断
#┴
