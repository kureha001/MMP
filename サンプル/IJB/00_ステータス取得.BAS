10 UART 1
20 ? "STA!"     ' DFPlayer接続ステータス（1または0）
30 WAIT 10
40 ? "PWS!"     ' PCA9685接続ステータス（ビット列）
50 WAIT 10
60 ? "VER!"     ' ファームウェアバージョン
70 WAIT 10
80 IF INKEY()>0 GOTO 100
90 GOTO 20
100 A=INKEY():?CHR$(A)
