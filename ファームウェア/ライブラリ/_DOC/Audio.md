# MMP ライブラリ API マニュアル
## Audio モジュール(音声プレイヤー)
MMPに搭載した音声デバイス(DFPlayer)を制御します。
デバイスは最大で４個まで増設できます。

---
### 1-1. 情報
**解説**
デバイス単位で、接続されているかを調べます。

**書式**：
- `ushort   Info(int     devId1to4, int     timeoutMs = 0)`
- `uint16_t Info(int32_t devId1to4, int32_t timeoutMs = 0)`
- `         Info(        devId1to4,         timeoutMs = 0)`

| 引数名    | 値  | 解説 |
|-----------|-----|------|
|`devId1to4`|1〜4 | DFPlayerのデバイスID|
|`timeoutMs`|0～  |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
|`0x0000–0xFFFF`| 成功 |
|<<右記参照>>   | 失敗時の扱いは**付録 A**参照 |

### 1-2. 音量設定
**解説**
デバイスごとに音量を設定します。

**書式**
- `bool Volume(int     devId1to4, int     vol0to30, int     timeoutMs = 0)`
- `bool Volume(int32_t devId1to4, int32_t vol0to30, int32_t timeoutMs = 0)`
- `     Volume(        devId1to4,         vol0to30,         timeoutMs = 0)`

| 引数名    | 値   | 解説 |
|-----------|------|------|
|`devId1to4`|1〜4  |MMPに搭載したDFPlayerのデバイスID |
|`vol0to30` |0〜30 | 音量                             | 
|`timeoutMs`|0～   |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


### 1-3. EQ設定
**解説**
デバイスごとにイコライザーの音質を設定します。

**書式**：
- `bool SetEq(int     devId1to4, int     mode0to5, int     timeoutMs = 0)`
- `bool SetEq(int32_t devId1to4, int32_t mode0to5, int32_t timeoutMs = 0)`
- `     SetEq(        devId1to4,         mode0to5,         timeoutMs = 0)`

| 引数名    | 値   | 解説 |
|-----------|------|------|
|`devId1to4`|1〜4  |MMPに搭載したDFPlayerのデバイスID |
|`mode0to5` |0〜5  | イコライザーのモード|
|`timeoutMs`|0～   |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |

---
### ２ 再生サブ
カレントファイルを制御します。

#### 2-1. 開始
**解説**
デバイスごとに曲を再生します。
曲はmicroSDカード内のフォルダとファイル位置で指定します。

**書式**：
- `bool Start(int     devId1to4, int     dir1to255, int     file1to255, int     timeoutMs = 0)`
- `bool Start(int32_t devId1to4, int32_t dir1to255, int32_t file1to255, int32_t timeoutMs = 0)`
- `     Start(        devId1to4,         dir1to255,         file1to255,         timeoutMs = 0)`

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


#### 2-1. ループ再生設定
**解説**
デバイスごとに再生中の曲に対して、リーピート再生の有無を指定します。

**書式**：
- `bool SetLoop(int     devId1to4, bool enable, int     timeoutMs = 0)`
- `bool SetLoop(int32_t devId1to4, bool enable, int32_t timeoutMs = 0)` 
- `     SetLoop(        devId1to4,      enable,         timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`enable`   | `true`＝ループ<br>`false`＝ループなし| 現在の再生トラックに設定する |  
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


#### 2-3. 停止
**解説**
デバイスごとに再生中の曲を停止します。

**書式**
- `bool Stop(int     devId1to4, int     timeoutMs = 0)`
- `bool Stop(int32_t devId1to4, int32_t timeoutMs = 0)` 
- `     Stop(        devId1to4,         timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


#### 2-4. 一時停止
**解説**
デバイスごとに再生中の曲を一時停止します。

**書式**
- `bool Pause(int     devId1to4, int     timeoutMs = 0)`
- `bool Pause(int32_t devId1to4, int32_t timeoutMs = 0)`
- `     Pause(        devId1to4,         timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


#### 2-5. 再開
**解説**
デバイスごとに一時停止している曲を再生します。

**書式**
- `bool Resume(int     devId1to4, int     timeoutMs = 0)`
- `bool Resume(int32_t devId1to4, int32_t timeoutMs = 0)`
- `     Resume(        devId1to4,         timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


---
### ３ 状態参照サブ
デバイスのさまざまな状態を参照します。

#### 3-1.  再生状態
**解説**：
再生状態を参照する。

**書式**
- `int     State(int     devId1to4, int     timeoutMs = 0)`
- `int32_t State(int32_t devId1to4, int32_t timeoutMs = 0)`
- `        State(        devId1to4,         timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
|||
|||
|||
|||
|||


#### 3-2. 音量
**解説**：
音量を参照する。

- `int     Volume(int     devId1to4, int     timeoutMs = 0)`
- `int32_t Volume(int32_t devId1to4, int32_t timeoutMs = 0)`
- `        Volume(        devId1to4,         timeoutMs =0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| 0～30   | 音量 |


#### 3-3. イコライザー モード
**解説**：
イコライザーのモードを参照する。

- `int     Eq(int     devId1to4, int     timeoutMs = 0)`
- `int32_t Eq(int32_t devId1to4, int32_t timeoutMs = 0)`
- `        Eq(        devId1to4,         timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説   |
|---------|--------|
| 0～255  | モード |


#### 3-4. 総ファイル数
**解説**：
microSDカード内の総ファイル数を参照する。

- `int     FileCounts(int     devId1to4, int     timeoutMs = 0)`
- `int32_t FileCounts(int32_t devId1to4, int32_t timeoutMs = 0)`
- `        FileCounts(        devId1to4,         timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説       |
|---------|------------|
| 0～255  | ファイル数 |


#### 3-5. 現在ファイル番号
**解説**： 
現在ファイルのID(番号)を参照する。

- `int     FileNumber(int     devId1to4, int     timeoutMs = 0)`
- `int32_t FileNumber(int32_t devId1to4, int32_t timeoutMs = 0)`
- `        FileNumber(        devId1to4,         timeoutMs = 0)`

| 引数名    | 値 | 解説 |
|-----------|----|------|
|`devId1to4`|1〜4|MMPに搭載したDFPlayerのデバイスID |
|`timeoutMs`|0～ |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説         |
|---------|--------------|
| 0～255  | ファイル番号 |
