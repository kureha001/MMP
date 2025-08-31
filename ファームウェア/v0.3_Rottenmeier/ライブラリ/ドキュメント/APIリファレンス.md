# API リファレンス（共通）

> 戻り値は基本的に True/False か整数（16進変換結果）。接続系は `ConnectException` を投げる場合があります。

## 接続管理
- `通信接続(...): bool`  
  CPython 版: `通信接続(comm_num=None)` で自動/指定切替。  
  Micro/Circuit 版: ピン/ID 指定後に実行。
- `通信切断(): None`
- `バージョン確認(): bool` → `mmp.version` 更新。

## アナログ入力（HC4067）
- `アナログ設定(スイッチ数=4, 参加人数=1, 丸め=5): None`  
  `mmpAnaVal[参加人数][スイッチ数]` を確保し、`ANS:%02X:%02X!` を送信。
- `アナログ読取(): None`  
  `ANU!` 後に `ANR:%02X:%02X!` を全走査し `mmpAnaVal` を更新。

## PWM（PCA9685）
- `PWM_VALUE(ch, val): bool`
- `PWM_INIT(角度下限, 角度上限, PWM最小, PWM最大): bool`
- `PWM_ANGLE(ch, ang): bool`
- `PWM_POWER(ch, on): None` ← **新規**（ON=4095, OFF=0 出力）
- `PWM_機器確認(): list[bool]`（長さ 16）
- `PWM_チャンネル使用可否(ch): bool`

## デジタル I/O
- `digital_OUT(port, val): bool`
- `digital_IN(port): int`（0..65535）

## MP3（DFPlayer）
- `DFP_Info(dev): str`
- `DFP_PlayFolderTrack(dev, folder, track): str`
- `DFP_Stop(dev) / DFP_Pause(dev) / DFP_Resume(dev)`
- `DFP_Volume(dev, vol) / DFP_set_eq(dev, mode)`
- `DFP_get_play_state(dev, kind): str`（1:MP3,2:音量,3:EQ,4:総番号,5:フォルダ内番号）

## I2C（プロキシ）
- `i2cWrite(addr, reg, val): bool`（1 バイト）
- `i2cRead(addr, reg): int`（1 バイト）
