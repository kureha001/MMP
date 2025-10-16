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
def _getValue(s: str):
    if not _is_five_bang(s) : return False, -1
    try                     : return True , int(s[:4])
    except Exception        : return False, -1

def _is_five_bang(s: str) -> bool:
    return isinstance(s, str) and len(s) == 5 and s.endswith('!')