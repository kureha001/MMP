# ＭＳＸで扱うには
～作成中～

### (使用言語)
MSX BASICを使用します。
DISK BASIC/ROM BASIC どちらでもOKです。

### (ボーレート設定)
DIPスイッチやジャンパースイッチがない。
確認中

### (サンプルコード)
```bas
10 OPEN "COM1:9600,N,8,1" FOR INPUT AS #1
20 PRINT #1, "VER!!"
30 LINE INPUT #1, A$
40 PRINT A$
50 CLOSE #1
```