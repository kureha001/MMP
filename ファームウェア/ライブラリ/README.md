# 第2世代：MMP  Library

第一世代のフラットなAPIを一新し、階層APIに生まれ変わりました。
よりもモダンなプログラミングが可能で、可読性・保守性に優れたソースコードを作り出せます。

**Platforms:**
- マイコン用(GPIO‑UART接続)
  - Arduino：RP2040 / RP2350 / UnoR4
- PC用(USB‑UART接続)
  - .NET：C# Class Library
  - COM for VBA 

**Protocol:** Simple 5‑byte frames (HEX4 + `!` or `!!!!!`)

> ひとつの *統一 API* で、Arduino・C#・VBA を横断。  
> 高速921,600bpsでも安定して MMP デバイスを制御できます。

---

## 特徴（Why MMP Client?）
- **プラットフォーム横断の同一API**：クラス名・メソッド階層・戻り値を共通化。移植が容易。
- **堅牢な固定長プロトコル**：応答は必ず「5文字」。分割受信・前ゴミ混入に強い受信ロジック。
- **超高速でも安定**：`9,600 / 115,200 / 230,400 / 921,600 bps` で全API確認済み。
- **フル装備の階層API**：`Info / Analog / Pwm / Digital / I2c / Audio(Play, Read)` を網羅。
- **オーディオ拡張**：`Audio.Play.SetLoop()`、`Audio.Read.*` を実装。  
- **明確なエラーハンドリング**：OK＝`"!!!!!"`、値＝`"hhhh!"`、形式外は即エラー扱い。

---

## 対応環境
- **Arduino**：RP2040 / RP2350 系（WaveShare RP2350 Zero 等）  
  - GPIO‑UART（`Serial1`、TX=GPIO0 / RX=GPIO1 を想定）
  - 自動ボーレート検出：`9,600 / 115,200 / 230,400 / 921,600`
- **C# (.NET)**：クラスライブラリ `Mmp.Core.MmpClient`  
  - COMブリッジ `Mmp.Com`（VBA から `CreateObject("Mmp.Com")` で利用）

---

## クイックスタート

### Arduino
```cpp
#include "MmpClient.h"
using Mmp::Core::MmpClient;

MmpClient mmp;

void setup() {
  Serial.begin(115200);
  while(!Serial) {}
  if (!mmp.ConnectAutoBaud()) {
    Serial.println("Connect failed: " + mmp.LastError());
    return;
  }
  Serial.print("Connected @ "); Serial.println(mmp.ConnectedBaud()); // 実際のボーレート参照

  Serial.println(mmp.Info.Version());               // "xyzz!"
  mmp.Analog.Configure(1, 1);                       // true
  mmp.Analog.Update();                              // true
  int v = mmp.Analog.Read(0, 0);                    // 0..4095
  mmp.Audio.Play.FolderTrack(1, 1, 1);              // true
  mmp.Audio.Play.SetLoop(1, true);                  // true（新）
  int st = mmp.Audio.Read.PlayState(1);             // 0/1/2（新）
}
void loop(){}
```

### C#
```csharp
using var m = new Mmp.Core.MmpClient();
m.Settings.BaudRate = 115200;
if (!m.Connect()) throw new Exception("connect failed");

Console.WriteLine("VER: " + m.Info.Version());
m.Analog.Configure(1,1);
m.Analog.Update();
int val = m.Analog.Read(0,0);

m.Audio.PlayFolderTrack(1,1,1);
m.Audio.Play.SetLoop(1, true);               // 新規API
Console.WriteLine("State: " + m.Audio.Read.PlayState(1));  // 新規API
```

### VBA（COM）
```vb
Dim mmp As Object: Set mmp = CreateObject("Mmp.Com")
Dim actual As String
actual = mmp.Connect("", 115200, 200, 400)
If LenB(actual) = 0 Then Debug.Print "Connect failed: "; mmp.LastError: End

Debug.Print mmp.Info.Version(0)
Call mmp.Analog.Configure(1,1,0)
Call mmp.Analog.Update(0)
Debug.Print mmp.Analog.Read(0,0,0)

Call mmp.Audio.Play.FolderTrack(1,1,1,0)
Call mmp.Audio.Play.SetLoop(1,1,0) '1=ON,0=OFF（新）
Debug.Print mmp.Audio.Read.PlayState(1,0)   '新
```

---

## アーキテクチャ／API 概要（共通）

```
MmpClient
 ├── Info
 │    ├── Version()
 │    └── Dev.Pwm(deviceId), Dev.Audio(id1to4)
 ├── Analog
 │    ├── Configure(players 1..16, switches 1..4) : bool
 │    ├── Update() : bool
 │    └── Read(player 0..15, switch 0..3) : int (0..4095)
 ├── Pwm
 │    ├── Out(ch, value 0..4095) : "!!!!!" or "hhhh!"
 │    ├── AngleInit(aMin,aMax,pwmMin,pwmMax) : bool
 │    └── AngleOut(ch, angle 0..180) : "!!!!!" or "hhhh!"
 ├── Digital
 │    ├── In(port)  : int (0/1)
 │    └── Out(port, value0or1) : bool
 ├── I2c
 │    ├── Write(addr, reg, value) : bool
 │    └── Read(addr, reg) : int (0..255)
 └── Audio
      ├── Info(id1to4) : ushort
      ├── Volume(dev, 0..30) : bool
      ├── SetEq(dev, 0..5) : bool
      ├── Read
      │     ├── PlayState(dev) : int (0停止/1再生/2一時停止)
      │     ├── Volume(dev) : int
      │     ├── Eq(dev) : int
      │     ├── FileCounts(dev) : int
      │     └── CurrentFileNumber(dev) : int
      └── Play
            ├── FolderTrack(dev, folder, track) : bool   ← 文字列から bool に変更
            ├── Stop(dev)   : bool
            ├── Pause(dev)  : bool
            ├── Resume(dev) : bool
            └── SetLoop(dev, onOff) : bool               ← 新規追加
```

> **Arduinoのみ**  
> `ConnectedBaud()`（`uint32_t`）で *実際に接続できた* ボーレートを参照可能。  
> `Settings.BaudRate` は「意図する値」、`ConnectedBaud()` は「接続結果」。

---

## ワイヤプロトコル（MMP ⇄ クライアント）
- **リクエスト**：ASCII、末尾 `!`。例 `ANR:00:00!`  
- **応答**：**固定5文字**  
  - 成功・非数値：`"!!!!!"`  
  - 数値：`"hhhh!"`（h=HEX桁）
- 代表コマンド
  - `VER!` → `"xyzz!"`
  - `ANS:<players>:<switches>!` → `"!!!!!"`
  - `ANU!` → `"!!!!!"` / `ANR:<pl>:<sw>!` → `"hhhh!"`
  - `PWM:<ch>:<value>!` / `PWA:<ch>:<angle>!` / `PWI:<...>!`
  - `POR:<port>!` → `"hhhh!"` / `POW:<port>:<0|1>!` → `"!!!!!"`
  - `I2W:<addr>:<reg>:<val>!` → `"!!!!!"` / `I2R:<addr>:<reg>!` → `"hhhh!"`
  - `DIR:<dev>:<folder>:<track>!` → `"!!!!!"`
  - `DSP/DPA/DPR/VOL/DEF/DST:<dev>:<kind>!` → `"!!!!!"` or `"hhhh!"`
  - **新** `DLP:<dev>:<1|0>!` → `"!!!!!"`（ループON/OFF）

---

## インストール

### Arduino ライブラリ配置
```
<sketch>/
  ├─ MmpClient.h / MmpClient.cpp
  ├─ MmpHardware.h         // ピン/シリアル定義（TX=GPIO0, RX=GPIO1 等）
  └─ examples/MmpApiConsoleTest/MmpApiConsoleTest.ino
```
- RP2040/2350（非Mbedコア想定）。`Serial1` が GPIO0/1 に割当の環境。  
- 自動ボーレート：`ConnectAutoBaud()` が `115200/9600/230400/921600` を順次試行。

### .NET（C#）
- プロジェクト参照：`Mmp.Core` をアプリから参照。
- COM for VBA：`Mmp.Com.dll` を `RegAsm` で登録、または Inno Setup で `/codebase /tlb` 付与で登録。  
  VBA からは `CreateObject("Mmp.Com")`。

---

## サンプル（抜粋）

### アナログ
```csharp
m.Analog.Configure(1,1);
m.Analog.Update();
int v = m.Analog.Read(0,0);
```

### PWM
```csharp
m.Pwm.AngleInit(0,180,150,600);
m.Pwm.AngleOut(0, 90);
m.Pwm.Out(15, 375);
```

### I2C
```csharp
m.I2c.Write(0x40, 0x00, 0x01);
int val = m.I2c.Read(0x40, 0x00);
```

### オーディオ（**Read.* 階層化／FolderTrack→bool／SetLoop追加**）
```csharp
m.Audio.Volume(1, 20);
m.Audio.Play.FolderTrack(1,1,1);
m.Audio.Play.SetLoop(1, true);
int st = m.Audio.Read.PlayState(1);
m.Audio.Play.Stop(1);
```

---

## トラブルシュート
- **接続できない**：Arduino 版は `SERIAL1` のピン多重化・配線を確認（TX=GPIO0、RX=GPIO1）。  
- **ANU/ANS が失敗**：ファームの HC4067 配線と `players/switches` の上限（1..16 / 1..4）を確認。  
- **応答が空**：プロトコルは 5文字固定、前ゴミがあると捨てられます。ファームからの応答形式を再確認。  
- **高速時の不安定**：ケーブル長・GND・電源品質、DFPlayer 併用時の電源を確認。

---

## 変更履歴（重要な互換性ポイント）
- **DigitalIo を削除**（`In/Out` を利用）。
- **Audio.Read.* を導入**（`ReadPlayState/ReadVolume/...` は `Audio.Read.*` に移行）。
- **Audio.Play.FolderTrack** の戻り値を **`string` → `bool`** に変更（`"!!!!!"`→`true`）。
- **Audio.Play.SetLoop(dev, onOff)** を追加（新コマンド `DLP`）。
- **Arduino**：応答受信を「5文字揃うまで待つ」方式に修正（安定化）。`ConnectedBaud()` を追加。

---

## ライセンス／表記
- Copyright © MMP
- 本ドキュメントの内容は将来変更される場合があります。

---

**Enjoy the unified MMP API 🚀** – プラットフォームを気にせず、同じ発想・同じコードで。
