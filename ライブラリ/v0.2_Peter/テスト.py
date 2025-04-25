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
    argMmpNum       = 4,                # 使用するHC4067の個数
    argMmpAnaPins   = 1,                # 使用するHC4067のPin数
    #argMmpAdrPins   = (10,11,12,13),    # RP2040-Zero
    argMmpAdrPins   = (2,3,4,5),        # Arduino
    argRundNum      = 20                # アナログ値の丸め
    )
#│
#〇MMPを接続する。
MMP.autoConnect()
#│
#◇MMPをテストする。
mode = 2
 #→アナログ入力（繰返）
if mode == 0:
    #〇繰り返しテスト（先頭と最終のチャンネルのみ表示）
    MMP.analog_Test(
        argLoop = 100,      # アドレス切替回数
        argWait = 0.3,      # ウェイト(秒)
        argAll  = True      # True:全件表示／False:先頭末尾のみ表示
        )

 #→アナログ入力（1回）
elif mode == 1:
    #〇1回テスト（全チャンネル表示）
    MMP.analog_IN_Each(0)
    print(MMP.mmpAnaVal[0])

elif mode == 2:
    #〇PWMエキスパンダのテスト（0,15チャンネル）
    for i in range(0,230):
        MMP.digital_PWX( 0,i)
        MMP.digital_PWX(15,i)
        time.sleep(0.02)

#→デジタル出力
elif mode == -1:
    light = True
    for j in range(10):
        light = not light
        if( light ):
            MMP.portOut_bit(2, 1)
            print("ON")
        else:
            MMP.portOut_bit(2, 0)
            print("OFF")
        time.sleep(5)
#│
#〇MMPを切断する。
MMP.disconnect
#┴
