#====================================================== 
# ＭＭＰライブラリ Ver 0.03 Rottenmeier
#------------------------------------------------------
# Ver 0.03.004　2025/08/28 By Takanari.Kureha
#   - 通信接続()を通信接続()_指定に変更
#   - 通信接続()を通信接続()に変更
#======================================================
# [システム共通]
import serial

#=============
# 例外クラス
#=============
class ConnectException(Exception):
    print("ConnectException")

#=============
# 本体クラス
#=============
class mmp:
    #┬
    #○└┐初期化：
    def __init__(
            self,
            arg通信速度 = 115200    # 通信速度(単位：bps)
            ):
        #│
        #○接続する。
        print("\n<<接続準備>>")
        self.ser = serial.Serial()
        self.ser.baudrate   = arg通信速度
        self.接続済   = False
        self.version = ""
        #│
        #○アナログパラメータを初期化する
        self.参加総人数  = None
        self.スイッチ数  = None 
        self.丸め        = None
        #┴

    #=====================================================================
    # 接続処理
    #=====================================================================
    def 通信接続(self):
        print("\n接続中...")
        for i in range(100):
            com_str = 'COM%d'%i
            try:
                self.ser.port = com_str
                self.ser.open()
                break
            except:
                i = i
        if self.ser.port == 'COM99': raise ConnectException()
        self.version = self.バージョン()
        print(f"接続済み(Ver.{self.version})")
        self.接続済 = True
        return com_str
    #---------------------------------------------------------------------
    def 通信接続_指定(self, comm_num):
        try:
            self.ser.port = comm_num
            self.ser.open()
        except:
            raise ConnectException()
        self.version = self.バージョン()
        print(f"接続済み(Ver.{self.version})")
        self.接続済 = True
    #---------------------------------------------------------------------
    def 通信切断(self):
        print("\n<<切断>>")
        self.ser.close()
        self.接続済 = False
    #---------------------------------------------------------------------
    def バージョン(self):
        self.ser.write(b'VER!')
        data = self.ser.read(5)
        sdata = data.decode('ascii')
        結果 = f"{sdata[0]}.{sdata[1]}.{sdata[2:4]}"
        return 結果

    #=====================================================================
    # アナログ入力
    #=====================================================================
    #---------------------------------------------------------------------
    # 設定コマンド
    #---------------------------------------------------------------------
    #┬
    #○└┐測定データ：
    def アナログ初期化(self):
        #◎└┐1アドレス分の空データを作る。
        tmpスイッチ = [] 
        for cntMmpNo in range(self.スイッチ数):
            #○読取値の初期値(ゼロ)を追加する。
            tmpスイッチ.append(0)
            #┴
        #│
        #◎└┐全アドレスの測定データに空データをセットする。
        self.mmpAnaVal = [] 
        for cntAnaPin in range(self.参加総人数):
            #○測定データ(初期値)を追加する。
            self.mmpAnaVal.append(tmpスイッチ.copy())
    #┴　┴　┴
    #---------------------------------------------------------------------
    # 設定コマンド
    #---------------------------------------------------------------------
    #┬
    #○└┐アナログ測定：
    def アナログ設定(self  ,
            argスイッチ数 = 4, # 使用するHC4067の個数(1～4)
            arg参加人数   = 1, # 使用するHC4067のPin数(1～16)
            arg丸め       = 5  # アナログ値の丸め
            ):
        #│
        #○アナログパラメータをセットする
        self.参加総人数  = arg参加人数
        self.スイッチ数  = argスイッチ数 
        self.丸め        = int(arg丸め)
        #│
        print("<<アナログ入力準備>>")
        print(" ・HC4067 使用個数     : %i"  % self.スイッチ数  )
        print(" ・HC4067 使用ポート数 : %i"  % self.参加総人数  )
        print(" ・アナログ値の丸め処理: "   , self.丸め)
        #│
        #○測定データをゼロ初期する。
        self.アナログ初期化()
        #│
        data = f"ANS:{self.参加総人数:02X}:{self.スイッチ数:02X}!"
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
        #┴
    #---------------------------------------------------------------------
    # 読取コマンド
    #---------------------------------------------------------------------
    #┬
    #○└┐アナログ測定：
    def アナログ読取(self):
        data = "ANU!"
        self.ser.write(str.encode(data))
        data = self.ser.read(5)

        #◎└┐プレイヤー分を読み取る。
        for pl in range(self.参加総人数):

            #◎└┐スイッチ分を読み取る。
            for sw in range(self.スイッチ数):

                data = "ANR:%02X:%01X!" % (pl, sw)
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
    # ＰＷＭ(PCA9685)：機器情報（機器No.0～15）
    #  コマンド書式：PWX!<機器No.0～15>!
    #---------------------------------------------------------------------
    def PWM_Info(self, arg機器No):
        cmd = f"PWX:{arg機器No:01x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")
    #---------------------------------------------------------------------
    # ＰＷＭ(PCA9685)：接続状態確認（機器No.0～15）
    #  コマンド書式：PWS!<機器No.0～15>!
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
    def PWM_チャンネル使用可否(self, argCh通番):
        if not (0 <= argCh通番 <= 255):
            raise ValueError("[エラー] PWMチャンネルは 0〜255 の範囲で指定")
        pwm_id = argCh通番 // 16  # 0〜15 の PCA9685番号
        if len(self.PWM機器状況) != 16: return False
        return self.PWM機器状況[pwm_id]
    #---------------------------------------------------------------------
    # PWM出力値指定
    # コマンド書式：PWM:<Ch通番>:<PWM出力値>!
    #---------------------------------------------------------------------
    def PWM_VALUE(self, ch, pwm):
        cmd = f"PWM:{int(ch):02X}:{int(pwm):04X}!"
        self.ser.write(cmd.encode("ascii"))
        resp = self.ser.read(5)              # bytes
        return resp == b"!!!!!"              # 5バイトOK応答
    #---------------------------------------------------------------------
    # サーボ特性設定（角度↔PWM）
    # コマンド書式：PWI:<RS>:<RE>:<PS>:<PE>!
    #   RS/RE: 角度 0–360 (%03X)、PS/PE: PWM 0–4095 (%04X)
    #---------------------------------------------------------------------
    def PWM_INIT(self, rs, re, ps, pe):
        cmd = f"PWI:{int(rs):03X}:{int(re):03X}:{int(ps):04X}:{int(pe):04X}!"
        self.ser.write(cmd.encode("ascii"))
        resp = self.ser.read(5)
        return resp == b"!!!!!"
    #---------------------------------------------------------------------
    # 角度指定
    # コマンド書式：PWA:<Ch通番>:<角度>!
    #   角度は 0–180 を %03X
    #---------------------------------------------------------------------
    def PWM_ANGLE(self, ch, ang):
        cmd = f"PWA:{int(ch):02X}:{int(ang):03X}!"
        self.ser.write(cmd.encode("ascii"))
        resp = self.ser.read(5)
        return resp == b"!!!!!"

    #=====================================================================
    # デジタル入出力
    #=====================================================================
    #---------------------------------------------------------------------
    # 入出力(デジタル) ※出力後に入力する
    #---------------------------------------------------------------------
    def digital_IO(self, argPort, argValue):
        data = "IO:%02X:%01X!" % (argPort, argValue)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
        data2 = data.decode('utf-8')
        data3 = data2.replace('!', '')
        return int(data3, 16)
    #---------------------------------------------------------------------
    # 出力
    #---------------------------------------------------------------------
    def digital_OUT(self, argPort, argValue):
        cmd = "POW:%02X:%01X!" % (argPort, argValue)
        self.ser.write(cmd.encode("ascii"))       
        data = self.ser.read(5)

        try: response = data.decode("ascii")
        except UnicodeDecodeError: return False

        if   response == b"!!!!!": return True
        else                     : return False
    #---------------------------------------------------------------------
    # 入力
    #---------------------------------------------------------------------
    def digital_IN(self, argPort):
        data = "POR:%02X!" % (argPort)
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
            if( d == '!'): return s
            s += d

    #=====================================================================
    # ＭＰ３プレイヤー
    #=====================================================================
    # 機器情報
    def DFP_Info(self, 引数_devNo):
        cmd = f"DPX:{引数_devNo:01x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # 指定フォルダ内トラック再生
    def DFP_PlayFolderTrack(self, 引数_devNo, folder, track):
        cmd = f"DIR:{引数_devNo:01x}:{folder:02x}:{track:02x}!"
        self.ser.write(cmd.encode("ascii"))
        return self.ser.read(5).decode("ascii")

    # ステータス 1:MP3/2:音量/3:イコライザ/4:ファイル番号(総合計)/4:ファイル番号(フォルダ内)
    def DFP_get_play_state(self, 引数_devNo, 引数_stNo):
        cmd = f"DST:{引数_devNo:01x}:{引数_stNo:01x}!"
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
        data = "I2W:%02X:%02X:%02X:1!" % (slave_addr, addr, val)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
        if( data.decode('utf-8') != '----!'):
            return True
        else:
            return False
    #---------------------------------------------------------------------
    def i2cRead(self, slave_addr, addr):
        data = "I2R:%02X:%02X:1!" % (slave_addr, addr)
        self.ser.write(str.encode(data))
        data = self.ser.read(5)
        data2 = data.decode('utf-8')
        data3 = data2.replace('!', '')
        return int(data3, 16)
