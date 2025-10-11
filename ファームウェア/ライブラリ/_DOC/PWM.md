# MMP ライブラリ API マニュアル
## PWM モジュール（ＰＷＭ出力）
MMPの搭載したPWM出力デバイス(PCA9685)を制御します。
デバイスは最大で１６個まで増設できます。
それぞれのチャンネル番号を0から連番で扱います。

---
### 1 PWM値出力
**解説**
チャンネル番号からPWM値を出力させます。

**書式**：`Out`
| 言語  | 戻値 |`ch`   |`pwm`  |`timeoutMs`|
|-------|------|-------|-------|-----------|
|C#     |bool  |int    |int    |int        |
|Arduino|bool  |int32_t|int32_t|int32_t    |
|Python |あり  |あり   |あり   |あり       |

| 引数名    | 有効範囲  |規定値| 解説 |
|-----------|-----------|------|------|
|`ch`       |`0`〜`127` |－    | チャンネル番号(機器ID横断の通し番号)|  
|`pwm`      |`0`〜`4095`|－    | 出力値<br>0:GNDレベル<br>4095:最大電圧で常時出力|
|`timeoutMs`|`0`～      |0     | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |

### 2 角度
#### 2-1. プリセット登録
**解説**
サーボモータの定格をプリセット登録します。

**書式**：
- `bool Angle.Init(int chFrom, int chTo, int degMax, int pwmMin, int pwmMax, int timeoutMs = 0)`
- `bool Angle.Init(int16_t chFrom, int16_t chTo, int32_t degMax, int32_t pwmMin, int32_t pwmMax, int32_t timeoutMs = 0)`
- `Angle.Init(degMin, degMax, pwmMin, pwmMax, timeoutMs = 0)`

| 引数名    | 値         | 解説 |
|-----------|------------|------|
|`chFrom`   |`0`〜`127`  | チャンネル番号(開始) |
|`chTo`     |`0`〜`127`  | チャンネル番号(終了) |
|`degMax`   |`0`〜`360`  | 角度の上限 |  
|`pwmMin`   |`150`〜`600`| PWM 範囲の下限 |
|`pwmMax`   |`150`〜`600`| PWM 範囲の上限 |  
|`timeoutMs`|`0`～       | 応答待ちの時間(単位：ミリ秒)|

| 戻り    | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


#### 2-2. プリセット削除
**解説**
プリセットを削除します。

**書式**：
- `bool Angle.Init(int     chFrom, int     chTo, int     timeoutMs = 0)`
- `bool Angle.Init(int16_t chFrom, int16_t chTo, int32_t timeoutMs = 0)`
- `     Angle.Init(        chFrom,         chTo,         timeoutMs = 0)`

| 引数名     | 値       | 解説 |
|------------|----------|------|
|`chFrom`|`0`〜`127`| チャンネル番号(開始) |
|`chTo`  |`0`〜`127`| チャンネル番号(終了) |
|`timeoutMs` |`0`～     | 応答待ちの時間(単位：ミリ秒)|

| 戻り    | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


#### 2-3. ＰＷＭ出力(角度指定)
**解説**
角度指定でPWM出力します。

**書式**：`Out`
| 言語  | 戻値 |`ch`   |`deg` |`timeoutMs`|
|-------|------|-------|-------|-----------|
|C#     |bool  |int    |int    |int        |
|Arduino|bool  |int16_t|int16_t|int32_t    |
|Python |あり  |あり   |あり   |あり       |

| 引数名    | 値       | 解説 |
|-----------|----------|------|
|`ch`       |`0`〜`127`| チャンネル番号 |
|`deg`      |`0`〜`360`| 角度 |  
|`timeoutMs`|`0`～     | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |

#### 2-4. ＰＷＭ出力(中間)
**解説**
サーボモータを中間の角度でPWM出力します。

**書式**：
- `bool Angle.Center(int     ch, int     timeoutMs = 0)`
- `bool Angle.Center(int32_t ch, int32_t timeoutMs = 0)`
- `     Angle.Center(        ch,         timeoutMs = 0)`

| 引数名      | 値       | 解説 |
|-------------|----------|------|
|`ch`       |`0`〜`127`| チャンネル番号 |
|`timeoutMs`  |`0`～     | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


### 3 ローテーションサーボ
#### 3-1. プリセット登録
**解説**
サーボモータの定格をプリセット登録します。

**書式**：
- `bool Rotation.Init(int degMin, int degMax, int pwmMin, int pwmMax, int timeoutMs = 0)`
- `bool Rotation.Init(int32_t degMin, int32_t degMax, int32_t pwmMin, int32_t pwmMax, int32_t timeoutMs = 0)`
- `Rotation.Init(degMin, degMax, pwmMin, pwmMax, timeoutMs = 0)`

| 引数名    | 値         | 解説 |
|-----------|------------|------|
|`degMin`   |`0`〜`360`  | 角度範囲の下限 |
|`degMax`   |`0`〜`360`  | 角度範囲の上限 |
|`pwmMin`   |`150`〜`600`| PWM 範囲の下限 |
|`pwmMax`   |`150`〜`600`| PWM 範囲の上限 |  
|`timeoutMs`|`0`～       | 応答待ちの時間(単位：ミリ秒)|

| 戻り    | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |


#### 3-2. プリセット削除
**解説**
プリセットを削除します。

**書式**：
- `bool Rotation.Init(int     chFrom, int     chTo, int     timeoutMs = 0)`
- `bool Rotation.Init(int16_t chFrom, int16_t chTo, int32_t timeoutMs = 0)`
- `     Rotation.Init(        chFrom,         chTo,         timeoutMs = 0)`

| 引数名    | 値       | 解説 |
|-----------|----------|------|
|`chFrom`   |`0`〜`127`| チャンネル番号(開始) |
|`chTo`     |`0`〜`127`| チャンネル番号(終了) |
|`timeoutMs`|`0`～     | 応答待ちの時間(単位：ミリ秒)|

| 戻り    | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |

#### 3-3. ＰＷＭ出力
**解説**
パーセンテージ指定でPWM出力します。

**書式**：`Out`
| 言語  | 戻値 |`ch`   |`rate` |`timeoutMs`|
|-------|------|-------|-------|-----------|
|C#     |bool  |int    |int    |int        |
|Arduino|bool  |int16_t|int16_t|int32_t    |
|Python |あり  |あり   |あり   |あり       |

| 引数名    | 値          | 解説 |
|-----------|-------------|------|
|`ch`       |`0`〜`127`   | チャンネル番号 |
|`rate`     |`-100`〜`100`| パーセンテージ |  
|`timeoutMs`|`0`～        | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |

#### 3-4. ＰＷＭ出力(中間)
**解説**
回転停止でPWM出力します。

**書式**：
- `bool Rotation.Stop(int     ch, int     timeoutMs = 0)`
- `bool Rotation.Stop(int32_t ch, int32_t timeoutMs = 0)`
- `     Rotation.Stop(        ch,         timeoutMs = 0)`

| 引数名     | 値       | 解説 |
|------------|----------|------|
|`ch`|`0`〜`127`| チャンネル番号 |
|`timeoutMs` |`0`～     | 応答待ちの時間(単位：ミリ秒)|

| 戻り値  | 解説 |
|---------|------|
| `true`  | 成功 |
| `false` | 失敗 |
