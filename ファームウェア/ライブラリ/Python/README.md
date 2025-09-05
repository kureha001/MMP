
# 第2世代：MMP Python用 Library

**統一階層 API**（C# / Arduino と同一の構造）で、Python 3系、MicroPython、CircuitPython から
MMP ファームウェアへシリアル接続し、固定 5 文字フレームのコマンドをやり取りします。

## 特徴
- **プラットフォーム横断**：CPython・MicroPython・CircuitPython で同じ API。
- **階層 API**：`Info`, `Analog`, `Pwm`, `Audio(Play/Read)`, `Digital`, `I2c`。
- **オートボーレート**：`ConnectAutoBaud()` が `115200/9600/230400/921600` を順次試行。
- **厳密な 5 文字フレーム**：断片化に強い受信（5 文字 & 末尾 `!` を確認）。
- **最新仕様**：
  - `Digital.Io()` **廃止**（`In/Out` のみ）
  - `Audio.Read.*` に **再編**（`PlayState/Volume/Eq/FileCounts/CurrentFileNumber`）
  - `Audio.Play.SetLoop(dev, onOff)` を **追加**（ファーム側コマンド `DLP`）

## ざっくり使い方

### CPython
```python
# pip install pyserial
from mmpC import MmpClient, CpyAdapter

cli = MmpClient(CpyAdapter(port=None, preferred_ports=["COM6","/dev/ttyUSB0"]))
if not cli.ConnectAutoBaud():
    raise SystemExit("connect failed: " + cli.LastError)

print("VER:", cli.Info.Version())
print("PWX(0):", hex(cli.Info.Dev.Pwm(0)))
print("DPX(1):", hex(cli.Audio.Info(1)))

cli.Audio.Volume(1, 20)
cli.Audio.Play.FolderTrack(1, 1, 1)
cli.Audio.Play.SetLoop(1, 1)  # 1=loop on
print("PlayState:", cli.Audio.Read.PlayState(1))
cli.Audio.Play.Stop(1)
cli.Close()
```

### MicroPython
```python
from mmpMicro import MmpClient, MicroPyAdapter
cli = MmpClient(MicroPyAdapter(uart_id=0, tx=0, rx=1))
cli.ConnectAutoBaud()
print(cli.Info.Version())
```

### CircuitPython
```python
import board
from mmpCircuit import MmpClient, CircuitPyAdapter
cli = MmpClient(CircuitPyAdapter(tx_pin=board.TX, rx_pin=board.RX))
cli.ConnectAutoBaud()
print(cli.Info.Version())
```

## API リファレンス（抜粋）

- `ConnectAutoBaud(candidates=(115200,9600,230400,921600)) -> bool`
- `ConnectWithBaud(baud, timeoutIo=0, verifyTimeoutMs=0) -> bool`
- `Close()`
- `ConnectedBaud`（接続した実効ボーレート）

### Info
- `Info.Version()` → `"hhhh!"`
- `Info.Dev.Pwm(id)` → `int`
- `Info.Dev.Audio(id)` → `int`

### Analog
- `Analog.Configure(players, switches) -> bool`
- `Analog.Update() -> bool`
- `Analog.Read(playerIndex0..15, switchIndex0..3) -> int`

### Pwm
- `Pwm.Info(id) -> int`
- `Pwm.Out(ch, value0..4095) -> "!!!!!"`
- `Pwm.AngleInit(aMin, aMax, pMin, pMax) -> bool`
- `Pwm.AngleOut(ch, angle0..180) -> "!!!!!"`

### Audio
- `Audio.Info(id) -> int`
- `Audio.Volume(dev, vol0..30) -> bool`
- `Audio.SetEq(dev, eq0..5) -> bool`
- `Audio.Play.FolderTrack(dev, folder, track) -> bool`
- `Audio.Play.Stop(dev) / Pause(dev) / Resume(dev) -> bool`
- `Audio.Play.SetLoop(dev, onOff) -> bool`  **← 新規**

#### Audio.Read
- `Audio.Read.PlayState(dev) -> int`
- `Audio.Read.Volume(dev) -> int`
- `Audio.Read.Eq(dev) -> int`
- `Audio.Read.FileCounts(dev) -> int`
- `Audio.Read.CurrentFileNumber(dev) -> int`

### I2c
- `I2c.Write(addr, reg, value) -> bool`
- `I2c.Read(addr, reg) -> int`

## ライセンス
BSD-3-Clause（必要に応じて変更してください）
