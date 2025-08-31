# Rottenmeier ライブラリ<BR>マニュアル(クライアント側)
> コードネーム：**Rottenmeier**  
> クライアント：CPython / MicroPython / CircuitPython  
> 対象：MMP サーバー(ファームウェア)と通信するクライアント用ライブラリ

本書は、クライアント側ライブラリの**共通仕様**と**環境別の相違点**、および
コード共通化後の**運用・配置ルール**、**アーキテクチャ**をまとめたものです。

---

## 1. 概要(共通化のポイント)
- **Core/Adapter/Wrapper** の 3 層構成に刷新し、**通信ロジック**と**環境依存 I/O**を分離しました
- 受信は「**5 バイト固定／末尾 `!`**」の MMP プロトコルを前提に、**バッファの末尾 5 バイトのみ保持**して判定
- タイムアウトは **ms(内部表現)**で統一、CPython では pyserial の秒指定に合わせてアダプタで吸収
- PWM 系は文字列フォーマットをやめて**バイト列直接書き込み**(`b"PWM:%02X:%04X!"` 等)で高速化
- **Command RX** は各環境とも同一ロジック：`_rx_ready()`→`read(n)`→末尾 5 バイト評価(共通 Core に集約)

---

## 2. サポート環境早見表
| 項目 | CPython 版 | MicroPython 版 | CircuitPython 版 |
|---|---|---|---|
| 下位ライブラリ | `pyserial` | `machine.UART` | `busio.UART` / `board` |
| 物理接続 | USB 仮想 COM | GPIO UART | GPIO/USB UART |
| ポート検出 | **自動スキャン**/指定(同関数で振分け) | なし(ID/TX/RX を指定) | ピン名解決/API 生成 |
| タイムアウト指定 | 秒(アダプタで吸収) | ms | ms(内部は s へ換算) |
| 受信最適化 | `in_waiting` + まとま読み | `any()` + まとま読み | `in_waiting` + まとま読み |
| 例外方針 | 接続時のみ `ConnectException` | 同左 | 同左 |

---

## 3. フォルダ配置とファイル構成(運用ルール)

### 3.1 推奨レイアウト(共通)
詳しい内容は、各ファイルのヘッダコメントを参照ください。
```
【開発環境】
[/MMP/ファームウェア/v0.3_Rottenmeier/ライブラリ/]
  mmp_core.py                # 共有Core(環境非依存)★編集の主体
  mmp_adapter_cpython.py     # CPython 用アダプタ
  mmp_adapter_micropython.py # MicroPython 用アダプタ
  mmp_adapter_circuitpython.py # CircuitPython 用アダプタ
  mmpC.py                    # CPythonラッパ
  mmpMicro.py                # MicroPythonラッパ
  mmpCircuit.py              # CircuitPythonラッパ

 [/MMP/ゲーム/00_Test/CLI/]
  共通ユーティリティ.py      # テスト/デモ用ユーティリティ
  Mech01_Bullets.py          # サンプルテスト本体１
  Mech02_Saurus.py           # サンプルテスト本体２
  Mech03_Hunter.py           # サンプルテスト本体３
  Mech04_Tornado.py          # サンプルテスト本体４
```
```
【実行環境：CPython版】
[/]
├[共通/]
│　mmp_core.py
│  mmp_adapter_micropython.py
│  mmpMicro.py
│  共通ユーティリティ.py
│
└[ゲーム/]
　　└[タイトル/]
    　　└[タイトル/]
    　　　　main.py
    　　　　<サブアプリ1>
    　 　　　　・・・
    　　　　<サブアプリn>
```
```
【実行環境：microPython版】
[/]
├[lib/]
│　mmp_core.py
│  mmp_adapter_micropython.py
│  mmpMicro.py
│  共通ユーティリティ.py
│
└ main.py
　<サブアプリ1>
　　・・・
　<サブアプリn>
```
```
【実行環境：circuitPython版】
[/]
├[lib/]
│　mmp_core.py
│  mmp_adapter_circuitpython.py
│  mmpCircuit.py
│  共通ユーティリティ.py
│
└ main.py
　<サブアプリ1>
　　・・・
　<サブアプリn>
```

### 3.2 配置上の注意
- **CPython**：`共通/`を sys.path へ追加するかパッケージ化。  
- **MicroPython / CircuitPython**：ボードのストレージ制約により、**`/lib` 直下**に置くか、ファイル階層を浅くする  
  - 例：`/lib/mmp_core.py`, `/lib/mmp_adapter_micropython.py`, `/lib/mmpMicro.py` など  
  - **ファイル名の変更/分割は最小限**にし、**Core は 1 ファイル**で維持  
- ライブラリ更新時は**Core を先に**、次に各**Adapter**、最後に**Wrapper**を更新。

> **運用のコツ**：Core の変更は 3 環境へ即時反映されるので、**共通部分は Core に集約**し、
> 環境固有の I/O は Adapter に限定して実装します。

---

## 4. ランタイム構成(技術概要)

### 4.1 3 層の役割
- **Core(mmp_core.Core)**
  - プロトコル実装(送受信、5 バイト評価、アナログ/デジタル/PWM/I2C/MP3 などのコマンド生成)。
  - 時間ヘルパ(`_now_ms`, `_time_left_ms`, `_sleep_ms`)。
  - 受信最適化(`_rx_ready()` と `read(n)` のまとま読み、末尾 5 バイトリング)。
- **Adapter(mmp_adapter_*)**
  - **環境依存 I/O** の最小実装：`open_port`/`close`/`write`/`read`/`rx_ready`/`flush`。
  - タイムアウト単位の揺れ(秒/ms)を吸収し、**Core からは同じ API** に見えるようにする。
- **Wrapper(mmpC/mmpMicro/mmpCircuit)**
  - 既存アプリ互換の**公開クラス名/メソッド名**を提供。  
  - 接続方法の差(自動スキャン/ピン解決)など**使い勝手**を整える。

### 4.2 データフロー
```
App → Wrapper.mmp → Core._コマンド送信() → Adapter.write()
                                  ↓
                          Core._コマンド受信() ← Adapter.read()/rx_ready()
```

---

## 5. 取り込み(使い方)
CPython / MicroPython / CircuitPython 毎に方向が異なります。

### 5.1 CPython
ミドルウェア生成関数で、必要に応じて各種設定ができます。
> ライブラリ.mmp(
>　　arg読込調整 = 64,
>　　arg通信速度 = 115200,
>　　arg時間切れ = 0.2
>　　):

**デフォルトで通信接続する例：**
```python
#======================================================
# ラウンチャー(CPython版)
#======================================================
import sys;sys.path.append("../..")
import mmpC as ライブラリ
from 共通ユーティリティ import (
    アナログ入力測定,
    MP3_再生
    )

def main():
    MMP = ライブラリ.mmp()
    MMP.通信接続()
```

### 5.2 MicroPython
ミドルウェア生成関数で、必要に応じて各種設定ができます。
> ライブラリ.mmp(
>　　arg読込調整 = 64,
>　　arg通信速度 = 115200,
>　　arg時間切れ = 100,
>　　arg割当ピンTX = 0,
>　　arg割当ピンRX = 1,
>　　argシリアル番号 = 0
>　　):

**デフォルトで通信接続する例：**
```python
#======================================================
# ラウンチャー(MicroPython版)
#======================================================
import mmpMicro as ライブラリ
from 共通ユーティリティ import (
    アナログ入力測定,
    MP3_再生
    )

def main():
    MMP = ライブラリ.mmp()
    MMP.通信接続()
```

### 5.3 CircuitPython
ミドルウェア生成関数で、必要に応じて各種設定ができます。
> import board
> ライブラリ.mmp(
>　　arg読込調整 = 128,
>　　arg通信速度 = 115200,
>　　arg時間切れ = 100,
>　　arg割当ピンTX = None,
>　　arg割当ピンRX = None 
>　　):

**デフォルトで通信接続する例：**
```python
#======================================================
# ラウンチャー(CircuitPython版)
#======================================================
import mmpCircuit as ライブラリ
from 共通ユーティリティ import (
    アナログ入力測定,
    MP3_再生
    )

def main():
    MMP = ライブラリ.mmp()
    MMP.通信接続()
```

---

## 6. パブリック API(共通)
- **接続系**：`通信接続([自動/指定])`, `通信切断()`, `バージョン確認()`
- **アナログ**：`アナログ設定()`, `アナログ読取()`, `mmpAnaVal[参加][スイッチ]`
- **デジタル**：`digital_IN(port)`, `digital_OUT(port, val)`
- **PWM**：`PWM_VALUE(ch, pwm)`, `PWM_ANGLE(ch, ang)`, `PWM_INIT(lo, hi, pmin, pmax)`  
  - **情報確認**：`PWM_機器確認()`, `PWM_チャンネル使用可否(ch)`
  - **特殊用途**：`PWM_POWER(ch, on_off)` — 電源スイッチ(`True=ON(4095) / False=OFF(0)`)
- **MP3(DFPlayer)**：`DFP_Info()`, `DFP_PlayFolderTrack()`, `DFP_Stop()/Pause()/Resume()`, `DFP_Volume()`, `DFP_set_eq()`, `DFP_get_play_state()`
- **I2C(プロキシ)**：`i2cWrite(addr, reg, val)`, `i2cRead(addr, reg)`(1 バイト)

> 返却は基本 **`True/False` または整数**。異常時は `None/False/0` を返す方針。  
> 接続時の致命エラーは `ConnectException`。

---

## 7. タイムアウト／速度最適化の要点
- **受信**は `_rx_ready()` の結果から **上限 `読込調整`(既定 64/128)** までまとめ読み。
- **MicroPython** は `time.sleep_ms(1)` の粒度影響が大きいため、テスト側で待ち時間を**必要最小**に調整。 
- **PWM スイープ**など連続コマンドは、可能な限り**バイト列直書き**の高速ルート(既定実装)を利用  
- **タイムアウト**はコマンド単位(ms)、5 バイトのレスポンス成立まで待機し、超過で `None`

---

## 8. 互換性／破壊的変更
- 既存アプリとの互換を重視し、**公開メソッド名は維持**  
- 接続 API(CPython)：`通信接続([comm_num])` で **自動/指定を同関数**に統合(ラッパ対応)

---

## 9. トラブルシュート
- **接続不可**：COM/UART 設定(ID/TX/RX/ボーレート 115200 8N1)、GND 共通、配線
- **タイムアウト多発**：`timeout_ms` を増やす／ノイズ対策／配線長短縮
- **速度が出ない**：`読込調整`(64～128)を環境に合わせて調整、テスト側の `sleep` を見直す

---

## 10. バージョニングと保守
- 変更は **Core → Adapter → Wrapper** の順に反映
- **コメントは保全**(設計判断や制約はコメントに集約)
- 3 環境の **動作確認テスト**(`Mech02_Saurus.py`)を通すことで互換性を担保

---

© Rottenmeier Client Library
