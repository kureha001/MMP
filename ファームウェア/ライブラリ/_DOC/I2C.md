# MMP ライブラリ API マニュアル
## I2c モジュール（Ｉ２Ｃ通信）
MMPに接続したi2cデバイスを制御します。
接続したデバイスの仕様に従います。

### 1 書込み
**解説**
I2Cデバイスへデータを書き込みます。

**書式**：
- `bool Write(int     addr, int     reg, int     val, int     timeoutMs = 0)`
- `bool Write(int32_t addr, int32_t reg, int32_t val, int32_t timeoutMs = 0)`

| 引数名 | 値  | 解説 |
|--------|-----|------|
| `addr`  | 0x00–0x7F | 7bit アドレス|  
| `reg`   | 0x00–0xFF | レジスタ番地 |
| `val` | 0x00–0xFF | 書き込む値   |
| `timeoutMs` | 0～ | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


### 2 読取り
**解説**
I2Cデバイスからデータを読み取ります。

**書式**
- `int     Read(int     addr, int     reg, int    timeoutMs = 0)`
- `int32_t Read(int32_t addr, int32_t reg, int32_t timeoutMs = 0)`

| 引数名 | 値  | 解説 |
|--------|-----|------|
| `addr`  | 0x00–0x7F | 7bit アドレス|  
| `reg`   | 0x00–0xFF | レジスタ番地 |
| `timeoutMs` | 0～ | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `0x00–0xFF`  | 成功                          |
| <<右記参照>> | 失敗時の扱いは**付録 A**参照 |