#====================================================== 
# DFPlayer用 テストプログラム
#====================================================== 
import time
import mmpRottenmeier

MMP = mmpRottenmeier.mmp()

#───────────────────────────    
def MP3_00():

    print("------------")
    print("DFPlayer制御")
    print("------------")

    機器番号 = 1 # 1 or 2

    print("・機器情報")
    print("1台目：",MMP.DFP_Info(1))
    print("2台目：",MMP.DFP_Info(2))

    print("・ボリューム設定")
    print(MMP.DFP_Volume(機器番号,20))

    print("・１曲目を再生")
    print(MMP.DFP_Play(機器番号,1))
    time.sleep(5)

    print("・停止")
    print(MMP.DFP_Stop(機器番号))

def MP3_01():
    print("--------------------------")
    print("フォルダーのトラックを再生")
    print("--------------------------")
    機器番号 = 1 # 1 or 2
    print(MMP.DFP_PlayFolderTrackLoop(機器番号,2,5))


#====================================================== 
# メイン処理
#====================================================== 
menu = {
    "1": ("各種制御"                    , MP3_00),
    "2": ("フォルダーのトラックを再生"  , MP3_01)
    }

def clear():
    os.system("cls" if os.name == "nt" else "clear")

def main():
    #┬
    #○MMPを接続する。
    MMP.通信接続()
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