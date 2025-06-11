#====================================================== 
# ＭＭＰライブラリ Ver 0.02 ペーター用 サンプル
#------------------------------------------------------
# Ver 0.02.005　2025/04/28 By Takanari.Kureha
#       1.PWMエキスパンダ(PCA9685)マルチ対応
#====================================================== 

#====================================================== 
# インクルード
#====================================================== 
import mmpPeter
import time


#====================================================== 
# メイン処理
#====================================================== 
#┬
#〇MMPを実体化する。
MMP = mmpPeter.mmp(
    argMmpNum       = 2,                # 使用するHC4067の個数
    argMmpAnaPins   = ,                # 使用するHC4067のPin数
    argMmpAdrPins   = (10,11,12,13),    # RP2040-Zero
    #argMmpAdrPins   = (2,3,4,5),        # Arduino
    argRundNum      = 10                # アナログ値の丸め
    )
#│
#〇MMPを接続する。
MMP.autoConnect()
#│
#◇┐MMPをテストする。
#　├→（アナログ入力（繰返））
mode = 0
if mode == 0:
    #〇繰り返しテスト（先頭と最終のチャンネルのみ表示）
    MMP.analog_Test(
        argLoop = 400,      # アドレス切替回数
        argWait = 0.05,     # ウェイト(秒)
        argAll  = True      # True:全件表示／False:先頭末尾のみ表示
        )


#　├→（アナログ入力（1回））
elif mode == 1:
    print("アナログ入力")
    for i in range(0,1):
        #〇1回テスト（全チャンネル表示）
        MMP.analog_IN_Each(i)
        値 = ""

        for j in range(4):
            区切 = "" if j==0 else " , "
            値 = f"{値}{区切}{str(MMP.mmpAnaVal[i][j]).zfill(4)}"

        print(f"{str(i).zfill(2)}ch：{値}")
        time.sleep(0.1)

#　├→（ＰＷＤ：ＤＣモータ）
elif mode ==3:
    print("ＰＷＤ：ＤＣモータ")
    番号    = 0
    最小    = 1800
    最大    = 3800 #4095:デューティー比100%
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
elif mode == 4:
    print("ＰＷＤ：電力供給")
    番号    = 1
    最小    = 4095 #4095:デューティー比100%
    停止    = 20
    for i in range(1):
        MMP.PWM_VALUE( 番号, 最小 )
        time.sleep(停止)
        MMP.PWM_VALUE( 番号, 0 )
#│
#〇MMPを切断する。
MMP.disconnect
#┴
