#====================================================== 
# インクルード
#====================================================== 
import mmpClara # MMP
import time

#====================================================== 
# メイン処理
#====================================================== 
#┬
#○MMPを実体化する。
objMMP = mmpClara.MMP(
    argAnaPins      = 3,                # ボードのアナログPin数 
    argMpxPins      = 1,               # HC4067のアナログPin数
    argAdrOutPin    = [2,3,6,7]         # THC4067アドレス指定Pin
   )
# │
#○テストモードをセットする。 
mode    = 0         # 0=繰り返しテスト,1=1回のみテスト
cntCh   = 10000     # チャンネル切替回数(mode=0 でのみ有効)
#│
#◇MMPをテストする。
if mode == 0:
    #○繰り返しテスト（先頭と最終のチャンネルのみ表示）
    tmpLoop = int(cntCh / objMMP.mpxPins)
    objMMP.readTest(tmpLoop,False)

if mode == 1:
    #○1回テスト（全チャンネルを表示）
    objMMP.read()
    print(objMMP.valAnaPin)
#┴