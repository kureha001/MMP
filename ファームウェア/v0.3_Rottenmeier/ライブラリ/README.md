# ライブラリ マニュアル
> コードネーム：**Rottenmeier**  
> クライアント：CPython / MicroPython / CircuitPython

本書は、MMPサーバー(ファームウェア)とやり取りする**クライアント(アプリ)側のライブラリ**のリファレンスです。  
ライブラリは **CPython 版**・**MicroPython 版**・**CircuitPython 版**があります。  
それぞれの**共通仕様**と**相違点**をまとめています。

---

## 0. CPython / MicroPython / CircuitPython の違い

| 項目 | CPython 版（Windows 等） | MicroPython 版（マイコン上） | CircuitPython 版（マイコン上） |
|---|---|---|---|
| 物理接続 | USB 仮想COM | **GPIO UART**（8N1） | **UART**（board.TX/RX / USBシリアル経由可） |
| ポート発見 | **自動スキャン**（COM*）/ 指定 | **自動スキャンなし**：明示指定（UART ID / TX / RX / ボーレート） | **明示指定**（`busio.UART` 生成 or TX/RX/baud 指定） |
| ライブラリ | `pyserial` | `machine.UART` | `busio.UART`, `board` |
| ログ出力 | `print()`（適宜） | **常時 `print()`**（抑制フラグ無し） | **常時 `print()`**（抑制フラグ無し） |
| タイムアウト | `serial.read(timeout)` 依存（将来統一予定） | **モジュール定数** / 引数（初期値：`100ms`） | `timeout`/`timeout_char` 相当（実装内で調整） |
| 例外方針 | 既存仕様 | **接続系のみ例外**、通常コマンドは戻り値で通知 | **接続系のみ例外**、通常コマンドは戻り値で通知 |
| ピン設定 | なし | **ユーザー変更可**（UART ID / TX / RX） | **ユーザー変更可**（TX / RX / `busio.UART` インスタンス） |
| I2C ワード系 | 実装あり | **廃止**（1 バイト版のみ） | **廃止**（1 バイト版のみ） |

> **重要**：通信プロトコル（コマンド/戻り値 5 文字固定、16進大文字、終端 `!`）は**サーバー仕様を厳守**します。  
> 送受信値のバリデーションは基本的に**行いません**（長さ5・末尾`!`のみ確認）。

---

## 1. 仕様の前提（共通ルール）

- **戻り値は常に 5 文字**  
  - 正常（値なし）：`!!!!!`  
  - 正常（値あり）：`XXXX!`（16進4桁＋`!`）  
  - 異常（処理失敗）：`<CMD>!!`  
  - 異常（不明/引数違反）：`<CMD>?!`
- **数値表記は 16 進・大文字・ゼロ埋め固定桁**  
  例：ID/ポート=`%02X`、角度=`%03X`、PWM/戻り値=`%04X`
- **同期（ブロッキング）実行**：1 コマンド完了後に次コマンドへ
- **誤受信対策**：先行ゴミバイトは破棄し、**`!` を含む 5 文字**を 1 レスポンスとして扱います（内容の整合はアプリ側方針で）

---

## 2. インストール / 取り込み

### CPython 版（Windows 等）
- ファイル名：`mmpRottenmeier.py`
- ライブラリを共有する場合、環境変数にパスを設定
  - Windows: set PYTHONPATH=C:\path\to\lib
  - macOS/Linux: export PYTHONPATH=/path/to/lib
- 例：<BR>(`../../共通`にライブラリを置き相対パス指定)

```python
import sys; sys.path.append("../..")
import 共通.mmpRottenmeier as MMP

接続 = MMP.mmp()            # コンストラクタで既定設定
接続.通信接続()              # 自動接続（または 接続.通信接続_指定("COM5")）
print("FW:", 接続.バージョン())  # "x.y.zz"
```

### MicroPython 版（マイコン上）
- ファイル名：`mmpRottenmeier_micro.py`
- ストレージの`/lib/`または`/`にライブラリを置く
- 例：

```python
from mmpRottenmeier_micro import mmp

接続 = mmp()  # 既定: ID=1, TX=0, RX=1, 115200bps, timeout=100ms
接続.通信接続()  # 自動スキャンなし（GPIO UART 固定）
print("FW:", 接続.バージョン())
```

### CircuitPython 版（マイコン上）
- ファイル名：`mmpRottenmeier_circuit.py`
- ストレージの`/lib/`または`/`にライブラリを置く
- 例：

```python
import board, busio
from mmpRottenmeier_circuit import mmp

# どちらかの方式を選択：
# 1) 既存UARTを渡す
uart = busio.UART(board.TX, board.RX, baudrate=115200, timeout=0.1)
接続 = mmp(uart=uart)

# 2) ピンと速度を渡して内部で生成
# 接続 = mmp(tx=board.TX, rx=board.RX, baud=115200, timeout=0.1)

接続.通信接続()
print("FW:", 接続.バージョン())
```

---

## 3. 例外 / エラー通知（共通）

- **例外を投げるメソッド**（致命的）：
  - `通信接続(...)`：UARTの初期化/オープンに失敗、または `VER!` 応答無し → `ConnectException`
  - `通信切断()`：未接続時の多重切断は安全に無視
- **戻り値で通知するメソッド**（通常系）：
  - デジタル I/O、PWM、アナログ、I2C、DFPlayer 系各メソッド  
    - 正常：`True` あるいは 16進値（int 変換結果）  
    - 異常：`False`（または `None`）

> タイムアウト/長さ不整合時は**接続維持のまま**戻り値で通知。致命的切断のみ例外。

---

## 4. パブリック API（抜粋・共通呼称）

### 4.1 接続管理
- `通信接続(...)` / `通信切断()` / `バージョン()`

### 4.2 アナログ入力（HC4067）
- `アナログ設定(CH, MUX, 丸め)` → `AN**` コマンド  
- `アナログ読取()`：`ANU!`→`ANR` 全走査、`mmpAnaVal[pl][sw]` 更新

### 4.3 デジタル I/O
- `digital_OUT(PORT, VAL)` / `digital_IN(PORT)` / `digital_IO(PORT, VAL)`

### 4.4 PWM（PCA9685）
- `PWM_VALUE(ch, val)` / `PWM_ANGLE(ch, ang)` / `PWM_INIT(rs, re, ps, pe)`  
- `PWM_機器確認()` / `PWM_チャンネル使用可否(ch)`

### 4.5 MP3（DFPlayer）
- `DFP_Info(id)` / `DFP_PlayFolderTrack(id, folder, track)` / `DFP_Stop()` ほか

### 4.6 I2C
- `i2cWrite(addr, reg, val)` / `i2cRead(addr, reg)`（1 バイト）

---

## 5. タイムアウトと再試行

- **既定タイムアウト**：約 `100ms`（実装により `timeout` / `timeout_char` を設定）
- **自動再試行は行いません**（必要に応じてアプリ側でラップ）
- 受信は**5 文字揃い**まで待機し、超過でタイムアウト扱い（接続は維持）

---

## 6. サンプル（共通ユーティリティ併用）

共通ユーティリティ `共通ユーティリティ.py` を用いると、CPython/MicroPython/CircuitPython いずれでも同一のテスト本体を呼び出せます。

```python
# 例：Bullets テスト（接続～実行～切断）
from Mech01_Bullets import テスト実行
import mmpRottenmeier_micro as drv  # 例：実行環境に応じて差し替え

テスト実行(lambda: drv.mmp())
```

---

## 7. 互換性メモ

- **関数名は既存のまま**（アプリ互換維持）  
- 接続系は環境ごとに異なるが、**テスト本体は共通化**可能（ランチャーで分岐）
- I2C ワード系は**廃止**（1 バイトで統一）
- PWM：`PWX`（機器接続確認）をクライアントにも実装
- MP3：`DST` の**項目番号 1～5**に対応

---

## 8. トラブルシュート

- **接続できない**：UART/COM、TX/RX、ボーレート（8N1）と GND 共有を確認  
- **タイムアウトが多い**：`timeout` を上げる（例：200ms）／配線・ノイズ対策  
- **想定外の値**：サーバーマニュアルの**「戻り値 5 文字」規約**を参照。必要に応じてアプリ側でリトライ

---

© Rottenmeier Client Library
