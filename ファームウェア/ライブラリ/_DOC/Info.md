# MMP ライブラリ API マニュアル
## Info モジュール（機器情報参照）
MMPの機器情報を提供する

---
### 1 バージョン取得
**解説**：
ファームウェア・バージョン取得  

**書式**：  
- `string Version(int     timeoutMs = 0)`
- `String Version(int32_t timeoutMs = 0)`

| 引数名      | 値  | 解説 |
|-------------|-----|------|
| `timeoutMs` | 0～<br>(0で`Settings.TimeoutGeneral`) |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `"XYZZ!"` | 成功(X:メジャー番号、Y:マイナー番号、ZZ:リビジョン番号) |
| `"!!!!!"` | 失敗 |

### 2 デバイス情報(PCA9685)
**解説**：
PCA9685 接続情報を取得  

**書式**：
- `ushort   Pwm(int     devId0to15, int     timeoutMs = 0)`
- `uint16_t Pwm(int32_t devId0to15, int32_t timeoutMs = 0)`

| 引数名      | 値    | 解説 |
|-------------|-------|------|
| `devId0to15`| 0～15 | MMPに搭載したPCA9685のデバイスID|
| `timeoutMs` | 0～   | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `0x0000–0xFFFF`  | 成功 |
| <<右記参照>>     | 失敗時の扱いは**付録 A**参照 |

### 3 デバイス情報(DFPlayer)
**解説**：
DFPlayer 接続情報を取得  

**書式**：
- `ushort   Audio(int     devId1to4, int     timeoutMs = 0)`
- `uint16_t Audio(int32_t devId1to4, int32_t timeoutMs = 0)`

| 引数名      | 値    | 解説 |
|-------------|-------|------|
| `devId1to4`  | 1〜4  | MMPに搭載したDFPlayerのデバイスID|
| `timeoutMs` | 0～   | 応答待ちの時間(単位：ミリ秒)|

| 戻り値          | 解説 |
|-----------------|------|
| `0x0000–0xFFFF` | 成功 |
| <<右記参照>>    | 失敗時の扱いは**付録 A**参照 |