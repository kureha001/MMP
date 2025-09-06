
# mmp_core.py
# Unified layered API for CPython / MicroPython / CircuitPython.
# Mirrors the structure used in C# and Arduino:
#   Info, Analog, Pwm, Audio(Play/Read), Digital, I2c.
#
# Transport is provided by an Adapter (see mmp_adapter_*.py)

from mmp_adapter_base import MmpAdapterBase

# Default baud candidates (match firmware presets)
BAUD_CANDIDATES = (115200, 9600, 230400, 921600)

def _hex1(v): return f"{v & 0xF:01X}"
def _hex2(v): return f"{v & 0xFF:02X}"
def _hex3(v): return f"{v & 0x3FF:03X}"
def _hex4(v): return f"{v & 0xFFFF:04X}"

def _is_five_bang(s: str) -> bool:
    return isinstance(s, str) and len(s) == 5 and s.endswith('!')

def _try_parse_hex4_bang(s: str):
    if not _is_five_bang(s):
        return False, 0
    try:
        return True, int(s[:4], 16)
    except Exception:
        return False, 0

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

def _resolve(v, fb): return v if (isinstance(v, int) and v > 0) else fb

class MmpClient:
    """Layered MMP client over a platform adapter."""
    def __init__(self, adapter: MmpAdapterBase):
        if adapter is None:
            raise ValueError("adapter is required")
        self.adapter = adapter
        self.Settings = Settings()
        self._is_open = False
        self._last_error = ""
        self._connected_baud = None

        # modules
        self.Info   = self._InfoModule(self)
        self.Analog = self._AnalogModule(self)
        self.Pwm    = self._PwmModule(self)
        self.Audio  = self._AudioModule(self)
        self.Digital= self._DigitalModule(self)
        self.I2c    = self._I2cModule(self)

    # ---------------- properties ----------------
    @property
    def IsOpen(self): return self._is_open

    @property
    def ConnectedBaud(self): return self._connected_baud

    @property
    def LastError(self): return self._last_error

    # ---------------- connect/close ----------------
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

    def ConnectAutoBaud(self, candidates = BAUD_CANDIDATES) -> bool:
        for b in candidates:
            if self.ConnectWithBaud(b, 0, 0):
                self.Settings.BaudRate = b   
                return True
        self._last_error = "No baud matched"
        return False

    def Close(self):
        try:
            self.adapter.close()
        finally:
            self._is_open = False

    # ---------------- low-level ----------------
    def _verify(self, timeout_ms: int) -> bool:
        resp = self._send_command("VER!", _resolve(timeout_ms, self.Settings.TimeoutVerify))
        return len(resp) == 5 and resp.endswith('!')  # accept "0304!" or "!!!!!"

    def _send_command(self, cmd: str, timeout_ms: int) -> str:
        if not self._is_open and not cmd.startswith("VER"):
            # allow verify probe on just-opened port
            pass
        if not cmd or not isinstance(cmd, str):
            self._last_error = "Empty command"
            return ""
        if not cmd.endswith('!'):
            cmd += '!'
        # write & collect exactly 5 chars (split proof)
        self.adapter.clear_input()
        self.adapter.write_ascii(cmd)
        self.adapter.flush()

        resp_chars = []
        start = self.adapter.now_ms()
        while len(resp_chars) < 5 and self.adapter.ticks_diff(self.adapter.now_ms(), start) < int(timeout_ms):
            ch = self.adapter.read_one_char()
            if ch is not None:
                resp_chars.append(ch)
            else:
                self.adapter.sleep_ms(1)
        if len(resp_chars) == 5 and resp_chars[-1] == '!':
            return "".join(resp_chars)
        self._last_error = "Timeout or invalid 5-char reply"
        return ""

    # =========================================================
    # Modules
    # =========================================================

    class _InfoModule:
        def __init__(self, p:'MmpClient'): self._p = p; self.Dev = self._DevModule(p)
        def Version(self, timeoutMs: int = 0) -> str:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutGeneral)
            return self._p._send_command("VER!", t)
        class _DevModule:
            def __init__(self, p:'MmpClient'): self._p = p
            def Pwm(self, deviceId:int, timeoutMs:int=0) -> int:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
                resp = self._p._send_command(f"PWX:{_hex2(deviceId)}!", t)
                ok, v = _try_parse_hex4_bang(resp); 
                return v if ok else -1
            def Audio(self, id1to4:int, timeoutMs:int=0) -> int:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DPX:{_hex2(id1to4)}!", t)
                ok, v = _try_parse_hex4_bang(resp); 
                return v if ok else -1

    class _AnalogModule:
        def __init__(self, p:'MmpClient'): self._p = p
        def Configure(self, players:int, switches:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAnalog)
            if not (1 <= players <= 16) or not (1 <= switches <= 4):
                return False
            resp = self._p._send_command(f"ANS:{_hex2(players)}:{_hex2(switches)}!", t)
            return resp == "!!!!!"
        def Update(self, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAnalog)
            resp = self._p._send_command("ANU!", t)
            return resp == "!!!!!"
        def Read(self, playerIndex0to15:int, switchIndex0to3:int, timeoutMs:int=0) -> int:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAnalog)
            if not (0 <= playerIndex0to15 <= 15) or not (0 <= switchIndex0to3 <= 3):
                return -1
            resp = self._p._send_command(f"ANR:{_hex2(playerIndex0to15)}:{_hex2(switchIndex0to3)}!", t)
            ok, v = _try_parse_hex4_bang(resp); 
            return v if ok else -1

    class _PwmModule:
        def __init__(self, p:'MmpClient'): self._p = p
        def Info(self, deviceId:int, timeoutMs:int=0) -> int:
            return self._p.Info.Dev.Pwm(deviceId, timeoutMs)
        def Out(self, ch:int, value0to4095:int, timeoutMs:int=0) -> str:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            return self._p._send_command(f"PWM:{_hex2(ch)}:{_hex4(value0to4095)}!", t)
        def AngleInit(self, aMin:int, aMax:int, pMin:int, pMax:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            resp = self._p._send_command(f"PWI:{_hex3(aMin)}:{_hex3(aMax)}:{_hex4(pMin)}:{_hex4(pMax)}!", t)
            return resp == "!!!!!"
        def AngleOut(self, ch:int, angle0to180:int, timeoutMs:int=0) -> str:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutPwm)
            return self._p._send_command(f"PWA:{_hex2(ch)}:{_hex3(angle0to180)}!", t)

    class _AudioModule:
        def __init__(self, p:'MmpClient'):
            self._p = p
            self.Play = self._PlayModule(p)
            self.Read = self._ReadModule(p)
        def Info(self, id1to4:int, timeoutMs:int=0) -> int:
            return self._p.Info.Dev.Audio(id1to4, timeoutMs)
        def Volume(self, dev:int, vol0to30:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
            resp = self._p._send_command(f"VOL:{_hex2(dev)}:{_hex2(vol0to30)}!", t); 
            return resp == "!!!!!"
        def SetEq(self, dev:int, eq0to5:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
            resp = self._p._send_command(f"DEF:{_hex2(dev)}:{_hex2(eq0to5)}!", t); 
            return resp == "!!!!!"
        class _PlayModule:
            def __init__(self, p:'MmpClient'): self._p = p
            def FolderTrack(self, dev:int, folder:int, track:int, timeoutMs:int=0) -> bool:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DIR:{_hex2(dev)}:{_hex2(folder)}:{_hex2(track)}!", t)
                return resp == "!!!!!"
            def Stop(self, dev:int, timeoutMs:int=0) -> bool:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DSP:{_hex2(dev)}!", t)
                return resp == "!!!!!"
            def Pause(self, dev:int, timeoutMs:int=0) -> bool:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DPA:{_hex2(dev)}!", t)
                return resp == "!!!!!"
            def Resume(self, dev:int, timeoutMs:int=0) -> bool:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DPR:{_hex2(dev)}!", t)
                return resp == "!!!!!"
            def SetLoop(self, dev:int, onOff:int, timeoutMs:int=0) -> bool:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DLP:{_hex2(dev)}:{_hex1(1 if onOff else 0)}!", t)
                return resp == "!!!!!"
        class _ReadModule:
            def __init__(self, p:'MmpClient'): self._p = p
            def PlayState(self, dev:int, timeoutMs:int=0) -> int:
                return self._dst_query(dev, 1, timeoutMs)
            def Volume(self, dev:int, timeoutMs:int=0) -> int:
                return self._dst_query(dev, 2, timeoutMs)
            def Eq(self, dev:int, timeoutMs:int=0) -> int:
                return self._dst_query(dev, 3, timeoutMs)
            def FileCounts(self, dev:int, timeoutMs:int=0) -> int:
                return self._dst_query(dev, 4, timeoutMs)
            def CurrentFileNumber(self, dev:int, timeoutMs:int=0) -> int:
                return self._dst_query(dev, 5, timeoutMs)
            def _dst_query(self, dev:int, kind:int, timeoutMs:int) -> int:
                t = _resolve(timeoutMs, self._p.Settings.TimeoutAudio)
                resp = self._p._send_command(f"DST:{_hex2(dev)}:{_hex2(kind)}!", t)
                ok, v = _try_parse_hex4_bang(resp); 
                return v if ok else -1

    class _DigitalModule:
        def __init__(self, p:'MmpClient'): self._p = p
        def In(self, portId:int, timeoutMs:int=0) -> int:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutDigital)
            resp = self._p._send_command(f"POR:{_hex2(portId)}!", t)
            ok, v = _try_parse_hex4_bang(resp); 
            return v if ok else 0
        def Out(self, portId:int, value0or1:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutDigital)
            bit = '1' if (value0or1 & 1) else '0'
            resp = self._p._send_command(f"POW:{_hex2(portId)}:{bit}!", t)
            return resp == "!!!!!"

    class _I2cModule:
        def __init__(self, p:'MmpClient'): self._p = p
        def Write(self, addr:int, reg:int, value:int, timeoutMs:int=0) -> bool:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutI2c)
            resp = self._p._send_command(f"I2W:{_hex2(addr)}:{_hex2(reg)}:{_hex2(value)}!", t)
            return resp == "!!!!!"
        def Read(self, addr:int, reg:int, timeoutMs:int=0) -> int:
            t = _resolve(timeoutMs, self._p.Settings.TimeoutI2c)
            resp = self._p._send_command(f"I2R:{_hex2(addr)}:{_hex2(reg)}!", t)
            ok, v = _try_parse_hex4_bang(resp); 
            return v if ok else 0
