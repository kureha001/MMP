# Machikania Type-Pで扱うには
## ～作成中～
---
Machikania Type-PでMMPを利用する方法を解説します。

---
## 1. MMPとの接続
| Machikania     | MMP                |
| -------------- | ------------------ |
| RX GP1 7番ピン | GPIO0 (Serial1 TX) |
| TX GP0 6番ピン | GPIO1 (Serial1 RX) |
| GND            | GND                |

---
## 2. 通信設定
書式：**SERIAL** `<通信速度>`,`<制御モード>`,`<バッファサイズ>`
| 引数           |値 | MMP      |
| -------------- |---| -------- |
| 通信速度       |   | 0で切断  |
| 制御モード     |   | 0にする  |
| バッファサイズ |   | 1024推奨 |

---
## 3. コマンドの実行(送受信)
### 3-1. コマンドの送信
一括送信が無いので、1文字ずつ送信します。

### 3-2. コマンド結果の受信
一括読取りが無いので、1文字ずつ読み取ります。


---
## 4. サンプルコード
```bas
CLS
SERIAL 115200,0,1024
C$="DIR:1:4:1!"
L=LEN(C$)

FOR I=1 TO LEN(C$)
 A=ASC(C$(I-1,1))
 PRINT A;:PRINT " ";
 SERIALOUT A
NEXT

PRINT ""
PRINT "COMMAND:" + C$ + "=";

J=0
FOR I=1 TO 100
 A=SERIALIN()
 IF A>=0 THEN
  PRINT CHR$(A);
  J=J+1
  IF J>=5 THEN BREAK
 ELSE
  PRINT ".";
  WAIT 5
 ENDIF
NEXT

SERIAL 0
```