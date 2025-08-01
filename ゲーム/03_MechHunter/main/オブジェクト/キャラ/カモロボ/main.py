#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃キャラクタ：カモロボ
#┠─────────────────────────────────────
#┃カモロボ オブジェクト用クラス
#┃・下位にデータセット･クラス(仕様｜情報)を持つ
#┃・下位に機能クラス(移動｜衝突｜発射｜描画)をもつ
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
from   main.データセット import データセット as DS
from   .FN衝突           import 衝突クラス
from   .FN描画           import 描画クラス

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃データセット：仕様
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 仕様クラス:
    #┌───────────────────────────────────
    #│初期化
    #└───────────────────────────────────
    def __init__(self       ,
            引数_所有者     ,   #① 所有者(チームNo) 
            引数_ポート一覧 ):  #② MMPポート番号)

        #□ ペダル情報リスト
        self.ポート一覧 = 引数_ポート一覧          

        #□ カモロボが命中でリアクションする最大回数
        self.反応回数 = 2

        #□命中エフェクトのタイマ
        self.命中タイマ = 40

        #□所有者(チームNo)
        self.所有者 = 引数_所有者

        print("--------------------")
        print("[所有者] :",self.ポート一覧)
        print("[ポート] :",self.ポート一覧)

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃データセット：情報
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 情報クラス:
    #┌───────────────────────────────────
    #│初期化
    #└───────────────────────────────────
    def __init__(self):

        #□明秀回数
        self.命中回数 = 0

        #□命中したいときにリアクションした回数
        self.反応回数 = 0

        #□命中エフェクトのタイマ
        self.命中タイマ = 0

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃メイン
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 本体:
    #┌───────────────────────────────────
    #│初期化
    #└───────────────────────────────────
    def __init__(self   ,
            引数_所有者     ,   #① 所有者(チームNo) 
            引数_ポート一覧 ):  #② MMPポート番号)
        #┬
        #≫データセットを用意する
        self.仕様   = 仕様クラス(引数_所有者, 引数_ポート一覧)
        self.情報   = 情報クラス()
        #│
        #≫機能を用意する
        self.FN衝突 = 衝突クラス(self)
        self.FN描画 = 描画クラス(self)
        #│
        #○用意済みのインスタンスに生成する
        DS.obj.カモロボ.insert(0,self)
        #┴