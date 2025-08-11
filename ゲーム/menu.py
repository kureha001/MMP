import os
import sys
from pathlib import Path

# 2階層メニュー: {カテゴリ番号: (カテゴリ名, {プログラム番号: (フォルダ, ファイル, タイトル)})}
MENU = {
    "1": ("ゲーム", {
        "1": ("01_MechBullets"  , "main.py", "メック・バレット"     ),
        "2": ("02_MechSaurus"   , "main.py", "メック・サウルス"     ),
        "3": ("03_MechHunter"   , "main.py", "メック・ハンター"     ),
        "4": ("04_MechTornado"  , "main.py", "メック・トルネード"   ),
    }),
    "2": ("ツール", {
        "1": ("00_Tools", "sd-card.py" , "FDPlayer用 SDカード作成"),
    }),
}

BASE_DIR = Path(__file__).resolve().parent  # ランチャーの場所

def clear():
    os.system('cls' if os.name == 'nt' else 'clear')

def exec_in_folder_replace(folder: str, file: str):
    """フォルダに移動し、現在プロセスを置き換えて実行（戻らない）"""
    target = (BASE_DIR / folder).resolve()
    if not target.is_dir():
        print(f"[エラー] フォルダが見つかりません: {target}")
        sys.exit(1)

    script = target / file
    if not script.is_file():
        print(f"[エラー] 実行ファイルが見つかりません: {script}")
        sys.exit(1)

    # 実行ディレクトリに移動して、プロセス置換実行
    os.system('cls' if os.name == 'nt' else 'clear')
    os.chdir(target)

    # sys.executable は現在の Python 実行ファイル
    # 第2引数は argv[0] 相当。以降にスクリプト名や引数を並べる
    os.execv(sys.executable, [sys.executable, file])


def main():
    clear()
    print("=== メインメニュー ===")
    for cat_num in sorted(MENU, key=lambda x: int(x)):
        cat_name = MENU[cat_num][0]
        print(f"{int(cat_num):02d}. {cat_name}")
    print("")
    print("00. 終了")
    print("")
    cat_choice = input("カテゴリ番号を入力してください: ").lstrip("0") or "0"
    if cat_choice == "0":
        print("終了します。")
        return
    if cat_choice not in MENU:
        print("無効な入力です。")
        return

    cat_name, programs = MENU[cat_choice]
    clear()
    print(f"=== {cat_name} ===")
    for prog_num in sorted(programs, key=lambda x: int(x)):
        _, _, title = programs[prog_num]
        print(f"{int(prog_num):02d}. {title}")
    print("")
    print("00. 戻る")
    print("")
    prog_choice = input("プログラム番号を入力してください: ").lstrip("0") or "0"
    if prog_choice == "0":
        print("メニューに戻るには、このランチャーを再実行してください。")
        return
    if prog_choice not in programs:
        print("無効な入力です。")
        return

    folder, file, _ = programs[prog_choice]
    exec_in_folder_replace(folder, file)  # ← ここでプロセス置換（戻りません）

if __name__ == "__main__":
    main()
