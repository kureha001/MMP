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
    argMmpNum       = 4,                # 使用するHC4067の個数
    argMmpAnaPins   = 1,                # 使用するHC4067のPin数
    argMmpAdrPins   = (10,11,12,13),    # RP2040-Zero
    #argMmpAdrPins   = (2,3,4,5),        # Arduino
    argRundNum      = 20                # アナログ値の丸め
    )
#│
#〇MMPを接続する。
MMP.autoConnect()
#│
#◇┐MMPをテストする。
mode = 2
#　├→（アナログ入力（繰返））
if mode == 0:
    #〇繰り返しテスト（先頭と最終のチャンネルのみ表示）
    MMP.analog_Test(
        argLoop = 100,      # アドレス切替回数
        argWait = 0.3,      # ウェイト(秒)
        argAll  = True      # True:全件表示／False:先頭末尾のみ表示
        )


#　├→（アナログ入力（1回））
elif mode == 1:
    #〇1回テスト（全チャンネル表示）
    MMP.analog_IN_Each(0)
    print(MMP.mmpAnaVal[0])

#　├→（ＰＷＭ出力）
elif mode == 2:
    #〇チャンネル番号リスト(0～922, ････)
    pwmNo = ( 0, 31 )

    #〇動作リスト；(開始角度，終了角度，増分，待ち時間(秒))
    pwmMove = (
        (   0, 181,  30, 1.00 ),
        ( 180,  -1, -15, 0.50 ),
        (   0, 181,   1, 0.05 ),
        ( 180,  91,  -1, 0.05 )
    )

    #◎└┐動作リストに従い繰り返す。
    for move in pwmMove:
        #◎└┐動作リストの内容に従い、増分しながら繰り返す。
        for angle in range(move[0],move[1],move[2]):
            #◎└┐チャンネル番号リストに従い、繰り返す。
            for No in (pwmNo):
                #○ＰＷＭ出力する。
                MMP.digital_PWM( No, angle )
            #○時間待ちする。
            time.sleep(move[3])

#　├→（デジタル出力）
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
