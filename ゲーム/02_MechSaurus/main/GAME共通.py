#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃共通処理
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
import time
from   main.汎用部品        import mmp
from   .データセット        import データセット as DS
from   .オブジェクト.シーン import シーンID

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ＭＭＰによる演奏
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class BGM_DFP():

    # シーン用の再生時間辞書(トラック番号:再生時間(単位:秒))
    シーン = { 1:(20,60), 2:(17,80), 3:(7,95) }

	#────────────────────────────────────
    # シーンに合ったBGMを演奏する
	#────────────────────────────────────
    def 自動選択():
        シーン  = DS.情報.シーン
        if   シーン == シーンID.タイトル画面: BGM_DFP.演奏(1)
        elif シーン == シーンID.プレイ画面  : BGM_DFP.演奏(2)
        elif シーン == シーンID.終了画面    : BGM_DFP.演奏(3)
	#────────────────────────────────────
    # シーン用(フォルダNo.2)の指定トラックを演奏する
	#────────────────────────────────────
    def 演奏(引数_ファイル番号):
        音量 = BGM_DFP.シーン[引数_ファイル番号][0]
        時間 = BGM_DFP.シーン[引数_ファイル番号][1]
        DS.情報.再生時間 = 時間
        # 停止➡WAIT➡音量➡再生の順に実行すること(フリーズする為)
        入出力.MMP.DFP_Stop(1)
        time.sleep(0.2)
        入出力.MMP.DFP_Volume(1,音量)
        入出力.MMP.DFP_PlayFolderTrack(1,1,引数_ファイル番号)
	#────────────────────────────────────
    # その他用(フォルダNo.3)の指定トラックを演奏する
	#────────────────────────────────────
    def 指定曲(引数_ファイル番号, 引数_音量=20):
        # 停止➡WAIT➡音量➡再生の順に実行すること(フリーズする為)
        入出力.MMP.DFP_Stop(1)
        time.sleep(0.2)
        入出力.MMP.DFP_Volume(1,引数_音量)
        入出力.MMP.DFP_PlayFolderTrack(1,2,引数_ファイル番号)

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ＭＭＰによる入力走査
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 入出力():

    MMP     = None          # MMPオブジェクト

    MMP_ON  = [             # Chごとの状態：ON(True)/OFF(False) 
            [False,False],  # ポート0用 [Ch0,Ch1]
            [False,False],  # ポート1用 [Ch0,Ch1]
            [False,False],  # ポート2用 [Ch0,Ch1]
            [False,False],  # ポート3用 [Ch0,Ch1]
            [False,False],  # ポート4用 [Ch0,Ch1]
            [False,False],  # ポート5用 [Ch0,Ch1]
            [False,False],  # ポート6用 [Ch0,Ch1]
            [False,False]]  # ポート7用 [Ch0,Ch1]

	#────────────────────────────────────    
    def MMP_初期化():
        #┬
        #●MMPを実体化する。
        入出力.MMP = mmp()
        #│
        #●MMPを接続する
        入出力.MMP.通信接続_自動()
        #│
        #●アナログ入力を設定する
        入出力.MMP.アナログ設定(
                2,  # 使用するHC4067の個数(1～4)
                10, # 使用するHC4067のPin数(1～16)
                100 # アナログ値の丸め(この数値以下は切り捨て)
                )
        #┴
	#────────────────────────────────────
    def 入力走査(引数_ポートNo):
		#┬
		#○結果を初期化する
        概要 = [ False, False ]     # ONしている，OFFした直後
        測定 = [ 0,0 ]              # Chごとの測定値(1:ON/-1:OFFになった/0:通常のOFF) 
		#│
        #○ポートのアナログ値を用意する
        ポートNo = 引数_ポートNo
        入力値 = 入出力.MMP.mmpAnaVal[ポートNo]
		#│
		#◎└┐ON/OFF状態を求める
        for 各Ch in range(2):
            #│
            #◇┐移動予定の座標を求める
            if 入力値[各Ch] > 700:
            #　├┐（状態が『ON』の場合）
                #↓
                #○状態を『ONしている』にする
                #○測定を『ON』にする
                #○概要(ON)を『YES』にする
                入出力.MMP_ON[ポートNo][各Ch] = True
                測定[各Ch] = 1
                概要[0] = True
                #┴
            elif 入出力.MMP_ON[ポートNo][各Ch]:
            #　├┐（前回の状態が『ON』の場合）
                #↓
                #○状態を『OFF』にする
                #○測定を『OFFした直後』にする
                #○概要(OFF)を『YES』にする
                入出力.MMP_ON[ポートNo][各Ch] = False
                測定[各Ch] = -1
                概要[1] = True
                #┴
            #　└┐（その他）
                #┴
		#│
        #▼結果を返す
        return (概要,測定)
    
#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ＭＭＰデバイス(発電機)
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 発電機():
	#────────────────────────────────────
    def 右足判定(引数_角度):
		#┬
        #▼右足を踏むタイミグか否かを返す
        return (True) if 引数_角度 <= 180 else False

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃ＭＭＰデバイス(運搬機)
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 運搬機():
	#────────────────────────────────────
    def 停止():
		#┬
        #○運搬機を確認する
        if len(DS.obj.運搬機) == 0: return
        #│＼（運搬機を走査し終えた場合）
        #│ ↓
        #│ ▼処理を中断する
        #│
        #●運搬機の全機能を停止する
        運搬機.電飾制御(False)
        運搬機.モータ停止()
        #┴
	#────────────────────────────────────
    def 電飾制御(引数_活性可否): #① True:活性化／False:非活性化
		#┬
        #○引数に応じてPWM値を設定する
        PWM値 = (4095) if 引数_活性可否 else (-1)
        #│
        #≫運搬機の電飾を活性制御する
        入出力.MMP.PWM_VALUE(DS.obj.運搬機[0].仕様.電飾No, PWM値)
        入出力.MMP.PWM_VALUE(DS.obj.運搬機[1].仕様.電飾No, PWM値)
        #┴
	#────────────────────────────────────
    def モータ停止():
		#┬
        #≫運搬機の動力(モータ)を停止する
        入出力.MMP.PWM_VALUE(DS.obj.運搬機[0].仕様.動力No, -1)
        入出力.MMP.PWM_VALUE(DS.obj.運搬機[1].仕様.動力No, -1)
        #┴
