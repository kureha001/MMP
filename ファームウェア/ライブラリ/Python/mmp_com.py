# -*- coding: utf-8 -*-
# filename : mmp_com.py
#============================================================
# ＭＭＰコマンド：ＭＰ３プレイヤー
# バージョン：0.5
#------------------------------------------------------------
# [インストール方法]
# ・ＰＣ：[PYTHONPASTH] ※環境変数をセットしておく
# ・マイコン：[LIB]
# ・プロジェクトと同一ディレクトリ
#============================================================
def _DECtoHEX1(v): return f"{v & 0xF:01X}"
def _DECtoHEX2(v): return f"{v & 0xFF:02X}"
def _DECtoHEX3(v): return f"{v & 0x3FF:03X}"
def _DECtoHEX4(v): return f"{v & 0xFFFF:04X}"

def _HEX4toDEC(s: str):
    if not _is_five_bang(s) : return False, -1
    try                     : return True , int(s[:4], 16)
    except Exception        : return False, -1

def _is_five_bang(s: str) -> bool:
    return isinstance(s, str) and len(s) == 5 and s.endswith('!')