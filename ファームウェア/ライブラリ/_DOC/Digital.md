# MMP ライブラリ API マニュアル
## Digital モジュール（デジタル入出力）
MMPに搭載したマイコンのGPIOを用い、デジタル信号を入出力します。
使用できるGPIOは搭載したマイコンに依存します。

---
### 1 入力
**解説**：
GPIOより、デジタル信号を読取ります。

**書式**：
- `int     In(int     gpioId, int     timeoutMs = 0)`
- `int32_t In(int32_t gpioId, int32_t timeoutMs = 0)`
- `        In(        gpioId,         timeoutMs = 0)`

| 引数名    | 値  | 解説 |
|-----------|-----|------|
|`gpioId`   |0～  | MMPに搭載したマイコンのGPIOのポート番号|
|`timeoutMs`|0～  | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `0x0000–0xFFFF`  | 成功 |
| <<右記参照>>     | 失敗時の扱いは**付録 A**参照 |

### 2 出力
**解説**：
GPIOより、デジタル信号を出力します。

**書式**
- `bool Out(int     gpioId, int     val0or1, int     timeoutMs = 0)`
- `bool Out(int32_t gpioId, int32_t val0or1, int32_t timeoutMs = 0)`
- `     Out(        gpioId,         val0or1,         timeoutMs = 0)`

| 引数名  | 値  | 解説 |
|---------|-----|------|
|`gpioId` |0～  |MMPに搭載したマイコンのGPIOのポート番号|  
|`val0or1`|`0`＝LOW <br> `1`＝HIGH | 出力信号レベル|
|`timeoutMs`|0～  | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |