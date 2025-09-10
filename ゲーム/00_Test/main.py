# main.py  — CPython / MicroPython / CircuitPython 互換版
# 変更点:
#  - match-case を廃止し、ディスパッチ(dict)で分岐
#  - モジュールは選択後に遅延import（省メモリ＆存在しない環境でもメニュー表示可）

def _read_one_digit(prompt="数字を１文字だけ入力してください: "):
    try:
        s = input(prompt).strip()
    except EOFError:
        return ""
    return s[:1] if s else ""

def _run_mech01():
    import Mech01_Bullets as M
    M.テスト実行()

def _run_mech02():
    import Mech02_Saurus as M
    M.テスト実行()

def _run_mech03():
    import Mech03_Hunter as M
    M.テスト実行()

def _run_mech04():
    import Mech04_Tornado as M
    M.テスト実行()

DISPATCH = {
    "1": _run_mech01,
    "2": _run_mech02,
    "3": _run_mech03,
    "4": _run_mech04,
}

def main():
    print("≪テスト メニュー≫")
    print("－－－－－－－－－－－－－－－")
    print("　１．ＭＥＣＨ　ＢＵＬＬＥＴ")
    print("　２．ＭＥＣＨ　ＳＡＵＲＵＳ")
    print("　３．ＭＥＣＨ　ＨＵＮＴＥＲ")
    print("　４．ＭＥＣＨ　ＴＯＲＮＡＤＯ")
    print("－－－－－－－－－－－－－－－")

    ch = _read_one_digit("数字を１文字だけ入力してください: ")
    func = DISPATCH.get(ch)
    if func is None:
        print("\n無効な入力です\n")
        return
    func()

if __name__ == "__main__":
    main()
