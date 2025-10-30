# filename : Network.py
# =========================================
# Ｎ層(ネットワーク層)
# Wifiによるネットワークを提供する。
# -----------------------------------------
#  Phase 2.8 : 
# =========================================

# ------------------------
# 設定ファイル読込（wifi.json）
# ------------------------
from    time import sleep_ms
import  network
import  ujson

import  Data    as D層  # D層(データベース層)
import  Presen  as P層  # P層(プレゼンテーション層)

# =========================================
# ユーティリティー：Wifi接続
# =========================================
# ------------------------
# 設定ファイル読込（wifi.json）
# ------------------------
def 設定ファイル読込開始():
    try:
        with open("wifi.json", "r") as f:
            cfg = ujson.loads(f.read())
        return cfg
    except Exception as e:
        print("　<< ERROR ファイル読込 >>")
        return None

# ------------------------
# Wi-Fi接続を試みる
# 引数：① 設定情報
# 戻値：① 合否
# ------------------------
def 接続開始(cfg):

    # Wi-Fi接続
    wlan = network.WLAN(network.STA_IF) # STAモード
    wlan.active(True)                   # 有効
    wlan.disconnect()                   # 切断
    try:
        wlan.connect(                   # Wifi接続
            cfg.get("ssid"    , ""),    # SSID
            cfg.get("password", ""),    # パスワード
            )
    except :
        print("　<< ERROR 接続認証 >>")
        return False

    # 接続完了待ち
    attempts = 0
    while attempts < 20:
        if wlan.isconnected(): break
        showConnectingDots(attempts + 1)
        sleep_ms(500)
        attempts += 1

    # 接続時間切れ
    if not wlan.isconnected():
        print("　<< ERROR 接続時間切れ >>")
        return False

    # 基本情報を取得
    D層.基本["名前"] = cfg.get("player", "")

    # ネット情報を取得
    ip, mask, gw, dns = wlan.ifconfig()
    D層.ネット["SSID"   ] = cfg.get("ssid", "")
    D層.ネット["IP"     ] = ip
    D層.ネット["SUBNET" ] = mask
    D層.ネット["GATEWAY"] = gw
    D層.ネット["DNS"    ] = dns

    # シリアル出力
    print("　SSID  :", D層.ネット["SSID"])
    print("　IP    :", D層.ネット["IP"  ])
    print("　PLAYER:", D層.基本["名前"]  )

    return True


# =========================================
# ユーティリティー：情報表示
# =========================================
# ------------------------
# 接続中ドット表示
# ------------------------
def showConnectingDots(count):
    dots = "." * min(count, 24)
    l1 = dots[0:8]
    l2 = dots[8:16]
    l3 = dots[16:24]
    P層.showRaw(l1, l2, l3)