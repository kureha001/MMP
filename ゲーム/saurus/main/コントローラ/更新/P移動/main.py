#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃コントローラ：更新：移動プロセス
#┠─────────────────────────────────────
#┃各オブジェクトのアクション・メソッドを実行する
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
from main.データセット        import データセット as DS
from main.オブジェクト.シーン import DB

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃メイン
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 本体:
    #┌───────────────────────────────────
    #│プロセス実行
    #└───────────────────────────────────
    def 実行(self):
        #┬
        if DS.情報.シーン == DB.シーンID.プレイ画面:

            for チームNo in range(2):

                発電機 = DS.obj.発電機[チームNo]
                運搬機 = DS.obj.運搬機[チームNo]

                発電機.FN移動.実行()

                # 運搬機をパワーダウンする
                if DS.情報.プレイ時間 % 4 == 0:
                    電力 = 運搬機.情報.入力  
                    運搬機.情報.入力 = int(電力 * 0.9)
                    運搬機.FN移動.実行()

                # 発電した電気を運搬機に供給する
                if DS.情報.プレイ時間 % 30 == 0:
                    電力 = 発電機.情報.発電容量
                    電力 = int(電力 / DS.情報.人数)
                    発電機.情報.発電容量   = 0
                    運搬機.情報.入力       += 電力
                    運搬機.FN移動.実行()

        DS.obj.シーン[ DS.情報.シーン ].FN移動.実行()
        #┴ 

#┌───────────────────────────────────
#│アクション・メソッド実行
#└───────────────────────────────────
def Pアクション(引数_対象一覧):  #① 処理の一覧 ※一覧の入れ子OK
    #┬
    #◎└┐すべてのオブジェクトを描画する
    for 各対象 in 引数_対象一覧:
        #│＼（すべての処理を終えた場合）
        #│ ↓
        #│ ▼繰り返し処理を抜ける
        #│
        #◇┐要素オブジェクトを描画する
        if isinstance(各対象, list):
        #　├┐（一覧オブジェクトの場合）
            #↓
            #◎└┐要素オブジェクトを描画する
            for 各要素 in 各対象:
                #│＼（すべての処理を終えた場合）
                #│ ↓
                #│ ▼繰り返し処理を抜ける
                #│
                #●オブジェクトの中味を確認する
                if 各対象 is None: continue
                #│＼（すべての処理を終えた場合）
                #│ ↓
                #│ ▼次の要素オブジェクトを処理する
                #│
                #●オブジェクトを移動する
                各要素.FN移動.実行()
                #┴ 

        elif 各対象 is not None:
        #　├┐（要素で中味がある場合）
            #↓
            #≫アクション・メソッドでオブジェクトを移動する
            各対象.FN移動.実行()
        #　└┐（その他）
    #┴　┴　┴
