#====================================================== 
# ＭＭＰライブラリ Ver 0.03 Rottenmeier
#------------------------------------------------------
# Ver 0.03.001　2025/06/01 By Takanari.Kureha
#       1.ファームウェア修正に伴いリカバコードを削除
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
    #argMmpAdrPins      = (10,11,12,13),    # RP2040-Zero
    #argMmpAdrPins      = (2,3,4,5),         # Arduino
    #argMmpAdrPins      = (6,7,8,9),         # PiZero
    #=====================================================================
    #┬
    #〇└┐初期化：
    def __init__(self):
        #│
        #〇接続する。
        print("\n<<接続準備>>")
        self.ser = serial.Serial()
#        self.ser.baudrate   = 921600
        self.ser.baudrate   = 115200
        self.connect_flag   = False
        self.version = ""
        #│
        #〇アナログパラメータを初期化する
        self.参加総人数  = None
        self.スイッチ数  = None 
        self.丸め        = None
        #┴

    #=====================================================================
    # 接続処理
    #=====================================================================
    def 通信接続_自動(self):
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

        self.version = self.バージョン()
        print(f"接続済み(Ver.{self.version})")
        self.connect_flag = True
        return com_str
    #---------------------------------------------------------------------
    def 通信接続(self, comm_num):
        try:
            self.ser.port = comm_num
            self.ser.open()
        except:
            raise ConnectException()
        time.sleep(2)
        self.connect_flag = True
    #---------------------------------------------------------------------
    def 通信切断(self):
        self.ser.close()
        self.connect_flag = False
    #---------------------------------------------------------------------
    def バージョン(self):
        self.ser.reset_input_buffer()  # ゴミを捨てる
        self.ser.write(b'VER!')
        data = self.ser.read(5)
        sdata = data.decode('ascii')
        print(f"受信データ: '{sdata}'")
        結果 = f"{sdata[0]}.{sdata[1]}.{sdata[2:4]}"
        return 結果

    #=====================================================================
    # アナログ入力
    #=====================================================================
    #---------------------------------------------------------------------
    # 設定コマンド
    #---------------------------------------------------------------------
    #┬
    #〇└┐測定データ：
    def アナログ初期化(self):
        #◎└┐1アドレス分の空データを作る。
        tmpスイッチ = [] 
        for cntMmpNo in range(self.スイッチ数):
            #〇読取値の初期値(ゼロ)を追加する。
            tmpスイッチ.append(0)
            #┴
        #│
        #◎└┐全アドレスの測定データに空データをセットする。
        self.mmpAnaVal = [] 
        for cntAnaPin in range(self.参加総人数):
            #〇測定データ(初期値)を追加する。
            self.mmpAnaVal.append(tmpスイッチ.copy())
    #┴　┴　┴
    #---------------------------------------------------------------------
    # 設定コマンド
    #---------------------------------------------------------------------
    #┬
    #〇└┐アナログ測定：
    def アナログ設定(self  ,
            argスイッチ数 = 4, # 使用するHC4067の個数(1～4)
            arg参加人数   = 1, # 使用するHC4067のPin数(1～16)
            arg丸め       = 5  # アナログ値の丸め
            ):
        #│
        #〇アナログパラメータをセットする
        print("<<アナログ入力準備>>")
        self.参加総人数  = arg参加人数
        self.スイッチ数  = argスイッチ数 
        self.丸め        = int(arg丸め)
        #│
        print(" ・HC4067 使用個数     : %i"  % self.スイッチ数  )
        print(" ・HC4067 使用ポート数 : %i"  % self.参加総人数  )
        print(" ・アナログ値の丸め処理: "   , self.丸め)
        #│
        #〇測定データをゼロ初期する。
        self.アナログ初期化()
        #│
        data = f"ANS:{self.参加総人数:02X}:{self.スイッチ数:02X}!"
        print(data)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
        #┴
    #---------------------------------------------------------------------
    # 読取コマンド
    #---------------------------------------------------------------------
    #┬
    #〇└┐アナログ測定：
    def アナログ読取(self):
        data = "ANU!"
        self.ser.write(str.encode(data))
        data = self.ser.read(5)

        #◎└┐プレイヤー分を読み取る。
        for pl in range(self.参加総人数):

            #◎└┐スイッチ分を読み取る。
            for sw in range(self.スイッチ数):

                data = "ANR:%02x:%01x!" % (pl, sw)
                self.ser.write(str.encode(data))
                data = self.ser.read(5)
                data = data.decode('utf-8')
                data = data.replace('!', '')
                data = int(data, 16)
                data = int(data / self.丸め) * self.丸め
                self.mmpAnaVal[pl][sw] = data
        #┴　┴　┴

    #=====================================================================
    # デジタル入出力
    #=====================================================================
    #---------------------------------------------------------------------
    # ＰＷＭ(PCA9685)：接続状態確認（機器No.0～15）
    #---------------------------------------------------------------------
    def PWM_機器確認(self):
        cmd = "PWS!"
        self.ser.write(cmd.encode("ascii"))
        res = self.ser.read(5).decode('ascii').replace("!", "")
        try:
            各機状況 = int(res, 16)
            self.PWM機器状況 = [(各機状況 >> i) & 1 == 1 for i in range(16)]
            return self.PWM機器状況
        except ValueError:
            print(f"PCA9685ステータス取得失敗:{res}")
            self.PWM機器状況 = [False] * 16
            return self.PWM機器状況
    #---------------------------------------------------------------------
    # ＰＷＭ(PCA9685)：接続状態確認（チャンネルNo.0～255）
    #---------------------------------------------------------------------
    def PWM_チャンネル確認(self):
        # まず接続状況を確認（16個分）
        self.PWM_機器確認()  # ← self.PWM機器状況 が更新される（長さ16）
        
        # 使用可能なPWMチャンネル（0〜255）をTrue/Falseで保持
        self.PWMチャンネル状況 = [False] * 256
        for 各機器No in range(16):
            if self.PWM機器状況[各機器No]:
                オフセット = 各機器No * 16
                for チャンネル in range(16):
                    self.PWMチャンネル状況[オフセット + チャンネル] = True
    #---------------------------------------------------------------------
    # 指定したPWMチャンネル（0〜255）が使用可能かを確認
    #---------------------------------------------------------------------
    def PWM_チャンネル状況(self, channel):
        if not (0 <= channel <= 255):
            raise ValueError("[エラー] PWMチャンネルは 0〜255 の範囲で指定")
        
        pwm_id = channel // 16  # 0〜15 の PCA9685番号
        if len(self.PWM機器状況) != 16:
            print("[警告] PWMステータスが未取得")
            return False
        
        return self.PWM機器状況[pwm_id]
    #---------------------------------------------------------------------
    # ＰＷＭ(PCA9685)：PWM値指定
    #---------------------------------------------------------------------
    def PWM_VALUE(self, argPort, argValue):
        data = "PWM:%02x:%01x!" % (argPort, argValue)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
    #---------------------------------------------------------------------
    # ＰＷＭ(PCA9685)：角度指定
    #---------------------------------------------------------------------
    def PWM_ANGLE(self, argPort, argValue):
        data = "PWA:%02x:%01x!" % (argPort, argValue)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)

    #=====================================================================
    # デジタル入出力
    #=====================================================================
    #---------------------------------------------------------------------
    # 入出力(デジタル) ※出力後に入力する
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
        cmd = "POW:%02x:%01x!" % (argPort, argValue)
        self.ser.reset_input_buffer()  # 念のためバッファをクリア
        self.ser.write(cmd.encode("ascii"))       
        data = self.ser.read(5)
        try:
            response = data.decode("ascii")
        except UnicodeDecodeError:
            print(f"受信データが不正: {data}")
            return False

        if response == "!!!!!":
            return True
        elif response == "----!":
            print(f"[エラー] 無効なポートまたは設定: {cmd}")
            return False
        else:
            print(f"[警告] 予期しない応答: {response}")
            return False
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
    # ＭＰ３プレイヤー
    #=====================================================================
    # 機器情報
    def DFP_Info(self, 引数_devNo):
        cmd = f"DPX:{引数_devNo:01x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # 指定トラック再生
    def DFP_Play(self, 引数_devNo, track_num):
        cmd = f"DPL:{引数_devNo:01x}:{track_num:02x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # 指定トラックのループ再生開始
    def DFP_PlayLoop(self, 引数_devNo, track_num):
        cmd = f"DRP:{引数_devNo:01x}:{track_num:02x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # 指定フォルダ内トラック再生
    def DFP_PlayFolderTrack(self, 引数_devNo, folder, track):
        cmd = f"DIR:{引数_devNo:01x}:{folder:02x}:{track:02x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # 停止
    def DFP_Stop(self, 引数_devNo):
        cmd = f"DSP:{引数_devNo:01x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # 一時停止
    def DFP_Pause(self, 引数_devNo):
        cmd = f"DPA:{引数_devNo:01x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # 再生再開
    def DFP_Resume(self, 引数_devNo):
        cmd = f"DPR:{引数_devNo:01x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # 音量設定（0〜30）
    def DFP_Volume(self, 引数_devNo, vol):
        cmd = f"VOL:{引数_devNo:01x}:{vol:02x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # 再生中トラック番号取得（戻り値は16進数）
    def DFP_get_current_track(self, 引数_devNo):
        cmd = f"DQT:{引数_devNo:01x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # 再生状態：0停止, 1再生, 2一時停止
    def DFP_get_play_state(self, 引数_devNo):
        cmd = f"DST:{引数_devNo:01x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # 全トラック数取得
    def DFP_get_file_count(self, 引数_devNo):
        cmd = f"DTC:{引数_devNo:01x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # イコライザー設定：0〜5
    # 0: Normal, 1: Pop, 2: Rock, 3: Jazz, 4: Classic, 5: Bass
    def DFP_set_eq(self, 引数_devNo, eq_mode):
        cmd = f"DEF:{引数_devNo:01x}:{eq_mode:02x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

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