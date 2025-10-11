# -*- coding: utf-8 -*-
# filename : mmp_core_COM.py
#============================================================
# ＭＭＰコマンド：ＭＰ３プレイヤー
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
def _hex1(v): return f"{v & 0xF:01X}"
def _hex2(v): return f"{v & 0xFF:02X}"
def _hex3(v): return f"{v & 0x3FF:03X}"
def _hex4(v): return f"{v & 0xFFFF:04X}"

def _is_five_bang(s: str) -> bool:
    return isinstance(s, str) and len(s) == 5 and s.endswith('!')

def _try_parse_hex4_bang(s: str):
    if not _is_five_bang(s) : return False, 0
    try                     : return True , int(s[:4], 16)
    except Exception        : return False, 0

def _resolve(v, fb): return v if (isinstance(v, int) and v > 0) else fb

# ==== Common encoding helpers (added) ====
def enc_ch_end(toCh: int) -> str:
    try:
        v = int(toCh)
    except Exception:
        return "FF"
    return "FF" if v == -1 else f"{v & 0xFF:02X}"

def enc_opt_u16(val: int) -> str:
    try:
        v = int(val)
    except Exception:
        return "FFFF"
    return "FFFF" if v == -1 else f"{v & 0xFFFF:04X}"

def enc_deg3(deg: int) -> str:
    try:
        v = int(deg)
    except Exception:
        v = 0
    return f"{v & 0xFFF:03X}"

def enc_pwm4(pwm: int) -> str:
    try:
        v = int(pwm)
    except Exception:
        v = 0
    return f"{v & 0xFFFF:04X}"

def enc_speed2(percent: int, fw_max: int | None = None) -> str:
    try:
        p = int(percent)
    except Exception:
        p = 0
    plus = p + 100
    upper = 255 if fw_max is None else int(fw_max)
    if plus < 0: plus = 0
    if plus > upper: plus = upper
    return f"{plus & 0xFF:02X}"