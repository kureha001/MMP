#====================================================== 
# MECH HYNTER用 テストプログラム
#====================================================== 
import os
import time
import sys; sys.path.append('..'); import 共通.mmpRottenmeier

#○MMPを実体化する。
MMP = 共通.mmpRottenmeier.mmp()

#───────────────────────────    
def 命中確認():

    print("--------")
    print("命中確認")
    print("--------")

    繰返回数 = 100    # アドレス切替回数
    待時間   = 0.05  # ウェイト(秒)

    MMP.アナログ設定(
        1, # 使用するHC4067の個数(1～4)
        2, # 使用するHC4067のPin数(1～16)
        50 # アナログ値の丸め(この数値以下は切り捨て)
        )

    # 電源ON
    for 装置No in range(2): MMP.PWM_VALUE(装置No,4095)
    time.sleep(3)

    # アナログ入力をスキャン
    for 回数 in range(繰返回数):        
        MMP.アナログ読取()
        if 待時間 > 0 : time.sleep(待時間)
        print("  %03i" % 回数,":", MMP.mmpAnaVal)

    # 電源OFF
    for 装置No in range(2): MMP.PWM_VALUE(装置No,-1)

#───────────────────────────    
def 電源供給():

    print("--------")
    print("電源供給")
    print("--------")

    print("-> 電源ＯＮ")
    for 装置No in range(2): MMP.PWM_VALUE(装置No,4095)
    time.sleep(5)

    print("-> 電源ＯＦＦ")
    for 装置No in range(2): MMP.PWM_VALUE(装置No,-1)

#====================================================== 
# メイン処理
#====================================================== 
menu = {
    "1": ("カモロボの電源"      , 電源供給),
    "2": ("カモロボの命中を判定", 命中確認),
    }

def clear():
    os.system("cls" if os.name == "nt" else "clear")

def main():
    #┬
    #○MMPを接続する。
    MMP.通信接続_自動()
    #│
    #◎メニュー
    while True:
        clear()
        print("=== メニュー ===")
        for key in sorted(menu, key=lambda x: int(x)):
            print(f"{int(key):02d}. {menu[key][0]}")
        print("00. 終了")

        choice = input("番号を入力してください: ").strip()
        if choice == "0":
            print("終了します。")
            MMP.通信切断()
            break

        choice_key = choice.lstrip("0") or "0"
        if choice_key in menu:
            clear()
            menu[choice_key][1]()  # 関数実行
            print("\nEnterキーでメニューに戻ります。")
            input()
        else:
            print("無効な入力です。Enterキーで続行。")
            input()

if __name__ == "__main__": main()
