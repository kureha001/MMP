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
#┃入力走査
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 操作手段():
	#────────────────────────────────────
    MMP_ON  = [
            [False,False,False,False],
            [False,False,False,False],
            [False,False,False,False]
            ]
    MMP_BTN = ( (20,100) , (450,500) , (260,300) , (50,100) )

	#────────────────────────────────────
    def 入力走査(引数_種類,引数_番号):

        操作手段.MMP.analog_IN_Each(引数_番号)

		#○結果を初期化する
        概要 = [ False, False ]
        測定 = [ 0,0,0,0 ]
        入力値 = 操作手段.MMP.mmpAnaVal[引数_番号]
		#│
        #〇縦移動を測定する
        測定[0] = (1) if 入力値[0] < 700 else (0)
        測定[1] = (1) if 入力値[0] > 700 else (0)
		#│
		#◎└┐概要を求める
        for i in  測定:
            if i != 0:
                概要 = [ True, True ]
                break
            #┴
		#│
        #▼結果を返す
        return (概要,測定)
	#────────────────────────────────────    
    def MMP_初期化():
        #┬
        #〇MMPを実体化する。
        操作手段.MMP = Perter.mmp(
                argMmpNum       = 4,                # 使用するHC4067の個数
                argMmpAnaPins   = DS.情報.人数,     # 使用するHC4067のPin数
                argMmpAdrPins   = (10,11,12,13),    # RP2040-Zero
                argRundNum      = 20                # アナログ値の丸め
                )
        #│
        #〇MMPを接続する
        操作手段.MMP.autoConnect()
        #┴