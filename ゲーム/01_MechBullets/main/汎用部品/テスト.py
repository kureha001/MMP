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
mode = 1
if mode == 1:
    繰返回数 = 300    # アドレス切替回数
    待時間   = 0.05  # ウェイト(秒)
    全権表示 = True  # True:全件表示／False:先頭末尾のみ表示

    print("--------------------")
    print(" アナログ入力テスト")
    print("--------------------")
    print("(測定データ)")
    MMP.アナログ設定(
            4,  # 使用するHC4067の個数(1～4)
            1, # 使用するHC4067のPin数(1～16)
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

#　├→（DFPlayer）
elif mode == 2:
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

    print("・フォルダーのトラックを再生")
    print(MMP.DFP_PlayFolderTrackLoop(機器番号,2,5))

#    print("・停止")
#    print(MMP.DFP_Stop(機器番号))
#│
#〇MMPを切断する。
MMP.通信切断
#┴
