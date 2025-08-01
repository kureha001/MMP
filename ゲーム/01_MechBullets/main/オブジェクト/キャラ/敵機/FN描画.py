#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃キャラクタ：敵機(描画プロセス)
#┠─────────────────────────────────────
#┃描画コントローラが描画プロセスで実行するアクション・メソッド
#┃・下位にデータセット･クラス(仕様)を持つ
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
import  pyxel
from    main.データセット import データセット as DS

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃仕様
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 仕様クラス():
    def __init__(self   ,
            引数_仕様   ):  #① 個体の仕様
        #┬
        #◇┐1辺の長さと切り出し位置(Y軸)を用意する
        if 引数_仕様.アイテム区分:
        #　├┐（アイテムの場合）
            #↓
            #○アイテム用の内容を用意する
            y           = DS.仕様.画像Y.アイテム
            キャラ幅    = DS.仕様.キャラ幅.通常
            #┴
        elif 引数_仕様.ボス区分:
        #　├┐（ボスキャラの場合）
            #↓
            #○ボスキャラ用の内容を用意する
            y           = DS.仕様.画像Y.ボス
            キャラ幅    = DS.仕様.キャラ幅.ボス
            #┴
        else:
        #　└┐（その他）
            #↓
            #○敵機用の内容を用意する
            y           = DS.仕様.画像Y.敵機
            キャラ幅    = DS.仕様.キャラ幅.通常
            #┴
        #│
        #○用意した内容で画像の切り出し仕様を初期化する
        x = キャラ幅 * 引数_仕様.種類ID
        座標 = (x,y)
        サイズ = (キャラ幅,キャラ幅)
        self.画像 = (0, *座標, *サイズ, 0)
        #┴

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃メイン
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 描画クラス:
    #┌───────────────────────────────────
    #│初期化
    #└───────────────────────────────────
    def __init__(self   ,
            引数_個体   ):  #① 個体オブジェクト
        #┬
        #〇個体オブジェクトのリファレンスを用意する
        self._仕様  = 引数_個体.仕様
        self._情報  = 引数_個体.情報
        #│
        #≫データセットを用意する
        self.仕様   = 仕様クラス(self._仕様)
        #┴

    #┌───────────────────────────────────
    #│機能実行
    #└───────────────────────────────────
    def 実行(self):
        #┬
        #○画像仕様に基づき描画する
        pyxel.blt(self._情報.X, self._情報.Y, *self.仕様.画像 )
        #┴