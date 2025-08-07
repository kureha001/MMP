#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃汎用部品：入力
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
from .                 import MMP
from main.データセット import データセット as DS

#────────────────────────────────────
class 情報:
    状態  = [     # Chごとの状態：ON(True)/OFF(False) 
        [False,False],  # ポート0用 [Ch0,Ch1]
        [False,False],  # ポート1用 [Ch0,Ch1]
        [False,False],  # ポート2用 [Ch0,Ch1]
        [False,False],  # ポート3用 [Ch0,Ch1]
        [False,False],  # ポート4用 [Ch0,Ch1]
        [False,False],  # ポート5用 [Ch0,Ch1]
        [False,False],  # ポート6用 [Ch0,Ch1]
        [False,False]]  # ポート7用 [Ch0,Ch1]

#────────────────────────────────────
def 入力走査(引数_ポートNo):
    #┬
    #○結果を初期化する
    概要 = [ False, False ]     # ONしている，OFFした直後
    測定 = [ 0,0 ]              # Chごとの測定値(1:ON/-1:OFFになった/0:通常のOFF) 
    #│
    #○ポートのアナログ値を用意する
    ポートNo = 引数_ポートNo
    入力値 = MMP.接続.mmpAnaVal[ポートNo]
    #│
    #◎└┐ON/OFF状態を求める
    for 各Ch in range(2):
        #│
        #◇┐入力状況を走査する
        if 入力値[各Ch] > DS.仕様.ハード設定.スイッチ閾値:
        #　├┐（状態が『ON』の場合）
            #↓
            #○状態を『ONしている』にする
            #○測定を『ON』にする
            #○概要(ON)を『YES』にする
            情報.状態[ポートNo][各Ch] = True
            測定[各Ch] = 1
            概要[0] = True
            #┴
        elif 情報.状態[ポートNo][各Ch]:
        #　├┐（前回の状態が『ON』の場合）
            #↓
            #○状態を『OFF』にする
            #○測定を『OFFした直後』にする
            #○概要(OFF)を『YES』にする
            情報.状態[ポートNo][各Ch] = False
            測定[各Ch] = -1
            概要[1] = True
            #┴
        #　└┐（その他）
            #┴
    #│
    #▼結果を返す
    return (概要,測定)