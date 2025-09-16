# PC-9801で扱うには
～作成中～

### (使用言語)
以下の2種類を実験中
- N88-DISK BASIC
- QuickBasic 4.5

### (ボーレート設定)
**～確認中～**
#### 1. N88-DISK BASIC
PC-98 では "COM1:" が一般的です。<br>
必要に応じて "COM2:" に。<br>
速度は 9600,8N1。動作はPC-88版と同一です。

#### 2. QuickBasic 4.5
COM1/9600bps想定。<br>
COM2や速度を変える場合は PORT$ / BAUD$ を差し替えてください。

**起動時の設定例（AUTOEXEC.BAT）**
```
@ECHO OFF
PATH C:\DOS;C:\QB45
REM --- COM1: 9600 / 8N1 / フロー制御なし ---
MODE COM1: BAUD=9600 PARITY=N DATA=8 STOP=1 ^
 TO=ON XON=OFF ODSR=OFF OCTS=OFF DTR=ON RTS=ON IDSR=OFF
```


### (サンプルコード)
**～確認中～**
