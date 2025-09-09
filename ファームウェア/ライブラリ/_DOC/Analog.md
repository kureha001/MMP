# MMP ライブラリ API マニュアル
## Analog モジュール（アナログ入力ＤＡＣ）
MMPに搭載した４個のマルチプレクサ(HC4067)を制御します。
構成をセットしたら、更新することでアナログ信号をバッファに取り込みます。
取り込んだバッファを読取り、アプリケーションの処理に利用します。
読取りには丸めを指定することもできます。

---
### １ 構成
**解説**：
アナログ入力の範囲を事前に設定します。
最大16x4=64chのアナログ信号を処理できますが、これより少ない利用の場合、処理速度を向上させるために、最低限のチャンネル数を定義します。

**書式**：
- `bool Configure(int     hc4067chTtl, int     hc4067devTtl, int     timeoutMs = 0)`
- `bool Configure(int32_t hc4067chTtl, int32_t hc4067devTtl, int32_t timeoutMs = 0)`
- `     Configure(        hc4067chTtl,         hc4067devTtl,         timeoutMs = 0)`

| 引数名    | 値    | 解説 |
|-----------|-------|------|
| `hc4067chTtl` | 1〜16 | RJ45のポートID - 1 および<br>`HC4067`のチャンネルID:0～15に相当|
| `hc4067devTtl`| 1〜4  | `HC4067`のデバイス(n個目)に相当 |
| `timeoutMs`   | 0～   | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


---
### ２ 更新
**解説**：
構成で設定した範囲でアナログ信号をバッファに取り込みます。

**書式**：
- `bool Update(int     timeoutMs = 0)`
- `bool Update(int32_t timeoutMs = 0)`
- `     Update(        timeoutMs = 0)`

| 引数名      | 値  | 解説 |
|-------------|-----|------|
| `timeoutMs` | 0～ | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |

---
### ３ 読取り

#### 3-1. 読取り(生データ)
**解説**：
更新で取り込んだバッファの値を参照します。

**書式**：
- `int     Read(int     hc4067ch0to15, int     hc4067dev0to3, int     timeoutMs = 0)`
- `int32_t Read(int32_t hc4067ch0to15, int32_t hc4067dev0to3, int32_t timeoutMs = 0)`
- `        Read(        hc4067ch0to15,         hc4067dev0to3,         timeoutMs = 0)`

| 引数名 | 値 | 解説 |
|--------|-----|------|
| `hc4067ch0to15` | 0～15 | HC4067 のチャンネルID |
| `hc4067dev0to3` | 0～3  | HC4067 のデバイスID |
| `timeoutMs` | 0～ | 応答待ちの時間(ミリ秒) |

| 戻り値   | 解説 |
|----------|------|
| `0–4095` | 成功：アナログ信号の値 |
| `-1`     | 失敗 |


#### 3-2. 読取り(丸め：中央値基準)
**解説**：
更新で取り込んだバッファの値を丸めて参照します。
中央値基準で切り上げ・切り下げます。

**書式**：
- `int     ReadRound(int     hc4067ch0to15, int     hc4067dev0to3, int     step, int     bits = 0, int     timeoutMs = 0)`
- `int32_t ReadRound(int32_t hc4067ch0to15, int32_t hc4067dev0to3, int32_t step, int32_t bits = 0, int32_t timeoutMs = 0)`
- `        ReadRound(        hc4067ch0to15,         hc4067dev0to3,         step,         bits = 0,         timeoutMs = 0)`

| 引数名 | 値 | 解説 |
|--------|-----|------|
| `hc4067ch0to15` | 0～15 | HC4067 のチャンネルID |
| `hc4067dev0to3` | 0～3  | HC4067 のデバイスID |
| `step` | 1～ | 丸めのステップ幅（偶数/奇数いずれも可）|
| `bits` | 1～16 | 分解能ビット数（`bits` が 1..16 以外は Settings.AnalogBits を使用）|
| `timeoutMs` | 0～ | 応答待ちの時間(ミリ秒) |

| 戻り値   | 解説 |
|----------|------|
| `0–4095` | 成功：アナログ信号の値 |
| `-1`     | 失敗 |

#### 3-3. 読取り(丸め：切り上げ)
**解説**：
更新で取り込んだバッファの値を丸めて参照します。
丸めた結果、端数は切り上げます。

**書式**：
- `int     ReadRoundUp(int     hc4067ch0to15, int     hc4067dev0to3, int     step, int     bits = 0, int     timeoutMs = 0)`
- `int32_t ReadRoundUp(int32_t hc4067ch0to15, int32_t hc4067dev0to3, int32_t step, int32_t bits = 0, int32_t timeoutMs = 0)`
- `        ReadRoundUp(        hc4067ch0to15,         hc4067dev0to3,         step,         bits = 0,         timeoutMs = 0)`

| 引数名 | 値 | 解説 |
|--------|-----|------|
| `hc4067ch0to15` | 0～15 | HC4067 のチャンネルID |
| `hc4067dev0to3` | 0～3  | HC4067 のデバイスID |
| `roundStep` | 1～ | 丸めのステップ幅（偶数/奇数いずれも可）|
| `bits` | 1～16 | 分解能ビット数（`bits` が 1..16 以外は Settings.AnalogBits を使用）|
| `timeoutMs` | 0～ | 応答待ちの時間(ミリ秒) |

| 戻り値   | 解説 |
|----------|------|
| `0–4095` | 成功：アナログ信号の値 |
| `-1`     | 失敗 |


#### 3-4. 読取り(丸め：切り下げ)
**解説**：
更新で取り込んだバッファの値を丸めて参照します。
丸めた結果、端数は切り下げます。

**書式**：
- `int     ReadRoundDown(int     hc4067ch0to15, int     hc4067dev0to3, int     step, int     bits = 0, int     timeoutMs = 0)`
- `int32_t ReadRoundDown(int32_t hc4067ch0to15, int32_t hc4067dev0to3, int32_t step, int32_t bits = 0, int32_t timeoutMs = 0)`
- `        ReadRoundDown(        hc4067ch0to15,         hc4067dev0to3,         step,         biti = 0,         timeoutMs = 0)`

| 引数名 | 値 | 解説 |
|--------|-----|------|
| `hc4067ch0to15` | 0～15 | HC4067 のチャンネルID |
| `hc4067dev0to3` | 0～3  | HC4067 のデバイスID |
| `roundStep` | 1～ | 丸めのステップ幅（偶数/奇数いずれも可）|
| `bits` | 1～16 | 分解能ビット数（`bits` が 1..16 以外は Settings.AnalogBits を使用）|
| `timeoutMs` | 0～ | 応答待ちの時間(ミリ秒) |

| 戻り値   | 解説 |
|----------|------|
| `0–4095` | 成功：アナログ信号の値 |
| `-1`     | 失敗 |