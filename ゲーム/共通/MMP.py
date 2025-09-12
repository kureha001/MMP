#============================================================
# Python共通：ＭＭＰ接続
#------------------------------------------------------------
# アプリが本プリグラムを利用してＭＭＰに接続することで、
# ＡＰＩライブラリによる操作ができるようになる。
#------------------------------------------------------------
# [インストール方法]
# １．PYTHONPASTH(推奨)か、プロジェクトと動ディレクトリに格納
#============================================================
from mmp_core    import MmpClient
from mmp_adapter import MmpAdapter

#引数指定の場合、各プラットフォームのアダプタを確認のこと。。
#・CPython      ： port= None, preferred_ports=None
#・MicroPython  ： uart_id=0, tx_pin=0, rx_pin=1
#・CircuitPython： tx_pin=None, rx_pin=None, timeout_s=0.05, buffer_size=128
接続 = MmpClient(MmpAdapter())

__all__ = ["MmpClient", "MmpAdapter"]

#────────────────────────────────────    
def 通信接続():
    #┬
    print("<< ＭＭＰライブラリ for Python>>")
    print("")
    print("接続中...")
    #│
    if not 接続.ConnectAutoBaud():
    #if not 接続.ConnectWithBaud(115200):
        print("ＭＭＰとの接続に失敗しました...")
        return False
    #│
    if 接続.IsOpen is not None:
        print(f"　・通信ポート　: {接続.ConnectedPort}")
    print(f"　・通信速度　　: {接続.ConnectedBaud}bps")
    print( "　・バージョン  : {}".format(接続.Info.Version()))
    print( "　・PCA9685 [0] : 0x{:04X}".format(接続.Info.Dev.Pwm(0)))
    print( "　・DFPlayer[1] : 0x{:04X}".format(接続.Info.Dev.Audio(1)))

    return True