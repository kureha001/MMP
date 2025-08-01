#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃シーン：プレイ画面(移動機能)
#┠─────────────────────────────────────
#┃更新コントローラが移動プロセスで実行するアクション・メソッド
#┃・下位にデータセット･クラス(仕様｜情報)を持つ
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
import  pyxel
import  main.GAME共通       as 共通処理
from    main.データセット   import データセット as DS
from    ..DB                import シーンID

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃データセット：仕様
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 仕様クラス:
    #------------------------------------------------------------------------
    # カウンタ
    #------------------------------------------------------------------------
    #□持続時間  ：ボス警告を持続する長さ（単位：fps）
    #□終了時間  ：終了画面に進行するまでの時間
    持続時間    = 150
    終了時間    = 50

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃データセット：情報
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 情報クラス:
    #------------------------------------------------------------------------
    # カウンタ
    #------------------------------------------------------------------------
    #□開始時間  ：ボス警告を持続するカウンタ
    開始時間    = None

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃メイン
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 移動クラス:
    #┌───────────────────────────────────
    #│初期化
    #└───────────────────────────────────
    def __init__(self):
        #┬
        #〇個体オブジェクトのリファレンスを用意する
        self._仕様  = DS.仕様
        self._情報  = DS.情報

        #≫データセットを用意する
        self.仕様 = 仕様クラス()
        self.情報 = 情報クラス()
        #┴

    #┌───────────────────────────────────
    #│機能実行
    #└───────────────────────────────────
    def 実行(self):
        #┬
        #◇┐状況に応じて進行する 
        if DS.obj.自機共通.情報.シールド <= 0:
        #　├┐（シールドが切れた場合）
            #↓
            #●次のシーン『終了画面』に進行する
            self.Fn次シーン準備()
            #┴   

        elif self._情報.ボスシーン != None:
        #　├┐（ボス襲来で警告中の場合）
            #↓
            #●ボスの登場を処理する
            self.Fnボス処理()
            #┴            

        elif self._情報.プレイ時間 % self._仕様.基本.難易度間隔 == 0:
        #　├┐（レベルの変わり目の場合）
            #↓
            #○ボスシーンを『ボス警告』に進行する
            #○ボス警告カウントを開始する
            #○プレイ時間を1つ進める ※ボスシーンを繰り返さない為
            self._情報.ボスシーン = シーンID.ボス警告
            self.情報.開始時間 = pyxel.frame_count
            self._情報.プレイ時間 += 1
            #┴            

        else:
        #　└┐（その他）
            #↓
            #○プレイ時間を１つ進める
            #○難易度をセットする
            self._情報.プレイ時間 += 1
            self._情報.難易度 = self._情報.プレイ時間 // self._仕様.基本.難易度間隔 + 1
        #┴　┴

    #┌───────────────────────────────────
    #│ボス登場を確認する
    #└───────────────────────────────────
    def Fnボス処理(self):
        #┬
        #○ボスシーンを確認する
        if self._情報.ボスシーン == None: return
        #│＼（ボス警告ではない場合）
        #│ ↓
        #│ ▼処理を中断する
        #│
        #◇┐ボスシーンを進行する
        if self._情報.ボスシーン == シーンID.ボス警告:
        #　├┐（ボス警告の場合）
            #↓
            #○警告時間を減らす
            経過時間 = pyxel.frame_count - self.情報.開始時間
            時間あり = (経過時間 < self.仕様.持続時間)
            if 時間あり: return
            #│＼（まだ時間がある場合）
            #│ ↓
            #│ ▼処理を中断する
            #│
            #○ボスシーンを『ボス登場』に進行する
            #○タイマーをリセットする
            #●BGMを切替える
            self._情報.ボスシーン = シーンID.ボス登場
            self.情報.開始時間 = None
            共通処理.BGM_DFP.指定曲ボス()
            #│
            #●特殊効果を解除(永続は残す)
            #〇シールドを満タンにする
            #〇弾を満タンにする
            DS.obj.特殊効果.FN移動.強制解除()
            DS.obj.自機共通.情報.シールド = self._仕様.基本.画面幅
            DS.obj.自機共通.情報.弾数 = DS.obj.自機共通.仕様.積載量
            #│
            #●敵機・アイテムを消滅する
            共通処理.爆発.壊滅(DS.obj.敵機    )
            共通処理.爆発.壊滅(DS.obj.アイテム)
            共通処理.効果音.爆発(1)
            #┴

        elif self._情報.ボスシーン == シーンID.ボス対決:
        #　├┐（ボス対決の場合）
            #↓
            #〇ボスを確認する
            if len(DS.obj.敵機) > 0: return
            #│＼（まだボスがいる場合）
            #│ ↓
            #│ ▼処理を中断する
            #│
            #〇ボスシーンを終了する
            #●BGMを切替える
            self._情報.ボスシーン = None
            共通処理.BGM_DFP.指定曲ザコ()
            共通処理.効果音.爆発(2)
            #│
            #〇シールドを満タンにする
            #〇弾を満タンにする
            DS.obj.自機共通.情報.シールド = self._仕様.基本.画面幅
            DS.obj.自機共通.情報.弾数 = DS.obj.自機共通.仕様.積載量
            #┴

    #┌───────────────────────────────────
    #│次シーンの前準備
    #└───────────────────────────────────
    def Fn次シーン準備(self):
        #┬
        #◇┐終了画面を開始する
        if self.情報.開始時間 is None:
        #　├┐（待ちを開始する場合）
            #↓
            #○持続カウントを開始する
            self.情報.開始時間   = pyxel.frame_count
            #│
            #◎└┐自機を爆発する
            for tmp自機 in DS.obj.自機:
                #│＼（すべての処理を終えた場合）
                #│ ↓
                #│ ▼繰り返し処理を抜ける
                #│
                #●大爆発する
                d = DS.obj.自機共通.仕様.爆発オフセット
                x = tmp自機.情報.X + d
                y = tmp自機.情報.Y
                共通処理.爆発.大爆発(x, y, 4, 6)
            #│
            #〇自機を削除する
            #●爆発音を鳴らす
            DS.obj.自機 = []
            共通処理.効果音.爆発(2)
            #┴
        #　└┐（その他）
            #↓
            #┴
        #│
        #◇┐次のシーンに進行する
        経過時間 = pyxel.frame_count - self.情報.開始時間
        時間あり = (経過時間 < self.仕様.終了時間)
        if 時間あり: return
        #│＼（まだ時間がある場合）
        #│ ↓
        #│ ▼処理を中断する
        #│
        #○シーンを『終了画面』に進行する
        #○ボスシーンをリセットする
        self._情報.シーン   = シーンID.終了画面
        DS.情報.ボスシーン  = None
        self.情報.開始時間  = None
        #┴