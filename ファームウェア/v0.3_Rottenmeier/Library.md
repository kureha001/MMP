# ライブラリ マニュアル
> コードネーム：**Rottenmeier**
> クライアント：CPython / MicroPython

本書は、MMPサーバー(ファームウェア)とやり取りする**クライアント(アプリ)側のライブラリ**のリファレンスです。

ライブラリは、**CPython 版**・**MicroPython 版**があります。
それぞれの**共通仕様**と**相違点**をまとめています。

---

## 0. CPython 版と MicroPython 版の違い

| 項目 | CPython 版（Windows） | MicroPython 版（マイコン上） |
|---|---|---|
| 物理接続 | USB 経由の仮想 COM | **GPIO UART**（8N1） |
| ポート発見 | **自動スキャン（COM0..）** | **自動スキャンなし**：明示指定（UART ID / TX / RX / ボーレート） |
| ライブラリ | `pyserial` | `machine.UART` |
| ログ出力 | `print()` → コンソール | 既定で**抑制**（アプリ側で必要に応じて出力） |
| タイムアウト | `Serial.read(timeout)` 依存 | **モジュール定数**で定義（初期値：`100ms`） |
| 例外方針 | 既存仕様 | **接続系のみ例外**、通常コマンドは戻り値で通知 |
| ピン設定 | なし | **ユーザー変更可**：UART ID / TX / RX を指定可能 |
| バッファ破棄 | 実装あり | **一律廃止**（不要） |
| I2C ワード系 | 実装あり | **廃止**（1 バイト版のみ） |

> **重要**：通信プロトコル（コマンド/戻り値 5 文字固定、16進大文字、終端 `!`）は**サーバー仕様を厳守**します。データ型のバリデーションは**行いません**（5 文字・終端のみ確認）。

---

## 1. 仕様の前提（共通ルール）

- **戻り値は常に 5 文字**  
  - 正常（値なし）：`!!!!!`  
  - 正常（値あり）：`XXXX!`（16 進 4 桁＋`!`）  
  - 異常（処理失敗）：`<CMD>!!`  
  - 異常（不明/引数違反）：`<CMD>?!`
- **数値表記は 16 進・大文字・ゼロ埋め固定桁**  
  例：ID/ポート=`%02X`、角度=`%03X`、PWM/戻り値=`%04X`
- **同期（ブロッキング）実行**：1 コマンド完了後に次コマンドへ
- **誤受信対策**：先行ゴミバイトは破棄し、**`!` を含む 5 文字**を 1 レスポンスとして扱います（内容バリデーションはしない）

---

## 2. インストール / 取り込み

### CPython 版（Windows）
- ファイル名：`mmpRottenmeier.py`
- `共通フォルダ` などに配置してから：

```python
import sys;sys.path.append("../..")
import 共通.mmpRottenmeier as MMP

接続 = MMP.mmp()
接続.通信接続()               # 自動接続の場合
#接続.通信接続_指定("COM5")   # COMポート指定接続の場合
ver = 接続.バージョン()       # "xyzz" 形式（実体は "xyzz!" を整形）
```

### MicroPython 版（マイコン上）
- ファイル名：`mmpRottenmeier_micro.py`
- `/lib/` や `同一フォルダ` などに配置してから：

```python
from mmpRottenmeier_micro import mmp

# UART 設定はユーザーが指定可能（既定：ID=1, TX=0, RX=1, 115200, 8N1, timeout=100ms）
接続 = mmp()
接続.通信接続(uart_id=1, tx_pin=0, rx_pin=1, baud=115200)  # 固定接続（自動スキャンなし）
ver = 接続.バージョン()  # "x.y.zz" 形式に整形
```

---

## 3. 設定（MicroPython 版）

モジュール内に既定値を定義します（変更可）。

- `既定_UART_ID = 1`
- `既定_TX_PIN = 0`
- `既定_RX_PIN = 1`
- `既定_BAUD = 115200`（8N1）
- `既定_TIMEOUT_MS = 100`

> 端子/速度は**環境に合わせて明示指定**してください。

---

## 4. 例外 / エラー通知

- **例外を投げるメソッド**（致命的）：
  - `通信接続(...)`：ポート初期化/オープン失敗時に `ConnectException` を送出
  - `通信切断()`：実装依存（未接続時の多重切断は安全に無視）
- **戻り値で通知するメソッド**（アプリ側で扱う想定）：
  - デジタル I/O、PWM、アナログ、I2C、DFPlayer 系各メソッド  
    - 正常：`True` あるいは 16 進値（int 変換結果）  
    - 異常：`False`（または `None`）等、現行実装に準拠

> タイムアウト/プロトコル長不整合時は**接続維持のまま**戻り値で通知します。致命的切断が検出できる場合のみ例外。

---

## 5. パブリック API（関数名は既存と同一）

### 5.1 接続管理
MMPサーバとの接続に関する機能です。
接続を確立せずに他のコマンドを実行しても、失敗します。
- `通信接続(uart_id=1, tx_pin=0, rx_pin=1, baud=115200, timeout_ms=既定)`  
  - **自動スキャンなし**。GPIO UART を**機械的に**初期化
  - 接続確立時に `self.接続済 = True`
- `通信切断()`  
  - UART をクローズし `self.接続済 = False`
- `バージョン() -> str`  
  - `VER!` → `"xyzz!"` を `"x.y.zz"` に整形して返す

### 5.2 アナログ入力（HC4067）
MMPサーバのアナログポート(RJ45ポート)の入力信号を扱います。
外部接続する部品のADC値を取得する際に使用します。
電圧のADC値を扱うので、デジタル信号も扱えます。
参照前は、必ず初期設定を行ってください。
- `アナログ設定(argスイッチ数=4, arg参加人数=1, arg丸め=5)`  
  - `ANS:<CH>:<MUX>!` を送信、内部バッファ `mmpAnaVal` を初期化
- `アナログ読取()`  
  - `ANU!` → 直後に `ANR:<pl>:<sw>!` を **全範囲**で走査し `mmpAnaVal[pl][sw]` を更新

### 5.3 デジタル I/O
MMPサーバに搭載したマイコン(GPIO)で、デジタル信号を扱います。
GPIO数は少ないので、あまり多くのことはできません。
- `digital_OUT(argPort, argValue) -> bool` … `POW`（`!!!!!` で True）  
- `digital_IN(argPort) -> int` … `POR`（戻り `XXXX!` を 16 進 int に）  
- `digital_IO(argPort, argValue) -> int` … `IO`（出力→入力の値）

### 5.4 PWM（PCA9685）
MMPサーバに搭載したPWM出力モジュール(PCA9685)を扱います。
外部接続する能動部品をPWM制御する際に使用します。
`パルス制御`,`電圧調整`,`電源のON/OFF`などで使用します。
- `PWM_機器確認() -> list[bool]` … `PWX` を 16 機分問い合わせ（True/False 配列）
- `PWM_チャンネル使用可否(channel: int) -> bool` … 機器接続状況から 0–255 の可否を返す
- `PWM_VALUE(argPort, argPWM)` … `PWM:<ch>:<val>!`
- `PWM_ANGLE(argPort, argAngle)` … `PWA:<ch>:<ang>!`
- `PWM_INIT(argPwmMin, argPwmMax)` … **インターフェースは PWI に準拠**（将来拡張を見据え、`RS/RE/PS/PE` 4 つ方式を採用）

> 角度↔PWM 特性は**ボード全体**に適用（将来 per-ch へ拡張予定）。

### 5.5 MP3（DFPlayer）
MMPサーバに搭載した、MP3プレイヤー(DFPlayer mini)を扱います。
アプリケーションの効果音やBGMなどに使用します。
- `DFP_Info(devNo)` → `DPX` （接続有無）
- `DFP_PlayFolderTrack(devNo, folder, track)` → `DIR`
- `DFP_get_play_state(devNo, stNo)` → `DST`（1～5 の項目番号に対応）
- `DFP_Stop/DFP_Pause/DFP_Resume/DFP_Volume/DFP_set_eq` → `DSP/DPA/DPR/VOL/DEF`

### 5.6 I2C
MMPサーバに搭載した、マイコンのI2Cを扱います。
MMPサーバに外部接続する部品の制御に使います。
- `i2cWrite(addr, reg, val) -> bool` … `I2W`（1 バイト書き込み）
- `i2cRead(addr, reg) -> int` … `I2R`（1 バイト読み出し）

---

## 6. タイムアウトと再試行

- **既定タイムアウト**：`100ms`（115200bps 前提、処理余裕を含む）
- **再試行は既定で行いません**（アプリ側ポリシーで必要ならラップしてください）
- 受信は**5 文字揃い**まで待機し、超過でタイムアウト扱い（接続は維持）

---

## 7. 使用例（MicroPython 版）

### 7.1 接続とバージョン
```python
from mmpRottenmeier_micro import mmp
m = mmp()
m.通信接続(uart_id=1, tx_pin=0, rx_pin=1, baud=115200)
print("FW:", m.バージョン())  # 例: "0.3.04"
```

### 7.2 アナログ入力（全更新→参照）
```python
m.アナログ設定(argスイッチ数=4, arg参加人数=16, arg丸め=5)
m.アナログ読取()
print(m.mmpAnaVal[1][2])  # プレイヤー1 / スイッチ2 の最新値
```

### 7.3 PWM（角度指定）
```python
m.PWM_機器確認()                   # 接続機器の True/False 配列
m.PWM_VALUE(0x00, 0x0000)          # CH0 を 0 に
m.PWM_ANGLE(0x00, 90)              # CH0 を 90 度に
m.PWM_INIT(150, 600)               # 将来拡張を見据え、PWI 互換の 4 引数版に準拠
```

### 7.4 デジタル I/O
```python
ok = m.digital_OUT(6, 1)           # GPIO6 を High（True/False）
val = m.digital_IN(7)              # GPIO7 を読む（0/1）
val2 = m.digital_IO(3, 1)          # 出力後に読み取り
```

### 7.5 I2C（1 バイト）
```python
m.i2cWrite(0x40, 0x00, 0x01)
value = m.i2cRead(0x40, 0x00)
```

---

## 8. 互換性メモ

- **関数名は既存のまま**（アプリ側からの呼び出し互換を維持）  
  ただし、**接続系**は `通信接続` に一本化（自動スキャン削除）。
- I2C ワード系は**廃止**（サーバー仕様に 1 バイトで整合）
- PWM：`PWX`（機器接続確認）を**クライアントにも実装**
- DFPlayer：`DST` の**項目番号 1～5**に対応（サーバーの統合仕様へ合わせ込み）

---

## 9. トラブルシュート
- **接続できない**：UART ID / TX / RX / ボーレート（8N1）をご確認ください
- **タイムアウトが多い**：`timeout_ms` を上げる（例：200ms）、配線と GND 共有を確認
- **想定外の値**：サーバー側マニュアルの**戻り値 5 文字規約**を参照。内容バリデーションは行わないため、アプリ側でリトライ等のポリシーを設けてください

---

© Rottenmeier Client Library
