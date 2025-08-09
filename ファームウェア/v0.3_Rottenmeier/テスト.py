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
#○MMPを実体化する。
MMP = mmpRottenmeier.mmp()
#│
#○MMPを接続する。
MMP.通信接続_自動()
#│
#◇┐MMPをテストする。
#　├→（アナログ入力（繰返））
mode = 310
if mode == 100:
    繰返回数 = 1000    # アドレス切替回数
    待時間   = 0.05  # ウェイト(秒)
    全権表示 = True  # True:全件表示／False:先頭末尾のみ表示

    print("--------------------")
    print(" アナログ入力テスト")
    print("--------------------")
    print("(測定データ)")
    MMP.アナログ設定(
            2,  # 使用するHC4067の個数(1～4)
            2, # 使用するHC4067のPin数(1～16)
            100 # アナログ値の丸め(この数値以下は切り捨て)
            )
    time_start = time.time()

    #◎└┐繰り返し読み取る。
    for cntLoop in range(繰返回数):        
        #◎└┐全アドレスから読み取る。
        MMP.アナログ読取()
        if 待時間 > 0 : time.sleep(待時間)
        if 全権表示 : print("  %03i" % cntLoop,":", MMP.mmpAnaVal)
        else        : print("  %03i" % cntLoop,":", MMP.mmpAnaVal[0],"～", MMP.mmpAnaVal[-1])

    #◇結果を表示する。
    time_end = time.time()
    time_diff = time_end - time_start

    print("\n(実施条件)")
    print("・繰返回数         : %i" % (繰返回数))
    print("・アドレス変更回数 : %i" % (MMP.参加総人数 * 繰返回数))

    print("\n(測定結果)")
    cntTtl = 繰返回数 * MMP.スイッチ数 * MMP.参加総人数
    print("・合計時間   : %02.06f秒"       % (time_diff             ))
    print("・データ平均 : %01.06f秒\n"   % (time_diff/cntTtl        ))

#　├→（ＰＷＤ：ＤＣモータ）
elif mode ==200:
    print("ＰＷＤ：ＤＣモータ")
    番号    = 0     #恐竜ランドのDCモータ
    最小    = 1800
    最大    = 3800  #4095がデューティー比100%で最大になる
    間隔S   = 30
    間隔E   = -50
    停止    = 0.2

    for val in range(最小,最大,間隔S):
        print("PWM:",val)
        MMP.PWM_VALUE( 番号, val )
        time.sleep(停止)

    time.sleep(2)

    for val in range(最大,最小,間隔E):
        print("PWM:",val)
        MMP.PWM_VALUE( 番号, val )
        time.sleep(停止)
    MMP.PWM_VALUE( 番号, -1 )

#　├→（ＰＷＤ：電力供給）
elif mode == 210:
    print("ＰＷＤ：電力供給")
    番号    = 1     #恐竜ランドの音声モジュール＆LED
    最小    = 4095  #4095:デューティー比100%
    停止    = 20
    for i in range(1):
        MMP.PWM_VALUE( 番号, 最小 )
        time.sleep(停止)
        MMP.PWM_VALUE( 番号, 0 )

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
#│
#○MMPを切断する。
MMP.通信切断
#┴
