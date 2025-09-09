# MMP ライブラリ API マニュアル
## Audio モジュール(音声プレイヤー)
MMPに搭載した音声デバイス(DFPlayer)を制御します。
デバイスは最大で４個まで増設できます。

---
### 1 情報
**解説**
デバイス単位で、接続されているかを調べます。

**書式**：
- `ushort Info(int devId1to4, int timeoutMs = 0)`  

| 引数名    | 値  | 解説 |
|-----------|-----|------|
|`devId1to4`|1〜4 | DFPlayerのデバイスID|
|`timeoutMs`|0～  |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
|`0x0000–0xFFFF`| 成功 |
|<<右記参照>>   | 失敗時の扱いは**付録 A**参照 |


### 2 開始
**解説**
デバイスごとに曲を再生します。
曲はmicroSDカード内のフォルダとファイル位置で指定します。

**書式**：
int timeoutMs = 0)`  
- `bool Start(int devId1to4, int dir1to255, int file1to255, int timeoutMs = 0)`

| 引数名     | 値   | 解説 |
|------------|------|------|
|`devId1to4` |1〜4  |MMPに搭載したDFPlayerのデバイスID |
|`dir1to255` |1〜255|microSDカード内のディレクトリID   |
|`file1to255`|1〜255|ディレクトリ内のファイルID        |
|`timeoutMs` |0～   |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


### 3 ループ再生設定
**解説**
デバイスごとに再生中の曲に対して、リーピート再生の有無を指定します。

**書式**：
`bool SetLoop(int devId1to4, bool enable, int timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`enable`   | `true`＝ループ<br>`false`＝ループなし| 現在の再生トラックに設定する |  
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


### 4 停止
**解説**
デバイスごとに再生中の曲を停止します。

**書式**
- `bool Stop(int devId1to4, int timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


### 5 一時停止
**解説**
デバイスごとに再生中の曲を一時停止します。

**書式**
- `bool Pause(int devId1to4, int timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


### 6 再開
**解説**
デバイスごとに一時停止している曲を再生します。

**書式**
- `bool Resume(int devId1to4, int timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


### 7 音量設定
**解説**
デバイスごとに音量を設定します。

**書式**
- `bool Volume(int devId1to4, int vol0to30, int timeoutMs = 0)`

| 引数名    | 値   | 解説 |
|-----------|------|------|
|`devId1to4`|1〜4  |MMPに搭載したDFPlayerのデバイスID |
|`vol0to30` |0〜30 | 音量                             | 
|`timeoutMs`|0～   |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


### 8 EQ 設定
**解説**
デバイスごとにイコライザーの音質を設定します。

**書式**：
- `bool SetEq(int devId1to4, int eq0to5, int timeoutMs = 0)`

| 引数名    | 値   | 解説 |
|-----------|------|------|
|`devId1to4`|1〜4  |MMPに搭載したDFPlayerのデバイスID |
|`mode0to5` |0〜5  | イコライザーのモード|
|`timeoutMs`|0～   |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


### 9 状態参照(Read サブモジュール)
**用途**：再生状態や音量等を参照  
**書式**
- `int PlayState (int devId1to4, int timeoutMs = 0)`
- `int Volume    (int devId1to4, int timeoutMs = 0)`
- `int Eq        (int devId1to4, int timeoutMs = 0)`
- `int FileCounts(int devId1to4, int timeoutMs = 0)`
- `int FileNumber(int devId1to4, int timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| 0 以上の値 | 成功 |
| `-1`       | 失敗(実装依存は**付録 A**参照) |

**解説**
デバイスごとの各種状態を参照します。