#====================================================== 
# MECH TORNADO用 テストプログラム
#====================================================== 
import os
import time
import sys; sys.path.append('..'); import 共通.mmpRottenmeier

#○MMPを実体化する。
MMP = 共通.mmpRottenmeier.mmp()

#───────────────────────────    
def スイッチ():

    print("--------")
    print("スイッチ")
    print("--------")

    MMP.アナログ設定(
        4, # 使用するHC4067の個数(1～4)
        1, # 使用するHC4067のPin数(1～16)
        50 # アナログ値の丸め(この数値以下は切り捨て)
        )

    # 0:脚の屈伸
    # 1:手１
    # 2:手２
    # 3:ゲーム開始
    for cntLoop in range(100):        
        #◎└┐全アドレスから読み取る。
        MMP.アナログ読取()
        time.sleep(0.1)
        print("  %03i" % cntLoop,":", MMP.mmpAnaVal)

#────────────────────────────────────    
def 人形の脚():

    print("--------")
    print("人形の脚")
    print("--------")

    print("-> 屈伸(足上げ;DCモータON)")
    MMP.PWM_VALUE(0,4095) # 屈伸
    time.sleep(2)

    print("-> まっすぐ(足さげ;DCモータOFF)")
    MMP.PWM_VALUE(0,-1)   # 直立

#────────────────────────────────────    
def 鉄棒のロック():

    print("------------")
    print("鉄棒のロック")
    print("------------")

    print("-> 解除(サーボモータHIGH)")
    MMP.PWM_VALUE(1,160)
    time.sleep(2)

    print("-> ロック(サーボモータLOW)")
    MMP.PWM_VALUE(1,100)
    time.sleep(2)

    print("-> 解除(サーボモータHIGH)")
    MMP.PWM_VALUE(1,160)

#====================================================== 
# メイン処理
#====================================================== 
menu = {
    "1": ("コントローラのスイッチ類", スイッチ      ),
    "2": ("選手人形の脚を上げ下げ"  , 人形の脚      ),
    "3": ("鉄棒のロック/解除"       , 鉄棒のロック  ),
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
