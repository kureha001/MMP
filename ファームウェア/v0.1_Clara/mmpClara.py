#====================================================== 
# ＭＭＰライブラリ Ver 0.01 クララ
# Ver 0.01.001　2025/04/14 By Takanari.Kureha
#               ・RP2040-Zero向けにチューニング
#====================================================== 

#====================================================== 
# インクルード
#====================================================== 
from pyfirmata2 import Arduino  # Firmate
import time                     # 日時

#====================================================== 
# クラス
#====================================================== 
#【MMP】
class MMP:

    #〇クラス資源
    ObjBoard = None # MMPのオブジェクト
    CntFinish = 0   # アナログPinのコールバック済件数

    #┬
    #〇└┐初期化処理：
    def __init__(
        self, 
        #------------------------------------------------------------
        argAnaPins      = 2,                # ボードのアナログPin数 
        argMpxPins      = 2,                # HC4067のアナログPin数
        #------------------------------------------------------------
        argWaitMpx      = 0.00195,          # HC4067のウェイト(秒)
        argWaitCBak     = 0.0,              # CallBackのウェイト(秒)
        #------------------------------------------------------------
        argSInterval    = 10,               # サンプリング間隔(ミリ秒)
        argSRate        = 12,               # サンプリング・レート(Hz)
        #------------------------------------------------------------
        argAnaInPin     = [],               # アナログ読取Pin
        argAdrOutPin    = [10,11,12,13]     # HC4067アドレス指定Pin
        ):
        #○MMPを接続する。
        print("接続中...")
        MMP.ObjBoard = Arduino(Arduino.AUTODETECT) 
        print("", MMP.ObjBoard.get_firmata_version)

        #○MMPをパラメータを設定する。
        MMP.ObjBoard.setSamplingInterval(argSInterval) 
        MMP.ObjBoard.samplingOn(argSRate)  
        print("・サンプリング間隔      ：%iミリ秒" % argSInterval)
        print("・サンプリング・レート  ：%iHz" % argSRate)
        #│
        #〇アナログPinの使用範囲(HC4067)
        self.anaPins    = argAnaPins    
        self.mpxPins    = argMpxPins   
        print("・アナログPin数(ボード) ：%i個"  % self.anaPins  )
        print("・アナログPin数(HC4067) ：%i個"  % self.mpxPins  )
        #│
        #〇ウェイト
        self.waitMpx    = argWaitMpx  
        self.waitCBak   = argWaitCBak
        print("・ウェイト(HC4067)      ：%i秒"  % self.waitMpx  )
        print("・ウェイト(CallBackk)   ：%i秒"  % self.waitCBak )
        #│
        #〇入力値読取のアナログPin(ボード)\
        self.anaInPin   = []
        if not argAnaInPin :
            for i in range(self.anaPins) : self.anaInPin.append(i)
        else : self.anaInPin = argAnaInPin
        print("・アナログ読取Pin       ："      , self.anaInPin )
        #│
        #〇HC4067アドレス指定のデジタルPin(ボード)
        self.adrOutPin  = argAdrOutPin
        print("・HC4067アドレス指定Pin ："      , self.adrOutPin)
        #│
        #〇初期する。
        self.initDefine()       # データ宣言
        self.initAnalogPin()    # アナログPin
        self.initBitList()      # Bitパターン
        self.initValAnaPin()    # 測定データ
        #│
        print("接続完了.\n")
        MMP.CntFinish = self.anaPins
        #┴
    #┴
    #┬
    #〇└┐２．データ宣言：
    def initDefine(self):
        #○HC4067関連
        print(self.adrOutPin)
        print('d:%i:o' % self.adrOutPin[0])
        print('d:%i:o' % self.adrOutPin[1])
        print('d:%i:o' % self.adrOutPin[2])
        print('d:%i:o' % self.adrOutPin[3])
        self.adrPin = [             # アドレス指定用のデジタルPin
                MMP.ObjBoard.get_pin('d:%i:o' % self.adrOutPin[0]),
                MMP.ObjBoard.get_pin('d:%i:o' % self.adrOutPin[1]),
                MMP.ObjBoard.get_pin('d:%i:o' % self.adrOutPin[2]),
                MMP.ObjBoard.get_pin('d:%i:o' % self.adrOutPin[3])
                ]
        self.mpxBitList     = []    # Bitパターン(4Bit)
        #│
        #○アナログ値読取り関連
        self.objAnaPin      = []    # 読取用のアナログPin(object)
        self.valAnaPin      = []    # 測定データ
        #│
        #○その他
        self.statusReady    = True  # ステータス
        #┴
    #┴
    #┬
    #〇└┐３．アナログPin：
    def initAnalogPin(self):
        #◎└┐アナログPinを初期化する。(値読取り用)
        for cntPin in range(self.anaPins):
            #〇当該アナログPinを実体化する。
            self.objAnaPin.append(MMP_AnaPort(cntPin))
            #┴
        #┴
    #┴
    #┬
    #〇└┐４．Bitパターン：
    def initBitList(self):
        #◎└┐アドレスごとのBitパターンを定義する。
        for cntMpxPin in range(2**4):
            #◎└┐Bitパターンを作成する。
            tmpBitList = [0,0,0,0]
            for cntBit in range(4):
                #〇当該桁のBitを更新する。
                tmpBitList[cntBit]  = int(bin((cntMpxPin >> cntBit) & 0b1)[2:])
                #┴
            #│
            #〇当該アドレスのBitパターンを追加する。
            self.mpxBitList.append(tmpBitList)
            #┴
        #┴
    #┴
    #┬
    #〇└┐５．測定データ：
    def initValAnaPin(self):
        #◎└┐アナログPinを初期化する。(値読取り用)
        tmpValAnaPin = [] 
        for cntPin in range(self.anaPins):
            #〇読取値の初期値(ゼロ)を追加する。
            tmpValAnaPin.append(0)
            #┴
        #│
        #◎└┐測定データをゼロ初期化する。
        for cntPin in range(self.mpxPins):
            #〇測定データ(初期値)を追加する。
            self.valAnaPin.append(tmpValAnaPin.copy())
            #┴
        #┴
    #┴
    #-----------------------------------------------------------------------------
    #┬
    #〇└┐読取り処理：
    def read(self):
        #○ステータスを処理中にセットする。
        self.statusReady = False
        #│
        #◎└┐全アドレス(コントローラ)から読み取る。
        for cntMpxPin in range(self.mpxPins):

            #●当該アドレスをコールバック登録する。
            self.read_CallBack(cntMpxPin)
            #│
            #〇コールバックと時間調整する。
            while self.read_Ready() == False:
                time.sleep(self.waitCBak)
            #│
            #◎└┐値を格納する。
            for cntAnaPin in range(self.anaPins):
                #〇当該アナログPinの値を格納する。
                self.valAnaPin[cntMpxPin][cntAnaPin] = self.objAnaPin[cntAnaPin].valAnaPin
                #┴
            #┴
        #│ 
        #○ステータスを終了にセットする。
        self.statusReady = True
        #┴
    #┴
    #┬
    #〇└┐読取り処理（コールバック完了）：
    def read_Ready(self):
        if MMP.CntFinish < self.anaPins : return False
        return True
    #┴
    #┬
    #〇└┐読取り処理（コールバック）：
    def read_CallBack(self, argAdr):
        #○コールバック済み件数をゼロ初期化する。
        MMP.CntFinish = 0
        #│
        #◎└┐引数アドレスのBitパターンをデジタルPinにセットする。
        for cntBit in range(4):
            #〇当該ビットにBitパターンのBit値をセットする。
            self.adrPin[cntBit].write(self.mpxBitList[argAdr][cntBit])
            #┴
        #│
        #〇HC4067のChが切り替わるまで待つ。
        time.sleep(self.waitMpx) 
        #│
        #◎└┐全アナログPinにコールバックを登録する。
        for cntObj in range(self.anaPins):
            #〇当該アナログPinにコールバックを登録する。
            self.objAnaPin[self.anaInPin[cntObj]].registCallBack()
            #┴
        #┴
    #┴
    #┬
    #〇└┐切断処理：
    def stop(self):
        #〇MMPを切断する。
        MMP.ObjBoard.exit() 
        #┴
    #┴
    #-----------------------------------------------------------------------------
    #┬
    #〇└┐読取りテスト：
    def readTest(self,argCntLOOP=50,argAll=True):
        #〇テストを開始する。
        time_start = time.time()
        #│
        #◎└┐繰り返し読取る。
        for i in range(argCntLOOP):
            #〇1回だけ読取る。
            self.read()
            #│
            #〇結果を表示する。(先頭と末尾のコントローラのみ)
            if argAll:
                print("%04i:" % (i), self.valAnaPin)
            else:
                print("%04i:" % (i), self.valAnaPin[0],"～", self.valAnaPin[-1])
            #┴
        #│
        #〇所要時間・平均時間を表示する。
        time_end = time.time()
        time_diff = time_end - time_start
        cntTtl  = argCntLOOP * self.anaPins * self.mpxPins
        print("\n(実施条件)")
        print("・繰返回数:%i" % (argCntLOOP))
        print("・アドレス変更回数:%i" % (argCntLOOP * self.mpxPins))
        print("・１回のANA入力数：%i" % (self.anaPins * self.mpxPins))
        print("　※ 内訳（ANA入力ピン数：%i，コントローラ数：%i）"
            % (self.anaPins, self.mpxPins)
            )
        print("\n(測定結果)")
        print("・所要時間：%02.06f秒\n・平均時間：%01.06f秒/Ch\n"
            % (time_diff, time_diff/cntTtl)
            )
        #┴
    #┴

#【MMP読取サブ】
class MMP_AnaPort:
    #┬
    #〇└┐初期化処理：
    def __init__(self, argAnaPin):
        #〇ＩＤを登録する。
        self.anaPin = argAnaPin
        #│
        #〇測定値をゼロ初期化する。
        self.valAnaPin = 0
        #│
        #〇コールバックをオフにする。
        MMP.ObjBoard.analog[self.anaPin].unregiser_callback()
        #┴
    #┴
    #-----------------------------------------------------------------------------
    #┬
    #〇└┐コールバック登録：
    def registCallBack(self):
        #│
        #〇測定値をゼロ初期化する。
        self.valAnaPin = 0
        #│
        #〇コールバック登録する。
        MMP.ObjBoard.analog[self.anaPin].register_callback(self.riseCallBack)
        MMP.ObjBoard.analog[self.anaPin].enable_reporting()
        #┴
    #┴
    #-----------------------------------------------------------------------------
    #┬
    #〇└┐コールバック処理：
    def riseCallBack(self, data):
        #│
        #〇コールバックをオフにする。
        MMP.ObjBoard.analog[self.anaPin].unregiser_callback()
        #│
        #〇取得値をメンバ変数に格納する。
        self.valAnaPin = int(data * 100)
        MMP.CntFinish += 1
        #┴
    #┴
#┴