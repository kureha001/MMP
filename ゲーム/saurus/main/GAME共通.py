#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃共通処理
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
import  pyxel
import  json
import  main.汎用部品.mmpPeter  as Perter
from    .データセット           import データセット as DS
from    .オブジェクト.シーン    import シーンID

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃効果音
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 効果音():
	#────────────────────────────────────
    def 爆発(引数_サイズ): #① 0:被弾/1:爆破/2:大爆発
        #┬
        pyxel.play( 0, 60 + 引数_サイズ, resume = True ) 
        #┴
	#────────────────────────────────────
    def アイテム取得():
        #┬
        #○アイテム取得音を鳴らす
        pyxel.play( 0, 63, resume = True ) 
        #┴

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ＢＧＭ
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class BGM():
	#────────────────────────────────────
    def 自動選択():

        pyxel.stop()

        シーン  = DS.情報.シーン

        if シーン == シーンID.タイトル画面: BGM.JSON再生("タイトル")
        elif シーン == シーンID.プレイ画面: BGM.JSON再生("プレイ")
        elif シーン == シーンID.終了画面  : BGM.JSON再生("終了")
	#────────────────────────────────────
    def JSON再生(引数_ファイル名):

        with open(f"./オーディオ/{引数_ファイル名}.json", "rt") as fin:
            DS.情報.音楽データ = json.loads(fin.read())

        for ch, sound in enumerate(DS.情報.音楽データ):
            pyxel.sound(ch).set(*sound)
            pyxel.play(ch, ch, loop=True)
	#────────────────────────────────────
    def バリヤ():
        pyxel.stop()
        pyxel.playm(2, loop=True)

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃その他
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 発電機():
    def 右足判定(引数_角度):
        return (True) if 引数_角度 <= 180 else False

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃入力走査
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 入出力():
	#────────────────────────────────────
    MMP     = None          # MMPオブジェクト
    MMP_ON  = [             # Chごとの状態：ON(True)/OFF(False) 
            [False,False],  # ポート0用
            [False,False],  # ポート1用
            [False,False],  # ポート2用
            [False,False],  # ポート3用
            [False,False],  # ポート4用
            [False,False],  # ポート5用
            [False,False],  # ポート6用
            [False,False]   # ポート7用
            ]
	#────────────────────────────────────    
    def MMP_初期化():
        #┬
        #〇MMPを実体化する。
        入出力.MMP = Perter.mmp(
                argMmpNum       = 2,                # 使用するHC4067の個数
                argMmpAnaPins   = 10,               # 使用するHC4067のPin数
                argMmpAdrPins   = (10,11,12,13),    # RP2040-Zero
                argRundNum      = 20                # アナログ値の丸め
                )
        #│
        #〇MMPを接続する
        入出力.MMP.autoConnect()
        #┴
	#────────────────────────────────────
    def 入力走査(引数_ポートNo):
		#┬
		#○結果を初期化する
        概要 = [ False, False ]     # ONしている，OFFした直後
        測定 = [ 0,0 ]              # Chごとの ON(1)/OFF(-1) 
		#│
        ポートNo = 引数_ポートNo
        入出力.MMP.analog_IN_Each(ポートNo)
        入力値 = 入出力.MMP.mmpAnaVal[ポートNo]
		#│
		#◎└┐すべてのChを測定する
        for 各Ch in range(2):
            #│
            #◇┐移動予定の座標を求める
            if 入力値[各Ch] > 700:
                #○概要(ON)を『YES』にする
                #○状態を『ONしている』にする
                #○測定を『ON』にする
                概要[0] = True
                入出力.MMP_ON[ポートNo][各Ch] = True
                測定[各Ch] = 1

            elif 入出力.MMP_ON[ポートNo][各Ch]:
            #　├┐（全壊の状態が『ON』の場合）
                #↓
                #○概要(OFF)を『YES』にする
                #○状態を『OFF』にする
                #○測定を『OFFした直後』にする
                概要[1] = True
                入出力.MMP_ON[ポートNo][各Ch] = False
                測定[各Ch] = -1
            #　└┐（その他）
                #┴
		#│
        #▼結果を返す
        return (概要,測定)