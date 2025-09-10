#==============
# ラウンチャー
#==============
import Mech01_Bullets
import Mech02_Saurus
import Mech03_Hunter
import Mech04_Tornado

print("≪テスト メニュー≫")
print("－－－－－－－－－－－－－－－")
print("　１．ＭＥＣＨ　ＢＵＬＬＥＴ")
print("　２．ＭＥＣＨ　ＳＡＵＲＵＳ")
print("　３．ＭＥＣＨ　ＨＵＮＴＥＲ")
print("　４．ＭＥＣＨ　ＴＯＲＮＡＤＯ")
print("－－－－－－－－－－－－－－－")

s = input("数字を１文字だけ入力してください。")
match s[:1]:
    case "1": Mech01_Bullets.テスト実行()
    case "2": Mech02_Saurus.テスト実行()
    case "3": Mech03_Hunter.テスト実行()
    case "4": Mech04_Tornado.テスト実行()
    case _  : print("\n無効な入力です\n")
