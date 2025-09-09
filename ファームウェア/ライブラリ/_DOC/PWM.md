# MMP ライブラリ API マニュアル
## PWM モジュール（ＰＷＭ出力）
MMPの搭載したPWM出力デバイス(PCA9685)を制御します。
デバイスは最大で１６個まで増設できます。
それぞれのチャンネル番号を0から連番で扱います。

---
### 1 PWM値出力
**解説**
チャンネル番号からPWM値を出力させます。

**書式**：
- `bool Out(int     chId0to255, int     val0to4095, int     timeoutMs = 0)`
- `bool Out(int32_t chId0to255, int32_t val0to4095, int32_t timeoutMs = 0)`
- `     Out(        chId0to255,         val0to4095,         timeoutMs = 0)`

| 引数名     | 値    | 解説 |
|------------|-------|------|
|`chId0to255`|0〜255 | チャンネル番号(機器ID横断の通し番号)|  
|`val0to4095`|0〜4095| 出力値<br>0：GNDレベル<br>4095：最大電圧で常時出力|
|`timeoutMs`   |0～    | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |

### 2 角度初期化
**解説**
サーボモータの角度指定制御に備え、諸条件をセットします。

**書式**：
- `bool AngleInit(int     angleMin, int     angleMax, int     pwmMin, int     pwmMax, int     timeoutMs = 0)`
- `bool AngleInit(int32_t angleMin, int32_t angleMax, int32_t pwmMin, int32_t pwmMax, int32_t timeoutMs = 0)`
- `     AngleInit(        angleMin,         angleMax,         pwmMin,         pwmMax,         timeoutMs = 0)`

| 引数名    | 値         | 解説 |
|-----------|------------|------|
|`angleMin` |`0`〜`180`  | 角度範囲の下限 |
|`angleMax` |<<同上>>    | 角度範囲の上限 |
|`pwmMin`   |`150`〜`600`| PWM 範囲の下限 |
|`pwmMax`   |<<同上>>    | PWM 範囲の上限 |  
|`timeoutMs`|0～         | 応答待ちの時間(単位：ミリ秒)|

| 戻り    | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


### 3 角度出力
**解説**
サーボモータを角度指定でPWM出力します。

**書式**：
- `bool AngleOut(int     chId0to255, int     angle0to180, int     timeoutMs = 0)`
- `bool AngleOut(int32_t chId0to255, int32_t angle0to180, int32_t timeoutMs = 0)`
- `     AngleOut(        chId0to255,         angle0to180,         timeoutMs = 0)`

| 引数名      | 値   | 解説 |
|-------------|------|------|
|`chId0to255` |0〜255| チャンネル番号(機器ID横断の通し番号)|  
|`angle0to180`|0〜180| 角度 |  
|`timeoutMs`  |0～   | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |