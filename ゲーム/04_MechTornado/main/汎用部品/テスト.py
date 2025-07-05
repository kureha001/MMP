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
繰返回数 = 20    # アドレス切替回数
待時間   = 0.1  # ウェイト(秒)
全権表示 = True  # True:全件表示／False:先頭末尾のみ表示

MMP.アナログ設定(
        2, # 使用するHC4067の個数(1～4)
        1, # 使用するHC4067のPin数(1～16)
        50 # アナログ値の丸め(この数値以下は切り捨て)
        )
time_start = time.time()

#◎└┐繰り返し読み取る。
MMP.PWM_VALUE( 0, 100 )
MMP.PWM_VALUE( 1, 4095 )
for cntLoop in range(繰返回数):        
    #◎└┐全アドレスから読み取る。
    MMP.アナログ読取()
    if 待時間 > 0 : time.sleep(待時間)
    if 全権表示 : print("  %03i" % cntLoop,":", MMP.mmpAnaVal)
    else        : print("  %03i" % cntLoop,":", MMP.mmpAnaVal[0],"～", MMP.mmpAnaVal[-1])
MMP.PWM_VALUE( 0, -1 )
MMP.PWM_VALUE( 1, -1 )

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

#│
#〇MMPを切断する。
MMP.通信切断
#┴
