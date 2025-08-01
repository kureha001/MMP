#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃キャラクター：運搬機
#┠─────────────────────────────────────
#┃運搬機オブジェクト用クラス
#┃・下位にデータセット･クラス(仕様｜情報)を持つ
#┃・下位に機能クラス(移動｜衝突｜発射｜描画)をもつ
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
from main.データセット import データセット as DS
from .FN移動           import 移動クラス
from .FN救出           import 救出クラス

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃データセット：仕様
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 仕様クラス:

    #□電力情報
    最高出力        = 4095  # 上限値(単位:PWM値)
    最低出力        = 1300  # 下限値(単位:PWM値) ※この値以下で停止させる
    変換効率        = 65    # 入力電力が出力電力に変換される効率(単位:％) 

    #┌───────────────────────────────────
    #│初期化
    #└───────────────────────────────────
    def __init__(self       ,
            引数_ポートNo   ,   #① MMPポート番号
            引数_送電No     ):  #② (動力[PWMポート番号] , 電飾[PWポート番号])

        #□ ポート情報リスト
        self.ポートNo   = 引数_ポートNo  # MMPポート番号
        self.動力No     = 引数_送電No[0] # モータのPWMポート番号
        self.電飾No     = 引数_送電No[1] # 電飾のPWMポート番号

        print("--------------------")
        print("[運搬機]")
        print(" MMP  :",self.ポートNo)
        print(" MOTOR:",self.動力No)
        print(" LED  :",self.電飾No)


#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃データセット：情報
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 情報クラス:
    #┌───────────────────────────────────
    #│初期化
    #└───────────────────────────────────
    def __init__(self):

        #□ 救出した恐竜の数
        self.救出数   = 0

        #□ ポート情報リスト
        self.入力電力 = 0 # 発電機から送電された電力
        self.出力電力 = 0 # 動力に供給する電力（画面表示用）

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃メイン
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 本体:
    #┌───────────────────────────────────
    #│初期化
    #└───────────────────────────────────
    def __init__(self       ,
            引数_ポートNo   ,   #① MMPポート番号
            引数_送電No     ):  #② (動力[PWMポート番号] , 電飾[PWポート番号])
        #┬
        #≫データセットを用意する
        self.仕様 = 仕様クラス(引数_ポートNo,引数_送電No)
        self.情報 = 情報クラス()
        #│
        #≫機能を用意する
        self.FN移動 = 移動クラス(self)
        self.FN救出 = 救出クラス(self)
        #│
        #≫用意済みのインスタンスに生成する
        DS.obj.運搬機.insert(0,self)
        #┴