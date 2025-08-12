# 動画メニュー再生ツール 説明書（`video_gui_player.py`）

## 概要
Tkinter で作ったメニューから動画を選び、**VLC（python-vlc）を埋め込んで再生**するツールです。  
次の仕様を備えています。

- 動画リスト（辞書）を渡してメニュー表示 → 選択して再生
- **URLは既定ブラウザで再生**（直リンクの動画拡張子は VLC で再生）
- **起動時ミュート／最大化／フルスクリーン／自動再生**を引数で制御
- 自動再生しない場合でも**最初のフレームを無音でプレビュー**表示
- 再生終了後は**最後のフレームを表示したまま**カウントダウンしてメニューに復帰
- **OSDを完全に無効化**（ファイル名やスナップショットのパス表示、左上の小画像を非表示）
- シークバー、音量スライダ、ミュート、最大化ボタンを備えた簡易コントロール

---

## 必要環境
- Python 3.8+ 推奨（Tkinterは同梱）
- パッケージ: `python-vlc`（`pip install python-vlc`）
- **VLC 本体**（`libvlc.dll` と `plugins`）  
  - 推奨: `video_gui_player.py` と**同じフォルダ**に `libvlc.dll` と `plugins/` を置く  
  - もしくは VLC を通常インストールして `libvlc.dll` が PATH から見えるようにする  
  - **ビット数は Python と VLC で合わせてください**（64bit Python ↔ 64bit VLC）

---

## インストール手順

1. Python をインストール（64bit 推奨）  
   - Windows の場合、インストーラで「PATH に追加」を有効にします。
   - **Python と VLC のビット数は必ず一致**させてください。

2. 依存パッケージをインストール
   ```bash
   pip install -r requirements.txt
   # もしくは
   pip install python-vlc
   ```

3. VLC 本体の用意
   - 公式の VLC をインストールする（推奨）。  
     → 自動で `libvlc.dll` が見つかる可能性が高いです。
   - もしくは、`libvlc.dll` と `plugins/` を `video_gui_player.py` と同じフォルダに配置。

4. 動作確認（サンプル）
   ```bash
   python example/run_demo.py
   ```

---

## 使い方

### 1) 動画リストを用意
```python
VIDEOS = {
    "mech_saurus": {
        "title": "MECH SAURUS ストーリー",
        "desc" : "恐竜を救出する物語",
        "path" : "./games/02_MechSaurus.mpg",  # 相対パスOK
    },
    "mech_hunter": {
        "title": "MECH SAURUS 遊び方",
        "desc" : "遊び方の紹介",
        "path" : "./games/02_MechSaurus実録.mp4",
    },
    # URL例（直リンクでなければ既定ブラウザで開く）
    # "site": {"title": "公式サイト", "desc": "", "path": "https://example.com"}
}
```

### 2) メニューを開く
```python
from video_gui_player import open_video_menu

open_video_menu(
    VIDEOS,
    window_title="MECH 動画メニュー",
    default_volume=70,
    countdown_secs=3,
    initial_mute=False,
    initial_maximized=False,
    initial_fullscreen=False,
    initial_autoplay=True,
)
```

---

## 引数（`open_video_menu(...)`）

| 引数名 | 型 / 既定値 | 説明 |
|---|---|---|
| `videos` | `dict` / **必須** | 再生候補の辞書（上記例参照） |
| `window_title` | `str` / `"動画メニュー"` | メニューウィンドウのタイトル |
| `default_volume` | `int` / `80` | 再生開始時の音量（0–100） |
| `countdown_secs` | `int` / `3` | 再生終了後にメニューへ戻るまでの秒数 |
| `initial_mute` | `bool` / `False` | 起動時ミュートにする |
| `initial_maximized` | `bool` / `False` | プレイヤーを最大化で開始 |
| `initial_fullscreen` | `bool` / `False` | プレイヤーをフルスクリーンで開始（F11で切替） |
| `initial_autoplay` | `bool` / `True` | 起動時に自動再生する（`False`で停止状態＋最初のフレーム表示） |

---

## 操作方法（再生画面）

- 再生／一時停止 … **Space** キー／「再生」「一時停止」ボタン  
- 停止（即カウントダウンへ）… 「停止」ボタン  
- 音量上下 … **↑ / ↓**  
- ミュート … 「ミュート」チェック  
- シーク … スライダをドラッグ  
- **フルスクリーン切替** … **F11**  
- **フルスクリーン中に戻る** … **Esc**  
- 終了（メニューへ戻る）… **Esc**（通常表示時） / **q** / **Ctrl+Q** / ウィンドウ右上の×  

> 再生終了時：最後のフレームを表示したままカウントダウン（既定3秒）。  
> 途中でキーやクリックをすると即座にメニューに戻れます。

---

## URLの取り扱い
- `http(s)://` で **動画ファイル拡張子（.mp4/.mpg/.mkv…）を含む直リンク**は VLC で再生します。
- それ以外（YouTube などの通常WebページURL）は **既定ブラウザで開きます**。

---

## OSD（オンスクリーン表示）について
- VLC 起動時に以下を指定し、**OSDを出しません**。  
  - `--no-video-title-show`（再生開始時のファイル名／パス表示を抑止）  
  - `--no-snapshot-preview`（スナップショット時の左上プレビュー抑止）  
  - `--no-osd`（音量など含む OSD 全消し。スナップショット保存先パスの表示も消える）  

---

## 内部動作のポイント
- **プレビュー**：`initial_autoplay=False` のときは無音で一瞬だけ再生→即ポーズして**最初のフレーム**を表示（黒画面回避）。  
- **最後のフレーム保持**：再生中に低頻度でスナップショットを取り、終了時はその画像を重ねて**最後の映像**を見せ続けます。  
- **メニュー復帰**：終了/停止/閉じる操作で必ずメニューに復帰し、再選択が可能です。  

---

## トラブルシューティング

### 1) `FileNotFoundError: libvlc.dll が見つからない`
- `video_gui_player.py` と**同じフォルダ**に `libvlc.dll` と `plugins/` を置く（推奨）  
- もしくは VLC を通常インストールして `libvlc.dll` へのパスが通るようにする

### 2) 音が出ない / ずっとミュートになる
- `initial_mute` が `True` になっていないか確認  
- GUI の「ミュート」チェックを外し、音量スライダを上げる

### 3) 画面が黒い／最初のフレームが見えない
- `initial_autoplay=False` でもツール側が**無音プリロール**で最初のフレームを表示します。出ない場合は動画コーデックやファイルを確認。

### 4) 画面上部に文字や左上に小画像が出る
- 本ツールでは `--no-osd` を指定済みです。もし残る場合は VLC のバージョンを更新するか、設定が上書きされていないか確認してください。

---

## ライセンス／同梱について
- 本スクリプトはプロジェクトに同梱して自由に利用できます。  
- VLC（libvlc, plugins）は VideoLAN のライセンス条件に従って配布・同梱してください。
