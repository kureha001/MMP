# MMP ライブラリ API マニュアル
## SYS モジュール（システム管理）
MMPのシステム情報の参照・動作制御を提供する

---
### １ バージョン取得
**解説**：
ファームウェア・バージョン取得  

**書式**：  
- `string VERSION(int     timeoutMs = 0)`
- `String VERSION(int32_t timeoutMs = 0)`
- `       VERSION(        timeoutMs = 0)`

| 引数名      | 値  | 解説 |
|-------------|-----|------|
| `timeoutMs` | 0～<br>(0で`Settings.TimeoutGeneral`) |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `"XYZZ!"` | 成功(X:メジャー番号、Y:マイナー番号、ZZ:リビジョン番号) |
| `"!!!!!"` | 失敗 |

---
### ２ 再起動
**解説**：
ＭＭＰを再起動します。  

**書式**：  
- `bool BOOT(int     timeoutMs = 0)`
- `bool BOOT(int32_t timeoutMs = 0)`
- `     BOOT(        timeoutMs = 0)`

| 引数名      | 値  | 解説 |
|-------------|-----|------|
| `timeoutMs` | 0～<br>(0で`Settings.TimeoutGeneral`) |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |


---
### ３ ログ出力レベル設定
**解説**：
ログ出力レベルをセットします。  

**書式**：  
- `bool LOG(int lvLog, int     timeoutMs = 0)`
- `bool LOG(int lvLog, int32_t timeoutMs = 0)`
- `     LOG(int lvLog,         timeoutMs = 0)`

| 引数名      | 値  | 解説 |
|-------------|-----|------|
|`lvLog`      |`0`〜`5`  | 0:出力無し<br>1～5:プロセスID |
| `timeoutMs` | 0～<br>(0で`Settings.TimeoutGeneral`) |応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |
