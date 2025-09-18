
#============================================================
# Python共通：ＵＡＲＴ接続アダプタ
#------------------------------------------------------------
# ＭＭＰシリアルコマンドを直接扱うコア処理のスケルトン
# 実際にプロダクト別のアダプタを配置しないと、ここで
# エラーをライズさせる。
#------------------------------------------------------------
# [インストール方法]
# １．PYTHONPASTH(推奨)か、プロジェクトと動ディレクトリに格納
#============================================================
class MmpAdapterBase:

    # 接続情報を初期化する。
    _uart           = None
    _is_open        = False
    _connected_port = None
    _connected_baud = None
    _lastError      = None
    
    #------------------------------------------------------------
    # 通信関連
    #------------------------------------------------------------
    # ボーレート指定でポートを開く。
    def open_baud(self, baud: int) -> bool  : raise NotImplementedError

    # ポートが開いている場合は閉じる。
    def close(self) -> None                 : raise NotImplementedError

    # バッファをクリアする。
    def clear_input(self) -> None           : raise NotImplementedError

    # ASCII文字列を書き込む。
    def write_ascii(self, s: str) -> None   : raise NotImplementedError

    # 読み取れた1文字を返す。失敗は Noneを返す。
    def read_one_char(self)                 : raise NotImplementedError

    # 出力をクリアする。
    def flush(self) -> None                 : pass

    #------------------------------------------------------------
    # ヘルパー
    #------------------------------------------------------------
    # 時間調整する。
    def sleep_ms(self, ms: int) -> None     : raise NotImplementedError

    # 現在時間をミリ秒で返す。
    def now_ms(self) -> int                 : raise NotImplementedError

    # 経過時間をミリ秒で返す
    def ticks_diff(self, now_ms: int, start_ms: int) -> int:
        return int(now_ms - start_ms)
