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
MMP.アナログ設定(
        3, # 使用するHC4067の個数(1～4)
        1, # 使用するHC4067のPin数(1～16)
        50 # アナログ値の丸め(この数値以下は切り捨て)
        )
time_start = time.time()

#----- 人形の脚 -----
MMP.PWM_VALUE( 1, 4095 )
time.sleep(2)
MMP.PWM_VALUE( 1, -1 )

#----- 鉄棒のロック -----
MMP.PWM_VALUE( 0, 100)
time.sleep(1)
for i in range(100,650,10):
    print(i)
    time.sleep(0.05)
    MMP.PWM_VALUE( 0, i )
MMP.PWM_VALUE( 0, 100)

#----- フットスイッチ -----
for cntLoop in range(100):        
    #◎└┐全アドレスから読み取る。
    MMP.アナログ読取()
    time.sleep(0.1)
    print("  %03i" % cntLoop,":", MMP.mmpAnaVal)
#│
#〇MMPを切断する。
MMP.通信切断
#┴
