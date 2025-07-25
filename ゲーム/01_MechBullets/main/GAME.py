#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ゲーム本体
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
import  pyxel
from    .データセット           import データセット as DS
import  main.GAME共通                   as 共通処理
from    .コントローラ.更新      import 更新コントローラ生成
from    .コントローラ.描画      import 描画コントローラ生成
from    .オブジェクト.シーン    import *
from    .オブジェクト.演出.背景 import 背景生成

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃メイン
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 開始:
    #┌───────────────────────────────────
    #│初期化
    #└───────────────────────────────────
    def __init__(self):
        #┬
        #○Pyxel(画面/音楽)を初期化する
        self.初期化_リソース()
        #│
        #〇処理で用いる各種オブジェクトを作成する
        self.初期化_処理セット()
        #│
        #≫シーンを『終了画面』に進行する
        DS.obj.シーン[ シーンID.終了画面 ].FN移動.Fn次シーン準備()
        #│
        #○フレーム処理に、コントローラを登録する
        pyxel.run(self.CTRL更新.実行, self.CTRL描画.実行)
        #┴

    #┌───────────────────────────────────
    #│リソースの初期化
    #└───────────────────────────────────
    def 初期化_リソース(self):
        #┬
        #○画面を初期化する
        pyxel.init(
                DS.仕様.基本.画面幅,
                DS.仕様.基本.画面高,
                title="Mega Wing  Ver.2025/06/18-00")
        pyxel.screen_mode(1)
        #│
        #○リソースファイルを読み込む
        pyxel.load("./リソース.pyxres")
        #│
        #〇MMPを初期化する
        共通処理.入出力.MMP_初期化()
        #┴

    #┌───────────────────────────────────
    #│処理セットの初期化
    #└───────────────────────────────────
    def 初期化_処理セット(self):
        #┬
        #≫シーンを生成する
        DS.obj.シーン = {
            シーンID.タイトル画面 : タイトル画面生成(),
            シーンID.プレイ画面   : プレイ画面生成()  ,
            シーンID.終了画面     : 終了画面生成()    }
        #│
        #≫更新コントローラを生成する
        self.CTRL更新 = 更新コントローラ生成()
        #│
        #≫描画コントローラを生成する
        self.CTRL描画 = 描画コントローラ生成()
        #│
        #≫背景を生成する
        DS.obj.背景 = 背景生成()
        #┴