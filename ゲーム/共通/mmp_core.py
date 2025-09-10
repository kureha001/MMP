# mmp_core.py
#============================================================
# Python共通：ＭＭＰコア機能
#------------------------------------------------------------
# ＭＭＰシリアルコマンドを直接扱うコア処理
#============================================================

from mmp_adapter_base import MmpAdapterBase

#=========================
# 通信速度のプリセット一覧
#=========================
BAUD_CANDIDATES = ( 9600, 115200, 230400, 921600 )

#========================
# ヘルパ
#========================
# HEX値変換ヘルパ(個別桁)
def _hex1(v): return f"{v & 0xF:01X}"
def _hex2(v): return f"{v & 0xFF:02X}"
def _hex3(v): return f"{v & 0x3FF:03X}"
def _hex4(v): return f"{v & 0xFFFF:04X}"

# "!!!!!"判定ヘルパ
def _is_five_bang(s: str) -> bool:
    return isinstance(s, str) and len(s) == 5 and s.endswith('!')

# HEX値変換ヘルパ(4桁)
def _try_parse_hex4_bang(s: str):
    if not _is_five_bang(s) : return False, 0
    try                     : return True , int(s[:4], 16)
    except Exception        : return False, 0

#============================
#===== 規定値モジュール =====
#============================
class Settings:
    def __init__(self):
        self.BaudRate       = 115200
        self.TimeoutIo      = 200
        self.TimeoutVerify  = 400
        self.TimeoutGeneral = 400
        self.TimeoutAnalog  = 400
        self.TimeoutPwm     = 400
        self.TimeoutAudio   = 400
        self.TimeoutDigital = 400
        self.TimeoutI2c     = 400
        self.AnalogBits     = 10

# ？？？これは何？？？
def _resolve(v, fb): return v if (isinstance(v, int) and v > 0) else fb


#========================
# メイン
#========================
class MmpClient:
    # コンストラクタ
    def __init__(self, adapter: MmpAdapterBase):
        if adapter is None: raise ValueError("adapter is required")

        self.adapter         = adapter
        self.Settings        = Settings()
        self._is_open        = False
        self._last_error     = ""
        self._connected_baud = None

        #========================
        #==== モジュール実装 ====
        #========================
        self.Info   = self._InfoModule(self)
        self.Analog = self._AnalogModule(self)
        self.Pwm    = self._PwmModule(self)
        self.Audio  = self._AudioModule(self)
        self.Digital= self._DigitalModule(self)
        self.I2c    = self._I2cModule(self)

    #----------------
    #---- 情報系 ----
    #----------------
    @property
    def IsOpen(self): return self._is_open

    @property
    def ConnectedBaud(self): return self._connected_baud

    @property
    def LastError(self): return self._last_error

    #=============================
    #===== 低レイヤ コマンド =====
    #=============================
    #-------------------
    # 接続：条件指定
    #-------------------
    def ConnectWithBaud(self, baud: int, timeoutIo: int = 0, verifyTimeoutMs: int = 0) -> bool:

        use_io = _resolve(timeoutIo, self.Settings.TimeoutIo)
        use_ver= _resolve(verifyTimeoutMs, self.Settings.TimeoutVerify)

        try:
            if not self.adapter.open_baud(int(baud)):
                self._last_error = "Open failed"
                return False

            # small settle + drain
            self.adapter.sleep_ms(10)
            self.adapter.clear_input()

            if not self._verify(use_ver):
                self._last_error = "Verify failed"
                self.adapter.close()
                self._is_open = False
                return False

            self._is_open = True
            self._connected_baud = self.adapter.connected_baud or int(baud)
            self.Settings.BaudRate = int(self._connected_baud)
            return True

        except Exception as ex:
            self._last_error = str(ex)
            try: self.adapter.close()
            except: pass
            self._is_open = False
            return False

    #-------------------
    # 接続：自動
    #-------------------
    def ConnectAutoBaud(self, candidates = BAUD_CANDIDATES) -> bool:
        for b in candidates:
            if self.ConnectWithBaud(b, 0, 0): return True
        self._last_error = "No baud matched"
        return False

    #-------------------
    # 切断
    #-------------------
    def Close(self):
        try    : self.adapter.close()
        finally: self._is_open = False

    #-------------------
    # バージョンチェック
    #-------------------
    def _verify(self, timeout_ms: int) -> bool:
        resp = self._send_command("VER!", _resolve(timeout_ms, self.Settings.TimeoutVerify))
        return len(resp) == 5 and resp.endswith('!')  # accept "0304!" or "!!!!!"

    #-------------------
    # ＭＭＰシリアル通信
    #-------------------
    def _send_command(self, cmd: str, timeout_ms: int) -> str:

        if not cmd or not isinstance(cmd, str):
            self._last_error = "Empty command"
            return ""

        if not cmd.endswith('!'):
            cmd += '!'

        self.adapter.clear_input()
        self.adapter.write_ascii(cmd)
        self.adapter.flush()

        resp_chars = []
        start = self.adapter.now_ms()

        while len(resp_chars) < 5 and self.adapter.ticks_diff(self.adapter.now_ms(), start) < int(timeout_ms):

            ch = self.adapter.read_one_char()

            if ch is not None: resp_chars.append(ch)
            else             : self.adapter.sleep_ms(1)

        if len(resp_chars) == 5 and resp_chars[-1] == '!': return "".join(resp_chars)

        self._last_error = "Timeout or invalid 5-char reply"
        return ""

    #=============================
    #=====（公開用）階層化API=====
    #=============================
    # -----------------------
    # --- 情報モジュール ----
    # -----------------------
    class _InfoModule:
        # ----------------------
        # コンストラクタ
        # ----------------------
        def __init__(self, p:'MmpClient'): self._p = p; self.Dev = self._DevModule(p)

        # ----------------------
        # １．バージョン
        # ----------------------
        def Version(self, timeoutMs: int = 0) -> str:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutGeneral)
            return self._p._send_command("VER!", t)

        # ----------------------
        # ２．デバイス情報
        # ----------------------
        class _DevModule:
            # ---------------------------
            # 2-0.コンストラクタ
            # ---------------------------
            def __init__(self, p:'MmpClient'): self._p = p

            # ---------------------------
            # 2-1.ＰＷＭデバイス(PCA9685)
            # ---------------------------
            def Pwm(self, devId0to15:int, timeoutMs:int=0) -> int:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
                resp = self._p._send_command(f"PWX:{_hex2(devId0to15)}!", t)
                ok, v = _try_parse_hex4_bang(resp); 
                return v if ok else -1

            # ---------------------------
            # 2-2.音声デバイス(DFPlayer)
            # ---------------------------
            def Audio(self, devId1to4:int, timeoutMs:int=0) -> int:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DPX:{_hex2(devId1to4)}!", t)
                ok, v = _try_parse_hex4_bang(resp); 
                return v if ok else -1

    # -----------------------------
    # ---- アナログ モジュール ----
    # -----------------------------
    class _AnalogModule:
        # ----------------------
        # コンストラクタ
        # ----------------------
        def __init__(self, p:'MmpClient'): self._p = p

        # ------------------
        # --- 内部ヘルパ ---
        # ------------------
        def _clamp_bits(self, bits:int) -> int:
            ub = bits if (isinstance(bits, int) and 1 <= bits <= 16) else self._p.Settings.AnalogBits
            if not (1 <= ub <= 16): ub = 10
            return ub
        # ----------------------
        def _clamp_raw(self, raw:int, bits:int) -> int:
            if raw < 0: return raw
            maxv = (1 << bits) - 1
            return 0 if raw < 0 else (maxv if raw > maxv else raw)
        # ----------------------
        def _round_mid(self, raw:int, step:int, bits:int) -> int:
            if raw < 0 or step <= 0: return raw
            maxv = (1 << bits) - 1
            v = raw if raw <= maxv else maxv
            q, r = divmod(v, step)
            mid = step // 2
            # 中央値の扱い：偶数stepは r>=mid で切り上げ、奇数stepは r<=mid で切り捨て（= r>=mid+1 で切り上げ）
            if r < mid: v2 = q * step
            elif r > mid or (step % 2 == 0 and r == mid): v2 = (q + 1) * step
            else: v2 = q * step
            if v2 > maxv: v2 = maxv
            return v2
        # ----------------------
        def _round_up(self, raw:int, step:int, bits:int) -> int:
            if raw < 0 or step <= 0: return raw
            maxv = (1 << bits) - 1
            v = raw if raw <= maxv else maxv
            q, r = divmod(v, step)
            v2 = v if r == 0 else (q + 1) * step
            if v2 > maxv: v2 = maxv
            return v2
        # ----------------------
        def _round_down(self, raw:int, step:int, bits:int) -> int:
            if raw < 0 or step <= 0: return raw
            maxv = (1 << bits) - 1
            v = raw if raw <= maxv else maxv
            q = v // step
            v2 = q * step
            if v2 < 0: v2 = 0
            return v2
        # ----------------------

        # -----------------------
        # １．使用範囲設定
        # -----------------------
        def Configure(self, hc4067chTtl:int, hc4067devTtl:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAnalog)
            if not (1 <= hc4067chTtl <= 16) or not (1 <= hc4067devTtl <= 4):
                return False
            resp = self._p._send_command(f"ANS:{_hex2(hc4067chTtl)}:{_hex2(hc4067devTtl)}!", t)
            return resp == "!!!!!"

        # -----------------------
        # ２．測定値バッファ更新
        # -----------------------
        def Update(self, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAnalog)
            resp = self._p._send_command("ANU!", t)
            return resp == "!!!!!"

        # ---------------------------------
        # ３．測定値バッファ読取（丸めなし）
        # ---------------------------------
        def Read(self, hc4067ch0to15:int, hc4067dev0to3:int, timeoutMs:int=0) -> int:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAnalog)
            if not (0 <= hc4067ch0to15 <= 15) or not (0 <= hc4067dev0to3 <= 3):
                return -1
            resp = self._p._send_command(f"ANR:{_hex2(hc4067ch0to15)}:{_hex2(hc4067dev0to3)}!", t)
            ok, v = _try_parse_hex4_bang(resp); 
            return v if ok else -1

        # -----------------------------------------
        # ４．測定値バッファ読取（中央値基準の丸め）
        # -----------------------------------------
        def ReadRound(self, hc4067ch0to15:int, hc4067dev0to3:int, step:int, bits:int=0, timeoutMs:int=0) -> int:
            raw = self.Read(hc4067ch0to15, hc4067dev0to3, timeoutMs)
            if raw < 0: return raw
            ub = self._clamp_bits(bits)
            return self._round_mid(raw, step, ub)

        # -----------------------------------------
        # ５．測定値バッファ読取（切り上げ丸め）
        # -----------------------------------------
        def ReadRoundUp(self, hc4067ch0to15:int, hc4067dev0to3:int, step:int, bits:int=0, timeoutMs:int=0) -> int:
            raw = self.Read(hc4067ch0to15, hc4067dev0to3, timeoutMs)
            if raw < 0: return raw
            ub = self._clamp_bits(bits)
            return self._round_up(raw, step, ub)

        # -----------------------------------------
        # ６．測定値バッファ読取（切り捨て丸め）
        # -----------------------------------------
        def ReadRoundDown(self, hc4067ch0to15:int, hc4067dev0to3:int, step:int, bits:int=0, timeoutMs:int=0) -> int:
            raw = self.Read(hc4067ch0to15, hc4067dev0to3, timeoutMs)
            if raw < 0: return raw
            ub = self._clamp_bits(bits)
            return self._round_down(raw, step, ub)

    # -----------------------------
    # ---- デジタル モジュール ----
    # -----------------------------
    class _DigitalModule:
        # ----------------------
        # コンストラクタ
        # ----------------------
        def __init__(self, p:'MmpClient'): self._p = p

        # ---------
        # １．入力
        # ---------
        def In(self, gpioId:int, timeoutMs:int=0) -> int:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutDigital)
            resp = self._p._send_command(f"POR:{_hex2(gpioId)}!", t)
            ok, v = _try_parse_hex4_bang(resp); 
            return v if ok else 0

        # ---------
        # ２．出力
        # ---------
        def Out(self, gpioId:int, val0or1:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutDigital)
            bit = '1' if (val0or1 & 1) else '0'
            resp = self._p._send_command(f"POW:{_hex2(gpioId)}:{bit}!", t)
            return resp == "!!!!!"

    # --------------------------
    # ---- ＰＷＭモジュール ----
    # --------------------------
    class _PwmModule:
        # ----------------------
        # コンストラクタ
        # ----------------------
        def __init__(self, p:'MmpClient'): self._p = p

        # ---------------------
        # １．デバイス情報
        # ---------------------
        def Info(self, devId0to15:int, timeoutMs:int=0) -> int:
            return self._p.Info.Dev.Pwm(devId0to15, timeoutMs)

        # ---------------------
        # ２．出力（boolに変更）
        # ---------------------
        def Out(self, chId0to255:int, val0to4095:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            resp = self._p._send_command(f"PWM:{_hex2(chId0to255)}:{_hex4(val0to4095)}!", t)
            return resp == "!!!!!"

        # ---------------------
        # ３．角度設定
        # ---------------------
        def AngleInit(self, angleMin:int, angleMax:int, pwmMin:int, pwmMax:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            resp = self._p._send_command(f"PWI:{_hex3(angleMin)}:{_hex3(angleMax)}:{_hex4(pwmMin)}:{_hex4(pwmMax)}!", t)
            return resp == "!!!!!"

        # -------------------------
        # ４．角度指定（boolに変更）
        # -------------------------
        def AngleOut(self, chId0to255:int, angle0to180:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            resp = self._p._send_command(f"PWA:{_hex2(chId0to255)}:{_hex3(angle0to180)}!", t)
            return resp == "!!!!!"

    # ------------------------
    # ---- 音声モジュール ----
    # ------------------------
    class _AudioModule:
        # ----------------------
        # コンストラクタ
        # ----------------------
        def __init__(self, p:'MmpClient'):
            self._p = p
            self.Play = self._PlayModule(p)
            self.Read = self._ReadModule(p)

        # ----------------------
        # １. 単独コマンド
        # ----------------------
        # 1-1. デバイス情報
        # ----------------------
        def Info(self, devId1to4:int, timeoutMs:int=0) -> int:
            return self._p.Info.Dev.Audio(devId1to4, timeoutMs)

        # ----------------------
        # 1-2. 音量設定
        # ----------------------
        def Volume(self, devId1to4:int, vol0to30:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
            resp = self._p._send_command(f"VOL:{_hex2(devId1to4)}:{_hex2(vol0to30)}!", t) 
            return resp == "!!!!!"

        # ----------------------
        # 1-3.イコライザ設定
        # ----------------------
        def SetEq(self, devId1to4:int, mode0to5:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
            resp = self._p._send_command(f"DEF:{_hex2(devId1to4)}:{_hex2(mode0to5)}!", t) 
            return resp == "!!!!!"

        # ----------------------
        # ２. 再生サブモジュール
        # ----------------------
        class _PlayModule:
            # ----------------------
            # コンストラクタ
            # ----------------------
            def __init__(self, p:'MmpClient'): self._p = p

            # ----------------------
            # 2-1. 再生（Start）
            # ----------------------
            def Start(self, devId1to4:int, dir1to255:int, file1to255:int, timeoutMs:int=0) -> bool:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DIR:{_hex2(devId1to4)}:{_hex2(dir1to255)}:{_hex2(file1to255)}!", t)
                return resp == "!!!!!"

            # ----------------------
            # 2-2. ループ再生指定
            # ----------------------
            def SetLoop(self, devId1to4:int, enable:int, timeoutMs:int=0) -> bool:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DLP:{_hex2(devId1to4)}:{_hex1(1 if enable else 0)}!", t)
                return resp == "!!!!!"

            # ----------------------
            # 2-3. 停止
            # ----------------------
            def Stop(self, devId1to4:int, timeoutMs:int=0) -> bool:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DSP:{_hex2(devId1to4)}!", t)
                return resp == "!!!!!"

            # ----------------------
            # 2-4. 一時停止
            # ----------------------
            def Pause(self, devId1to4:int, timeoutMs:int=0) -> bool:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DPA:{_hex2(devId1to4)}!", t)
                return resp == "!!!!!"

            # ----------------------
            # 2-5. 再開
            # ----------------------
            def Resume(self, devId1to4:int, timeoutMs:int=0) -> bool:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DPR:{_hex2(devId1to4)}!", t)
                return resp == "!!!!!"

        # ----------------------
        # ３. 参照サブモジュール
        # ----------------------
        class _ReadModule:
            # ----------------------
            # コンストラクタ
            # ----------------------
            def __init__(self, p:'MmpClient'): self._p = p

            # ----------------------
            # 3-1. 再生状況
            # ----------------------
            def State(self, devId1to4:int, timeoutMs:int=0) -> int:
                return self._dst_query(devId1to4, 1, timeoutMs)

            # ----------------------
            # 3-2. ボリューム
            # ----------------------
            def Volume(self, devId1to4:int, timeoutMs:int=0) -> int:
                return self._dst_query(devId1to4, 2, timeoutMs)

            # ----------------------
            # 3-3. イコライザのモード
            # ----------------------
            def Eq(self, devId1to4:int, timeoutMs:int=0) -> int:
                return self._dst_query(devId1to4, 3, timeoutMs)

            # ----------------------
            # 3-4. 総ファイル総数
            # ----------------------
            def FileCounts(self, devId1to4:int, timeoutMs:int=0) -> int:
                return self._dst_query(devId1to4, 4, timeoutMs)

            # ----------------------
            # 3-5. 現在のファイル番号
            # ----------------------
            def FileNumber(self, devId1to4:int, timeoutMs:int=0) -> int:
                return self._dst_query(devId1to4, 5, timeoutMs)

            # ----------------------
            # 内部ヘルパ
            # ----------------------
            def _dst_query(self, devId1to4:int, kind:int, timeoutMs:int) -> int:
                t     = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp  = self._p._send_command(f"DST:{_hex2(devId1to4)}:{_hex2(kind)}!", t)
                ok, v = _try_parse_hex4_bang(resp); 
                return v if ok else -1

    # --------------------------
    # ---- Ｉ２Ｃモジュール ----
    # --------------------------
    class _I2cModule:
        # ----------------------
        # コンストラクタ
        # ----------------------
        def __init__(self, p:'MmpClient'): self._p = p

        # ----------------------
        # １．書き込み
        # ----------------------
        def Write(self, addr:int, reg:int, val:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutI2c)
            resp = self._p._send_command(f"I2W:{_hex2(addr)}:{_hex2(reg)}:{_hex2(val)}!", t)
            return resp == "!!!!!"

        # ----------------------
        # ２．読み出し
        # ----------------------
        def Read(self, addr:int, reg:int, timeoutMs:int=0) -> int:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutI2c)
            resp = self._p._send_command(f"I2R:{_hex2(addr)}:{_hex2(reg)}!", t)
            ok, v = _try_parse_hex4_bang(resp); 
            return v if ok else 0
