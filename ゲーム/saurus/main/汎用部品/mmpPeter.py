#====================================================== 
# ＭＭＰライブラリ Ver 0.02 ペーター
#------------------------------------------------------
# Ver 0.02.006　2025/06/05 By Takanari.Kureha
#       1.不要なimportを削除
#------------------------------------------------------
# Ver 0.02.005　2025/04/28 By Takanari.Kureha
#       1.PWMエキスパンダ(PCA9685)マルチ対応
#------------------------------------------------------
# Ver 0.02.004　2025/04/24 By Takanari.Kureha
#       1.PWMエキスパンダ(PCA9685)対応
#------------------------------------------------------
# Ver 0.02.003　2025/04/18 By Takanari.Kureha
#       1.テスト関数に引数でウェイトを指定可能にした
#       2.丸めの処理を修正
#           ・1未満の場合は１に補正(バグ修正)
#           ・1以外で丸めを実施する(速度向上)
#------------------------------------------------------
# Ver 0.02.002　2025/04/17 By Takanari.Kureha
#       1.テスト関数(全件表示)バグ修正
#------------------------------------------------------
# Ver 0.02.001　2025/04/16 By Takanari.Kureha
#        2.RP2040-Zero向けにチューニング
#====================================================== 

#====================================================== 
# インクルード
#====================================================== 
import serial
import time

#====================================================== 
# クラス
#====================================================== 

#=============
# 例外クラス
#=============
class ConnectException(Exception):
    pass

#=============
# 本体クラス
#=============
class mmp:
    #=====================================================================
    # 初期化処理
    #=====================================================================
    #┬
    #〇└┐初期化：
    def __init__( 
        self,
        argMmpNum           = 4,                # 使用するHC4067の個数
        argMmpAnaPins       = 1,                # 使用するHC4067のPin数
        argMmpAdrPins       = (10,11,12,13),    # RP2040-Zero
        #argMmpAdrPins      = (2,3,4,5),         # Arduino
        #argMmpAdrPins      = (6,7,8,9),         # PiZero
        argRundNum = 5                          # アナログ値の丸め
        ):
        #│
        #〇接続する。
        print("\n１．接続準備")
        self.ser = serial.Serial()
        self.ser.baudrate   = 921600
        self.connect_flag   = False
        self.version = ""
        #│
        #〇アナログPinの使用範囲(HC4067)
        print("２．アナログ入力準備")
        self.mmpNum         = argMmpNum 
        self.mmpAnaPins     = argMmpAnaPins
        print("　・HC4067 使用個数        : %i"  % self.mmpNum  )
        print("　・HC4067 使用ポート数    : %i"  % self.mmpAnaPins  )
        #│
        #〇HC4067アドレス指定のデジタルPin(ボード)
        self.mmpAdrPins     = argMmpAdrPins
        print("　・HC4067 アドレス指定Pin : "   , self.mmpAdrPins)
        #│
        #〇データの丸め
        self.mmpRoundNum    = int(argRundNum)
        if argRundNum < 1 : argRundNum = 1
        print("　・アナログ値の丸め処理   : "   , self.mmpRoundNum)
        #│
        #〇Bitパターン(4Bit)を登録する。
        self.initANA_BitList()
        #│
        #〇測定データをゼロ初期する。
        self.initANA_Value()
        #┴
    #┴
    #---------------------------------------------------------------------
    #┬
    #〇└┐１．Bitパターン：
    def initANA_BitList(self):
        #◎└┐アドレスごとのBitパターンを定義する。
        self.mmpBit = [] 
        for cntAnaPin in range(2**4):
            #◎└┐Bitパターンを作成する。
            tmpBit = [0,0,0,0]
            for cntBit in range(4):
                #〇当該桁のBitを更新する。
                tmpBit[cntBit]  = int(bin((cntAnaPin >> cntBit) & 0b1)[2:])
                #┴
            #│
            #〇当該アドレスのBitパターンを追加する。
            self.mmpBit.append(tmpBit)
            #┴
        #┴
    #┴
    #---------------------------------------------------------------------
    #┬
    #〇└┐２．測定データ：
    def initANA_Value(self):
        #◎└┐1アドレス分の空データを作る。
        tmpValAnaPin = [] 
        for cntMmpNo in range(self.mmpNum):
            #〇読取値の初期値(ゼロ)を追加する。
            tmpValAnaPin.append(0)
            #┴
        #│
        #◎└┐全アドレスの測定データに空データをセットする。
        self.mmpAnaVal = [] 
        for cntAnaPin in range(self.mmpAnaPins):
            #〇測定データ(初期値)を追加する。
            self.mmpAnaVal.append(tmpValAnaPin.copy())
            #┴
        #┴
    #┴

    #=====================================================================
    # アナログ入力
    #=====================================================================
    #---------------------------------------------------------------------
    # 全アドレス(チャンネル)
    #---------------------------------------------------------------------
    #┬
    #〇└┐アナログ測定（全アドレス）：
    def analog_IN_All(self):
        self.ser.reset_input_buffer
        self.ser.reset_output_buffer
        #◎└┐繰り返し読取る。
        for cntAnaPin in range(self.mmpAnaPins):
            #〇1アドレス読取る。
            self.analog_IN_Each(cntAnaPin)
            #┴
    #┴
    #---------------------------------------------------------------------
    # 全モジュールの特定アドレス
    #---------------------------------------------------------------------
    #┬
    #〇└┐アナログ測定（１アドレス）：
    def analog_IN_Each(self, argAnaPin):
        self.ser.reset_input_buffer
        self.ser.reset_output_buffer
        #◎└┐アドレスをセットする。
        for cntBit in range(4):
            #〇当該ビットにBitパターンをセットする。
            tmpPort = self.mmpAdrPins[cntBit]
            tmpBit  = self.mmpBit[argAnaPin][cntBit]
            self.digital_OUT(tmpPort,tmpBit)
        #│
        #◎└┐当該アドレスの全アナログ・ポートを読み取る。
        for cntMmpID in range(self.mmpNum):
            tmpData = self.analog_IN(cntMmpID)
            self.mmpAnaVal[argAnaPin][cntMmpID] = tmpData
            #┴
        #┴
    #┴
    #---------------------------------------------------------------------
    # マイコンの特定ANAチャンネル
    #---------------------------------------------------------------------
    #┬
    #〇└┐アナログ測定（１ポート）：
    def analog_IN(self, argAnaPin):
        self.ser.reset_input_buffer
        self.ser.reset_output_buffer
        #◎└┐アドレスをセットする。
        data3=''
        while data3=='----' or data3=='':
            data = "ADR:%02x!" % (argAnaPin)
            self.ser.write(str.encode(data))
            data = self.ser.read(5)
            data2 = data.decode('utf-8')
            data3 = data2.replace('!', '')
        try:
            data4 = int(data3, 16) 
            if self.mmpRoundNum != 1 :
                data4 = int(data4 / self.mmpRoundNum) * self.mmpRoundNum
            return data4
        except:
            print("エラー:[",argAnaPin,"][1:",data,"][2:",data2,"][3:",data3,"]")
        #┴
    #┴
    #---------------------------------------------------------------------
    #┬
    #〇└┐テスト（アナログ測定）：
    def analog_Test(
        self,
        argLoop = 100,      # アドレス切替回数
        argWait = 0.0,      # ウェイト(秒)
        argAll  = False     # True:全件表示／False:先頭末尾のみ表示
        ):
        #◎└┐繰り返し読み取る。
        print("--------------------")
        print(" アナログ入力テスト")
        print("--------------------")
        print("(測定データ)")
        tmpLoop = int(argLoop / self.mmpAnaPins) + 1 
        time_start = time.time()
        for cntLoop in range(tmpLoop):        
            #◎└┐全アドレスから読み取る。
            self.analog_IN_All()

            if argWait > 0 : time.sleep(argWait)

            #◇データを表示する。
            if argAll:
                print("　%03i" % cntLoop,":", self.mmpAnaVal)
            else:
                print("　%03i" % cntLoop,":", self.mmpAnaVal[0],"～", self.mmpAnaVal[-1])

        #◇結果を表示する。
        time_end = time.time()
        time_diff = time_end - time_start

        print("\n(実施条件)")
        print("・繰返回数         : %i" % (tmpLoop                      ))
        print("・アドレス変更回数 : %i" % (self.mmpAnaPins * tmpLoop    ))
 
        print("\n(測定結果)")
        cntTtl = tmpLoop * self.mmpNum * self.mmpAnaPins
        print("・合計時間   : %02.06f秒"       % (time_diff             ))
        print("・データ平均 : %01.06f秒\n"   % (time_diff/cntTtl        ))
        #┴
    #┴

    #=====================================================================
    # デジタル入出力
    #=====================================================================
    #---------------------------------------------------------------------
    # ＰＷＭ(PCA9685)：PWM値指定
    #---------------------------------------------------------------------
    def PWM_VALUE(self, argPort, argValue):
        self.ser.reset_input_buffer
        self.ser.reset_output_buffer
        data = "PWM:%02x:%01x!" % (argPort, argValue)
        self.ser.write(str.encode(data))
        data    = self.ser.read(5)
        data    = self.ser.read(5)
#        data2   = data.decode('utf-8')
#        data3   = data2.replace('!', '')
    #---------------------------------------------------------------------
    # ＰＷＭ(PCA9685)：角度指定
    #---------------------------------------------------------------------
    def PWM_ANGLE(self, argPort, argValue):
        data = "PWA:%02x:%01x!" % (argPort, argValue)
        self.ser.write(str.encode(data))
#        data    = self.ser.read(5)
#        data2   = data.decode('utf-8')
#        data3   = data2.replace('!', '')

    #=====================================================================
    # デジタル入出力
    #=====================================================================
    #---------------------------------------------------------------------
    # 入出力(デジタル)
    #---------------------------------------------------------------------
    def digital_IO(self, argPort, argValue):
        data = "IO:%02x:%01x!" % (argPort, argValue)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
        data2 = data.decode('utf-8')
        data3 = data2.replace('!', '')
        return int(data3, 16)
    #---------------------------------------------------------------------
    # 出力
    #---------------------------------------------------------------------
    def digital_OUT(self, argPort, argValue):
        data = "POW:%02x:%01x!" % (argPort, argValue)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
        data2 = data.decode('utf-8')
        data3 = data2.replace('!', '')
    #---------------------------------------------------------------------
    # 入力
    #---------------------------------------------------------------------
    def digital_IN(self, argPort):
        data = "POR:%02x!" % (argPort)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
        data2 = data.decode('utf-8')
        data3 = data2.replace('!', '')
        return int(data3, 16)
    #---------------------------------------------------------------------
    def readline(self):
        s = ''
        while self.ser.inWaiting():
            d = self.ser.read()
            if( d == '!'):
                return s
            s += d

    #=====================================================================
    # 接続処理
    #=====================================================================
    def autoConnect(self):
        print("\n接続中...")
        for i in range(100):
            com_str = 'COM%d'%i
            try:
                self.ser.port = com_str
                self.ser.open()
                break
            except:
                i = i
        time.sleep(2)
        if self.ser.port == 'COM99':
            raise ConnectException()

        print("接続済み")
        self.connect_flag = True
        return com_str
    #---------------------------------------------------------------------
    def connect(self, comm_num):
        try:
            self.ser.port = comm_num
            self.ser.open()
        except:
            raise ConnectException()
        time.sleep(2)
        self.connect_flag = True
    #---------------------------------------------------------------------
    def disconnect(self):
        self.ser.close()
        self.connect_flag = False
    #---------------------------------------------------------------------
    def reset(self):
        self.ser.write(str.encode("!"))
        data = self.ser.read(5)

    #=====================================================================
    # i2c
    #=====================================================================
    def i2cWrite(self, slave_addr, addr, val):
        data = "I2W:%02x:%02x:%02x:1!" % (slave_addr, addr, val)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
        if( data.decode('utf-8') != '----!'):
            return True
        else:
            return False
    #---------------------------------------------------------------------
    def i2cRead(self, slave_addr, addr):
        data = "I2R:%02x:%02x:1!" % (slave_addr, addr)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
        data2 = data.decode('utf-8')
        data3 = data2.replace('!', '')
        return int(data3, 16)
    #---------------------------------------------------------------------
    def i2cWrite_word(self, slave_addr, addr, val):
        data = "I2W:%02x:%02x:%02x:2!" % (slave_addr, addr, val)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
        if( data.decode('utf-8') != '----!'):
            return True
        else:
            return False
    #---------------------------------------------------------------------
    def i2cRead_word(self, slave_addr, addr):
        data = "I2R:%02x:%02x:2!" % (slave_addr, addr)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
        data2 = data.decode('utf-8')
        data3 = data2.replace('!', '')
        return int(data3, 16)