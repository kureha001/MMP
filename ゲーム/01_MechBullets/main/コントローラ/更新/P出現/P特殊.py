#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃コントローラ：更新：出現プロセス：アクション（アイテム：特殊効果）
#┠─────────────────────────────────────
#┃アイテム・オブジェクトのアクション・メソッドで特殊効果を出現させる
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
import  pyxel
import  random
from    main.データセット               import データセット as DS
from    main.オブジェクト.管理.特殊効果 import アイテムDB
from    main.オブジェクト.キャラ.敵機   import 敵機生成

#┌───────────────────────────────────
#│アクション・メソッド実行
#└───────────────────────────────────
def Pアクション_特殊(引数_間隔):
    #┬
    #〇出現タイミングを確認する
    タイミング判定  = (DS.情報.プレイ時間 % 引数_間隔 == 0)
    if not タイミング判定: return
    #│＼（出現時間が未到来の場合）
    #│ ↓
    #│ ▼処理を中断する
    #│
    #●アイテムを抽選する
    アイテムID = act特殊_選定()
    if アイテムID is None: return
    #│
    #≫アクション・メソッドでアイテム(特殊効果)を生成する
    画面幅   = DS.仕様.基本.画面幅
    キャラ幅 = DS.仕様.キャラ幅.通常
    X座標    = pyxel.rndi(0, 画面幅 - キャラ幅)
    敵機生成(X座標, -DS.仕様.キャラ幅.ボス, アイテムID, True)
    #┴

#────────────────────────────────────
def act特殊_選定():
    #┬
    #〇前処理
    発動中      = DS.obj.特殊効果.情報.発動中
    永続ID      = -1
    抽選候補    = []
    重みリスト  = []
    #│
    #◎└┐
    for アイテムID, DBレコード in アイテムDB.items():
        #│
        #〇アイテムDBから基本情報を用意する
        DB効果ID    = DBレコード[0]
        DB出現条件  = DBレコード[1][0]
        DB出現率    = DBレコード[1][1]
        DB持続性    = DBレコード[2][0]
        DBデータ    = DBレコード[2][1]
        #│
        #〇属性を確認する
        if DB出現条件 > DS.情報.難易度 or DB持続性 == 永続ID: continue
        #│＼（難易度制限が不一致 or 使い切り の場合）
        #│ ↓
        #│ ▼処理を中断する
        #│
        #◇┐上限に達したアイテムを除外する
        if DB持続性 == 0 and DB効果ID in 発動中:
        #　├┐（効果が永続する場合）
            #↓
            #〇アイテムDBの上限設定を確認する
            if DBデータ is None: continue
            #│＼（空の場合）
            #│ ↓
            #│ ▼次をサーチする
            #│
            #〇アイテムDBの上限設定を確認する
            if not isinstance(DBデータ, tuple): continue
            #│＼（上限設定が無い場合）
            #│ ↓
            #│ ▼次をサーチする
            #│
            #〇発動状況から、現在の値を用意する
            #〇アイテムDBから上限値を用意する
            登録済データ = 発動中[DB効果ID]
            登録値, 上限値 = DBデータ
            #│
            #〇属性を確認する
            chk上限付   = isinstance(登録済データ[1], (int, float))
            chk上限到達 = 登録済データ[1] >= 上限値
            if chk上限付 and chk上限到達: continue 
            #│＼（上限に足している場合）
            #│ ↓
            #│ ▼次をサーチする
            #┴
        #│
        #〇発動中の効果は出現率を半減（最低1）する
        if DB効果ID in 発動中: DB出現率 = max(1, DB出現率 // 2)
        #│
        #〇抽選箱にくじを入れる
        #〇このくじの当選確率をセットする
        抽選候補.append(アイテムID)
        重みリスト.append(DB出現率)
        #┴
    #│
    #〇1つだけ抽選する(なければ[None])
    抽選結果 = random.choices(抽選候補, weights=重みリスト, k=1)[0] if 抽選候補 else None
    #│
    #▼抽選結果を返す
    return 抽選結果
